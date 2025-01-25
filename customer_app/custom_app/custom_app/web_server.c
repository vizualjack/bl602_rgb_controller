#include <FreeRTOS.h>
#include <stdbool.h>
#include <bl_mtd.h>
#include <hal_boot2.h>
#include <lwip/api.h>
#include <lwip/err.h>
#include <string.h>
#include <hal_sys.h>
#include <queue.h>
#include <lwip/sockets.h>

#include "connection.c"
#include "page.h"
#include "pwm.h"
#include "persistence.h"
#include "starter.h"

// #define REQUEST_HANDLER_PRIO 20
#define REQUEST_HANDLER_STACK_SIZE REAL_STACK(4096) // E 4096; NE 1024: SUCKS??? 2048; SUCKS??? 3072
#define RTOS_DELAY_MS 20
#define BUFFER_SIZE 1024

void rebooter() {
    puts("[Rebooter] Waiting a second...");
    sys_msleep(1000);
    puts("[Rebooter] Rebooting...");
    sys_msleep(500);
    hal_reboot();
}

void trigger_delayed_reboot() {
    sys_thread_new("rebooter", rebooter, NULL, 512, 29);
}

// void handle_get_requests(struct netconn *conn, const char* initial_data) {
void handle_get_requests(int fd, const char* buffer) {
    if(strstr(buffer, "GET /script.js") != NULL) {
        const char *response_header_template =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/javascript\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n";
        int content_length = strlen(js_script); 
        char response_header[100]; 
        snprintf(response_header, sizeof(response_header), response_header_template, content_length);
        // netconn_write(conn, response_header, strlen(response_header), NETCONN_NOCOPY);
        send(fd,response_header, strlen(response_header), 0);
        // netconn_write(conn, js_script, content_length, NETCONN_NOCOPY);
        send(fd, js_script, content_length, 0);
    } else if(strstr(buffer, "GET /switch_light") != NULL) {
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n"
            "Switched light";
        bool turn_on = strstr(buffer, "turn=on") != NULL;
        float pwm_duty = turn_on ? 10.f : 0.f;
        set_rgbw_duty(pwm_duty, pwm_duty, pwm_duty, pwm_duty);
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
    } 
    else {
        const char *response_header_template =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n";
        int content_length = strlen(html_page); 
        char response_header[100]; 
        snprintf(response_header, sizeof(response_header), response_header_template, content_length);
        // netconn_write(conn, response_header, strlen(response_header), NETCONN_NOCOPY);
        send(fd, response_header, strlen(response_header), 0);
        // netconn_write(conn, html_page, content_length, NETCONN_NOCOPY);
        send(fd, html_page, content_length, 0);
    }
}

void handle_ota_update(int fd, int content_length, char* buffer, int buffer_length) {
    printf("[OTA] buffer_length: %d\r\n", buffer_length);
    printf("[OTA] buffer: %s\r\n", buffer);
    printf("[OTA] content_length: %d\r\n", content_length);
	bl_mtd_handle_t handle;
	int ret;
	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &handle, BL_MTD_OPEN_FLAG_BACKUP);
	if(ret) {
        puts("Open Default FW partition failed\r\n");
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Connection: close\r\n\r\n"
            "Can't open partition. Please try again!";
        send(fd, error_response, strlen(error_response), 0);
        return;
	}
	unsigned int flash_offset, ota_addr;
	uint32_t bin_size;
	uint8_t activeID = hal_boot2_get_active_partition();
	HALPartition_Entry_Config ptEntry;
	if(hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &ptEntry))
	{
		bl_mtd_close(handle);
        puts("PtTable_Get_Active_Entries fail\r\n");
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Connection: close\r\n\r\n"
            "Can't get active entries. Please try again!";
        send(fd, error_response, strlen(error_response), 0);
        return;
	}
	ota_addr = ptEntry.Address[!ptEntry.activeIndex];
	bin_size = ptEntry.maxLen[!ptEntry.activeIndex];
    if(content_length >= bin_size)
    {
        puts("BIN file is too big");
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Connection: close\r\n\r\n"
            "File is too big";
        send(fd, error_response, strlen(error_response), 0);
        return;
    }
	printf("[OTA] Erasing flash (%lu)...\r\n", bin_size);
	hal_update_mfg_ptable();
	uint32_t erase_offset = 0;
	uint32_t erase_len = 0;
	while(erase_offset < bin_size)
	{
		erase_len = bin_size - erase_offset;
		if(erase_len > 0x10000) erase_len = 0x10000; //Erase in 64kb chunks
		bl_mtd_erase(handle, erase_offset, erase_len);
		erase_offset += erase_len;
        sys_msleep(200);
	}
	printf("[OTA] Flash erased\r\n");
	printf("[OTA] Flashing new firmware\r\n");
    // char* buf;
    u16_t buflen;
    int remaining = content_length;
	int total = 0;
    char *data_end;
    bool first = true;
    flash_offset = 0;
    bool initial_buffer_handled = false;
    int raw_total = 0;
    int highest_buffer = 0;
    int last_buffer_size = 0;
    // char buffer[BUFFER_SIZE];
    while (remaining > 0) {
        if(!initial_buffer_handled) {
            puts("[OTA] Using inital_body\n");
            // buf = (char*) initial_body;
            buflen = buffer_length;
            initial_buffer_handled = true;
            if(buflen <= 0) continue;
        } else {
            // puts("[OTA] Deleting current inbuf\n");
            // netbuf_delete(*inbuf);
            puts("[OTA] Cleaning buffer\n");
            memset(buffer, 0, BUFFER_SIZE);
            // buf = (char*) initial_body;
            puts("[OTA] Receiving next data\n");
            buflen = recv(fd, buffer, BUFFER_SIZE, 0);
            if(buflen <= 0) {
	            bl_mtd_close(handle);
                puts("[OTA] Can't receive next inbuf\n");
                const char *error_response =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Connection: close\r\n\r\n"
                    "Error while receiving request. Please try again!";
                send(fd, error_response, strlen(error_response), 0);
                return;
            }
        }
        // do {
            // if(!initial_buffer_handled) {
            //     buf = (char*)initial_body;
            //     buflen = initial_body_length;
            //     initial_buffer_handled = true;
            // }
            // else if(netbuf_data(*inbuf, (void **)&buf, &buflen) != ERR_OK) {
            //     bl_mtd_close(handle);
            //     const char *error_response =
            //         "HTTP/1.1 500 Internal Server Error\r\n"
            //         "Connection: close\r\n\r\n"
            //         "Error while receiving request. Please try again!";
            //     // netconn_write(conn, error_response, strlen(error_response), NETCONN_NOCOPY);
            //     send(fd, error_response, strlen(error_response), 0);
            //     return;
            // }
        raw_total += buflen;
        remaining -= buflen;
        printf("Remaining: %d\r\n", remaining);
        bl_mtd_write(handle, flash_offset, buflen, (u_int8_t*)buffer);
        flash_offset += buflen;
        total += buflen;
        // } while(netbuf_next(*inbuf) != -1);
        // netbuf_first(*inbuf);
    }
	printf("[OTA] Flashed new firmware\r\n");
	printf("[OTA] set OTA partition length\r\n");
	ptEntry.len = total;
	hal_boot2_update_ptable(&ptEntry);
	puts("[OTA] Flashed\r\n");
	bl_mtd_close(handle);
	puts("[OTA] Starting rebooter...\r\n");
    // netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
    trigger_delayed_reboot();
    puts("[OTA] Sending response...\r\n");
    const char *success_response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n\r\n"
        "Firmware uploaded successfully.";
    send(fd, success_response, strlen(success_response), 0);
    // if(inbuf == NULL) netbuf_delete(inbuf);
}

void handle_wifi_settings(int fd, const char* body) {
    puts("[httpd_handler] Handling wifi settings request...\n");
    // SSID
    char* ssid_start = strstr(body, "name=\"ssid\"\r\n\r\n");
    ssid_start += 15;
    if(ssid_start == NULL) {
        puts("Can't find SSID");
        const char *success_response =
            "HTTP/1.1 500 OK\r\n"
            "Connection: close\r\n\r\n"
            "No ssid provided.";
        // netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
        send(fd, success_response, strlen(success_response), 0);
        return;
    }
    char* ssid_end = strstr(ssid_start, "\r\n------");
    if(ssid_end == NULL) {
        puts("Can't find SSID end");
        printf("Start: %s\r\n", ssid_start);
        const char *success_response =
            "HTTP/1.1 500 OK\r\n"
            "Connection: close\r\n\r\n"
            "No ssid provided.";
        // netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
        send(fd, success_response, strlen(success_response), 0);
        return;
    }
    int ssid_len = ssid_end - ssid_start;
    printf("SSID length: %d\r\n", ssid_len);
    char ssid[ssid_len + 1];
    memcpy(ssid, ssid_start, ssid_len);
    ssid[ssid_len] = '\0';
    printf("Wifi SSID: %s\r\n", ssid);
    // PASSWORD
    char* pass_start = strstr(ssid_end, "name=\"pass\"\r\n\r\n");
    if(pass_start == NULL) {
        puts("Can't find PASS\r\n");
        const char *success_response =
            "HTTP/1.1 500 OK\r\n"
            "Connection: close\r\n\r\n"
            "No password provided.";
        // netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
        send(fd, success_response, strlen(success_response), 0);
        return;
    }
    pass_start += 15;
    char* pass_end = strstr(pass_start, "\r\n------");
    if(pass_end == NULL) {
        puts("Can't find PASS end\r\n");
        const char *success_response =
            "HTTP/1.1 500 OK\r\n"
            "Connection: close\r\n\r\n"
            "No password provided.";
        // netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
        send(fd, success_response, strlen(success_response), 0);
        return;
    }
    int pass_len = pass_end - pass_start;
    printf("PASS length: %d\r\n", pass_len);
    char pass[pass_len + 1];
    memcpy(pass, pass_start, pass_len);
    pass[pass_len] = '\0';
    printf("Wifi PASS: %s\r\n", pass);
    set_saved_value(WIFI_SSID_KEY, (const char *)&ssid);
    set_saved_value(WIFI_PASS_KEY, (const char *)&pass);
    const char *success_response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n\r\n"
        "Changed wifi settings.";
    // netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
    send(fd, success_response, strlen(success_response), 0);
    trigger_delayed_reboot();
}

void handle_pin_set_state(int fd, const char* path, const char* body) {
    char* start = strstr(body, "name=\"pin\"\r\n\r\n");
    start += 14;
    int channel_index = atoi(start);
    bool setHigh = strstr(path, "/set_pin_high") != NULL;
    printf("path: %s\n", path);
    printf("setHigh: %i\n", setHigh);
    // update_channel_duty(channel_index, setHigh ? 100 : 0);
    set_channel_duty(channel_index, setHigh ? 100.f : 0.f);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Set pin state";
    send(fd, response, strlen(response), 0);
}

void handle_pin_mapping(int fd, const char* body) {
    char colors[5] = {'-', '-', '-', '-', '\0'};
    char* start = strstr(body, "name=\"p0\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[0] = start[0];
    start = strstr(body, "name=\"p1\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[1] = start[0];
    start = strstr(body, "name=\"p2\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[2] = start[0];
    start = strstr(body, "name=\"p3\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[3] = start[0];
    for(int i = 0; i < 4; i++) {
        char color = colors[i];
        if(color == 'r') set_red_channel(i);
        if(color == 'g') set_green_channel(i);
        if(color == 'b') set_blue_channel(i);
        if(color == 'w') set_white_channel(i);
    }
    save_channels();
    printf("Colors: %s\r\n", colors);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Successfully changed pin mapping";
    // netconn_write(conn, response, strlen(response), NETCONN_COPY);
    send(fd, response, strlen(response), 0);
}

void handle_hostname(int fd, const char* body) {
    char* hostname = strstr(body, "name=\"hostname\"\r\n\r\n");
    if(hostname == NULL) {
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n"
            "Can't find hostname";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    }
    hostname += 19;
    char* hostname_end = strstr(hostname, "\r\n------");
    if(hostname_end == NULL) {
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n"
            "Can't find hostname";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    }
    *hostname_end = '\0';
    set_saved_value(HOSTNAME_KEY, hostname);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Successfully changed hostname, device is restarting";
    // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    send(fd, response, strlen(response), 0);
    trigger_delayed_reboot();
}

int get_color_value_from_json(const char* json, char color_key) {
    char search_key[4];
    int val;
    snprintf(search_key, sizeof(search_key), "\"%c\":", color_key);
    char* value_start = strstr(json, search_key);
    if (value_start == NULL) return -1;
    sscanf(value_start + 4, "%d", &val);
    return val;
}

void handle_new_duty(int fd, const char* body) {
    int r, g, b, w;
    r = get_color_value_from_json(body, 'r');
    g = get_color_value_from_json(body, 'g');
    b = get_color_value_from_json(body, 'b');
    w = get_color_value_from_json(body, 'w');
    printf("r = %d, g = %d, b = %d, w = %d\n", r, g, b, w);

    float red, green, blue, white;
    red =  r / 255.f * 100;
    green = g / 255.f * 100;
    blue = b / 255.f * 100;
    white = w / 255.f * 100;
    printf("r = %f, g = %f, b = %f, w = %f\n", red, green, blue, white);

    // update_rgbw_duties(r, g, b, w);
    set_rgbw_duty(red, green, blue, white);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Successfully changed color";
    // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    send(fd, response, strlen(response), 0);
}

void handle_rebooting(int fd, const char* body) {
    puts("[httpd_handler] Rebooting...\n");
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Rebooting...";
    // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    send(fd, response, strlen(response), 0);
    trigger_delayed_reboot();
}

void handle_clean_easyflash(int fd, const char* body) {
    puts("[httpd_handler] Cleaning all easyflash values...\n");
    char* keys = clean_all_saved_values();
    const char *response_template =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Cleaned all values:\n%s\n";
    char response[250];
    snprintf(response, sizeof(response), response_template, keys);
    // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    send(fd, response, strlen(response), 0);
}

bool handle_special_cases(int fd, const char* path, char* buffer, int buffer_length, int content_length) {
    bool handled = false;
    if(strstr(path, "/ota") != NULL) {
        handle_ota_update(fd, content_length, buffer, buffer_length);
        handled = true;
    }
    return handled;
}

// void handle_post_requests(struct netconn* conn, const char* inital_data, int initial_data_length, struct netbuf** inbuf) {
void handle_post_requests(int fd, char* buffer, int current_buffer_length) {
    puts("[httpd_handler] Handling POST request...\n");
    // Get content length
    char *content_length_str = strstr(buffer, "Content-Length:");
    if (!content_length_str) {
        puts("Content-Length header not found\n");
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Content-Length header not found";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    } 
    int content_length = 0;
    sscanf(content_length_str, "Content-Length: %d", &content_length);
    // Got content length
    printf("Content-Length header found: %d\n", content_length);
    // Getting path
    char* path_pos = strstr(buffer, "/");
    char* path_end = strstr(buffer, "\r\n");
    int path_length = path_end - path_pos;
    char path[path_length + 1];
    strncpy(path, path_pos, path_length);
    path[path_length] = '\0';
    // 
    // Get initial body
    // char body_content[BUFFER_SIZE];
    // memset(body_content, 0, BUFFER_SIZE);
    char* body = strstr(buffer, "\r\n\r\n");
    int body_length;
    if(body == NULL) {
        memset(buffer, 0, BUFFER_SIZE);
        puts("[httpd_handler] Need to get inital body\n");
        int datalen = recv(fd, buffer, BUFFER_SIZE, 0);
        printf("[httpd_handler] After receive: %s\n", buffer);
        if(datalen <= 0) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get inital body data";
            // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
            send(fd, response, strlen(response), 0);
            return;
        }
        body = strstr(buffer, "\r\n\r\n");
        if(body == NULL) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get inital body data";
            // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
            send(fd, response, strlen(response), 0);
            return;
        }
        body_length = datalen - (body - buffer);
        memcpy(buffer, body, body_length);
        // printf("[httpd_handler] After memcpy: %s\n", body_content);
        int need_to_clean_size = BUFFER_SIZE - body_length;
        if(need_to_clean_size > 0) {
            memset(buffer+body_length, 0, need_to_clean_size);
            // printf("[httpd_handler] After memset: %s\n", body_content);
        }
        body = buffer;
    } else {
        puts("[httpd_handler] No need for getting inital body\n");
        body_length = current_buffer_length - (body - buffer);
        memcpy(buffer, body, body_length);
        // printf("[httpd_handler] After memcpy: %s\n", buffer);
        int need_to_clean_size = BUFFER_SIZE - body_length;
        if(need_to_clean_size > 0) {
            memset(buffer+body_length, 0, need_to_clean_size);
            // printf("[httpd_handler] After memset: %s\n", buffer);
        }
        body = buffer;
        // body_length = strlen(inital_data) - (body - inital_data);
        // body_length = strlen(body);
    }
    if(body == NULL) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "No body";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    }
    body += 4;
    body_length -= 4;
    if(handle_special_cases(fd, path, body, body_length, content_length)) return;
    // Getting total body
    if(content_length > BUFFER_SIZE) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Body is too large";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    }
    puts("Getting total body\n");
    int current_body_size = strlen(body);
    printf("current_body_size: %d\n", current_body_size);
    while(current_body_size < content_length) {
        puts("Getting next part...\n");
        int new_data_length = recv(fd, buffer+current_body_size, BUFFER_SIZE-current_body_size, 0);
        current_body_size += new_data_length;
        if(new_data_length <= 0) break;
        // char* data;
        // u16_t datalen;
        // if(netbuf_data(*inbuf, (void**)&data, &datalen) != ERR_OK) {
        //     const char *response =
        //         "HTTP/1.1 500 Internal error\r\n"
        //         "Content-Type: text/plain\r\n"
        //         "Connection: close\r\n\r\n"
        //         "Can't get additional data";
        //     netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        //     return;
        // }
        // puts("Copying data to body_content...\n");
        // memcpy(body_content + current_body_size, data, datalen);
        // current_body_size += datalen;
        // puts("Copyed data to body_content\n");
    }
    if(current_body_size < content_length) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Can't get all data";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    }
    // puts("Zeroing body_content\n");
    // memset(body_content, 0, BUFFER_SIZE);
    // puts("Copy inital body to body_content\n");
    // memcpy(body_content, body, current_body_size);
    // while(current_body_size < content_length) {
    //     puts("Deleting prev inbuf...\n");
    //     netbuf_delete(*inbuf);
    //     puts("Getting new inbuf...\n");
    //     if(netconn_recv(conn, inbuf) != ERR_OK) {
    //         const char *response =
    //             "HTTP/1.1 500 Internal error\r\n"
    //             "Content-Type: text/plain\r\n"
    //             "Connection: close\r\n\r\n"
    //             "Can't get receive additional data";
    //         netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    //         return;
    //     }
    //     puts("Getting new inbuf data...\n");
    //     char* data;
    //     u16_t datalen;
    //     do {
    //         if(netbuf_data(*inbuf, (void**)&data, &datalen) != ERR_OK) {
    //             const char *response =
    //                 "HTTP/1.1 500 Internal error\r\n"
    //                 "Content-Type: text/plain\r\n"
    //                 "Connection: close\r\n\r\n"
    //                 "Can't get additional data";
    //             netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    //             return;
    //         } 
    //         puts("Copying data to body_content...\n");
    //         memcpy(body_content + current_body_size, data, datalen);
    //         current_body_size += datalen;
    //         puts("Copyed data to body_content\n");
    //     } while(netbuf_next(*inbuf) != -1);
    // }
    puts("Got total body\n");
    printf("Body: %s\n", body);
    if (strstr(path, "/wifi") != NULL) {
        handle_wifi_settings(fd, body);
    }
    else if (strstr(path, "/set_pin_high") != NULL || strstr(path, "/set_pin_low") != NULL) {
        handle_pin_set_state(fd, path, body);
    }
    else if (strstr(path, "/pin_mapping") != NULL) {
        handle_pin_mapping(fd, body);
    }
    else if (strstr(path, "/new_duty") != NULL) {
        handle_new_duty(fd, body);
    }
    else if (strstr(path, "/reboot") != NULL) {
        handle_rebooting(fd, body);
    } 
    else if (strstr(path, "/clean_easyflash") != NULL) {
        handle_clean_easyflash(fd, body);
    }
    else if (strstr(path, "/hostname") != NULL) {
        handle_hostname(fd, body);
    }
}

// void httpd_handler(struct netconn *conn) {
void httpd_handler(int fd) {
    // struct netbuf *inbuf;
    // err_t error = netconn_recv(conn, &inbuf);
    // if(error != ERR_OK) {
    //     const char *response =
    //         "HTTP/1.1 500 Internal error\r\n"
    //         "Content-Type: text/plain\r\n"
    //         "Connection: close\r\n\r\n"
    //         "Can't handle inital request";
    //     netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    //     return;
    // }
    // char buffer[BUFFER_SIZE];
    char* buffer = pvPortMalloc(BUFFER_SIZE);
    puts("[httpd_handler] Cleaning buffer\n");
    memset(buffer, 0, BUFFER_SIZE);
    puts("[httpd_handler] Receiving data\n");
    // error = netbuf_data(inbuf, (void**)&inital_data, &inital_data_length);
    int received = recv(fd, buffer, BUFFER_SIZE, 0);
    // lwip_recv(int s, void *mem, size_t len, int flags)
    if(received <= 0) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Can't get initial data";
        // netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        send(fd, response, strlen(response), 0);
        return;
    }
    if(strstr(buffer, "GET") != NULL) {
        handle_get_requests(fd, buffer);
    }
    else { // only post is left
        handle_post_requests(fd, buffer, received);
    }
    vPortFree(buffer);
    // free(buffer);
    // netbuf_delete(inbuf); // Closing last netbuf i got
}

void request_handle_thread(void* arg) {
    // struct netconn* conn = (struct netconn*) arg;
    int fd = (int)arg;
    vTaskDelay(RTOS_DELAY_MS);
    puts("[http_request_handler] Handling request\n");
    httpd_handler(fd);
    puts("[http_request_handler] Request handled\n");
    puts("[http_request_handler] Close newconn\n");
    close(fd);
    // puts("[http_request_handler] Delete newconn\n");
    // netconn_delete(conn);
    puts("[http_request_handler] Connection handled and closed\n");
    vTaskDelete(NULL);
}

// Start the HTTP server
void http_server(void *pvParameters) {
    vTaskDelay(2000);
    init_pwm();
	struct sockaddr_in server_addr, client_addr;
	socklen_t sockaddr_t_size = sizeof(client_addr);
	int tcp_listen_fd = -1, client_fd = -1, err = -1;
	fd_set readfds;
	tcp_listen_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(80);
	err = bind(tcp_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err != 0) puts("[http_server] Can't bind\n");
    if(err == 0) err = listen(tcp_listen_fd, 0);
    if(err != 0) puts("[http_server] Can't listen\n");
    if(err == 0) {
        puts("[http_server] Listening on port 80......\r\n");
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(tcp_listen_fd, &readfds);
            select(tcp_listen_fd + 1, &readfds, NULL, NULL, NULL);
            if (FD_ISSET(tcp_listen_fd, &readfds)){
                client_fd = accept(tcp_listen_fd, (struct sockaddr*)&client_addr, &sockaddr_t_size);
                if (client_fd >= 0) {
                    vTaskDelay(RTOS_DELAY_MS);
                    if(sys_thread_new("request_handler", request_handle_thread, (void*)client_fd, REQUEST_HANDLER_STACK_SIZE, MY_TASK_PRIO) == NULL) {
                        puts("[http_server] Can't create new request_handler\n");
                        lwip_close(client_fd);
                        client_fd = -1;
                    }
                }
            }

            // //////////////
            // if(uxQueueMessagesWaiting(conn->acceptmbox) > 0) {
            //     vTaskDelay(RTOS_DELAY_MS);
            //     printf("uxMessagesWaiting: %d\n", uxQueueMessagesWaiting(conn->acceptmbox));
            //     puts("[http_server] New tcp connection in queue\n");
            //     if (netconn_accept(conn, &newconn) == ERR_OK) {
            //         puts("[http_server] New tcp connection established\n");
            //         puts("[http_server] Create request handler\n");
            //         if(sys_thread_new("request_handler", request_handle_thread, newconn, REQUEST_HANDLER_STACK_SIZE, REQUEST_HANDLER_PRIO) != NULL) {
            //             puts("[http_server] Request handler created\n");
            //         } else puts("[http_server] Request handler not created\n");
            //     }
            //     puts("[http_server] Tcp connection in queue handled\n");
            // }
            // //////////////
            vTaskDelay(RTOS_DELAY_MS);
        }
    }
    puts("[http_server] Shutdown server\n");
    lwip_close(tcp_listen_fd);
    vTaskDelete(NULL);
}