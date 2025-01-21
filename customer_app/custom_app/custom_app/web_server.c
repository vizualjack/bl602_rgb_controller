#include <FreeRTOS.h>
#include <stdbool.h>
#include <bl_mtd.h>
#include <hal_boot2.h>
#include <lwip/api.h>
#include <lwip/err.h>
#include <string.h>
#include <hal_sys.h>

#include "connection.c"
#include "page.h"
#include "pwm.h"
#include "persistence.h"

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

void handle_get_requests(struct netconn *conn, const char* initial_data) {
    if(strstr(initial_data, "GET /script.js") != NULL) {
        const char *response_header_template =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/javascript\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n";
        int content_length = strlen(js_script); 
        char response_header[100]; 
        snprintf(response_header, sizeof(response_header), response_header_template, content_length);
        netconn_write(conn, response_header, strlen(response_header), NETCONN_NOCOPY);
        netconn_write(conn, js_script, content_length, NETCONN_NOCOPY);
    } else {
        const char *response_header_template =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n\r\n";
        int content_length = strlen(html_page); 
        char response_header[100]; 
        snprintf(response_header, sizeof(response_header), response_header_template, content_length);
        netconn_write(conn, response_header, strlen(response_header), NETCONN_NOCOPY);
        netconn_write(conn, html_page, content_length, NETCONN_NOCOPY);
    }
    
}

void handle_ota_update(struct netconn* conn, int content_length, const char* initial_body, int initial_body_length, struct netbuf** inbuf) {
	bl_mtd_handle_t handle;
	int ret;
	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &handle, BL_MTD_OPEN_FLAG_BACKUP);
	if(ret)
	{
        puts("Open Default FW partition failed\r\n");
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Connection: close\r\n\r\n"
            "Can't open partition. Please try again!";
        netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
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
        netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
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
        netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
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
    char* buf;
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
    while (remaining > 0) {
        if(!initial_buffer_handled) {
            puts("[OTA] Set buf to inital_body\n");
        } else {
            puts("[OTA] Deleting current inbuf\n");
            netbuf_delete(*inbuf);
            puts("[OTA] Receiving next data\n");
            if(netconn_recv(conn, inbuf) != ERR_OK) {
	            bl_mtd_close(handle);
                puts("[OTA] Can't receive next inbuf\n");
                const char *error_response =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Connection: close\r\n\r\n"
                    "Error while receiving request. Please try again!";
                netconn_write(conn, error_response, strlen(error_response), NETCONN_NOCOPY);
                return;
            }
        }
        do {
            if(!initial_buffer_handled) {
                buf = (char*)initial_body;
                buflen = initial_body_length;
                initial_buffer_handled = true;
            } else if(netbuf_data(*inbuf, (void **)&buf, &buflen) != ERR_OK) {
                bl_mtd_close(handle);
                const char *error_response =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Connection: close\r\n\r\n"
                    "Error while receiving request. Please try again!";
                netconn_write(conn, error_response, strlen(error_response), NETCONN_NOCOPY);
                return;
            }
            raw_total += buflen;
            remaining -= buflen;
            printf("Remaining: %d\r\n", remaining);
            bl_mtd_write(handle, flash_offset, buflen, (u_int8_t*)buf);
            flash_offset += buflen;
            total += buflen;
        } while(netbuf_next(*inbuf) != -1);
        netbuf_first(*inbuf);
    }
	printf("[OTA] Flashed new firmware\r\n");
	printf("[OTA] set OTA partition length\r\n");
	ptEntry.len = total;
	hal_boot2_update_ptable(&ptEntry);
	puts("[OTA] Flashed\r\n");
	bl_mtd_close(handle);
    puts("[OTA] Sending response...\r\n");
    const char *success_response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n\r\n"
        "Firmware uploaded successfully.";
    netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
    // if(inbuf == NULL) netbuf_delete(inbuf);
    trigger_delayed_reboot();
}

void handle_wifi_settings(struct netconn *conn, const char* body) {
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
        netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
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
        netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
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
        netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
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
        netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
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
    netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
    trigger_delayed_reboot();
}

void handle_pin_set_state(struct netconn *conn, const char* initial_data, const char* body) {
    char* start = strstr(body, "name=\"pin\"\r\n\r\n");
    start += 14;
    int channel_index = atoi(start);
    bool setHigh = strncmp(initial_data, "POST /set_pin_high", 18) == 0;
    set_channel_duty(channel_index, setHigh ? 100 : 0);
}

void handle_pin_mapping(struct netconn *conn, const char* body) {
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
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
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

void handle_new_duty(struct netconn *conn, const char* body) {
    int r, g, b, w;
    r = get_color_value_from_json(body, 'r');
    g = get_color_value_from_json(body, 'g');
    b = get_color_value_from_json(body, 'b');
    w = get_color_value_from_json(body, 'w');
    printf("r = %d, g = %d, b = %d, w = %d\n", r, g, b, w);
    set_rgbw_duty(r, g, b, w);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Successfully changed color";
    netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
}

void handle_rebooting(struct netconn *conn, const char* body) {
    puts("[httpd_handler] Rebooting...\n");
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Rebooting...";
    netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
    trigger_delayed_reboot();
}

void handle_clean_easyflash(struct netconn *conn, const char* body) {
    puts("[httpd_handler] Cleaning all easyflash values...\n");
    char* keys = clean_all_saved_values();
    const char *response_template =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Cleaned all values:\n%s\n";
    char response[250];
    snprintf(response, sizeof(response), response_template, keys);
    netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
}

bool handle_special_cases(struct netconn* conn, const char* inital_data, const char* inital_body, int inital_body_length, struct netbuf** inbuf, int content_length) {
    bool handled = false;
    if(strstr(inital_data, "POST /ota") != NULL) {
        handle_ota_update(conn, content_length, inital_body, inital_body_length, inbuf);
        handled = true;
    }
    return handled;
}

#define MAX_BODY_SIZE 1024
void handle_post_requests(struct netconn* conn, const char* inital_data, int initial_data_length, struct netbuf** inbuf) {
    puts("[httpd_handler] Handling POST request...\n");
    // Get content length
    char *content_length_str = strstr(inital_data, "Content-Length:");
    if (!content_length_str) {
        puts("Content-Length header not found\n");
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Content-Length header not found";
        netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        return;
    } 
    int content_length = 0;
    sscanf(content_length_str, "Content-Length: %d", &content_length);
    // Got content length
    printf("Content-Length header found: %d\n", content_length);
    // Get initial body
    char* body = strstr(inital_data, "\r\n\r\n");
    int body_length;
    if(body == NULL) {
        netbuf_delete(*inbuf);
        if(netconn_recv(conn, inbuf) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Initial body request failed";
            netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
            return;
        }
        char* data;
        u16_t datalen;
        if(netbuf_data(*inbuf, (void**)&data, &datalen) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get inital body data";
            netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
            return;
        }
        body = strstr(data, "\r\n\r\n");
        body_length = datalen - (body - data);
    } else {
        body_length = initial_data_length - (body - inital_data);
    }
    if(body == NULL) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "No body";
        netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        return;
    }
    body += 4;
    body_length -= 4;
    if(handle_special_cases(conn, inital_data, body, body_length, inbuf, content_length)) return;
    // Getting total body
    if(content_length > MAX_BODY_SIZE) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Body is too large";
        netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        return;
    }
    puts("Getting total body\n");
    char body_content[MAX_BODY_SIZE];
    int current_body_size = strlen(body);
    printf("current_body_size: %d\n", current_body_size);
    while(netbuf_next(*inbuf) != -1) {
        puts("Getting next inbuf data part...\n");
        char* data;
        u16_t datalen;
        if(netbuf_data(*inbuf, (void**)&data, &datalen) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get additional data";
            netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
            return;
        }
        puts("Copying data to body_content...\n");
        memcpy(body_content + current_body_size, data, datalen);
        current_body_size += datalen;
        puts("Copyed data to body_content\n");
    }
    puts("Zeroing body_content\n");
    memset(body_content, 0, MAX_BODY_SIZE);
    puts("Copy inital body to body_content\n");
    memcpy(body_content, body, current_body_size);
    while(current_body_size < content_length) {
        puts("Deleting prev inbuf...\n");
        netbuf_delete(*inbuf);
        puts("Getting new inbuf...\n");
        if(netconn_recv(conn, inbuf) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get receive additional data";
            netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
            return;
        }
        puts("Getting new inbuf data...\n");
        char* data;
        u16_t datalen;
        do {
            if(netbuf_data(*inbuf, (void**)&data, &datalen) != ERR_OK) {
                const char *response =
                    "HTTP/1.1 500 Internal error\r\n"
                    "Content-Type: text/plain\r\n"
                    "Connection: close\r\n\r\n"
                    "Can't get additional data";
                netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
                return;
            } 
            puts("Copying data to body_content...\n");
            memcpy(body_content + current_body_size, data, datalen);
            current_body_size += datalen;
            puts("Copyed data to body_content\n");
        } while(netbuf_next(*inbuf) != -1);
    }
    puts("Got total body\n");;
    printf("Body: %s\n", body_content);
    if (strstr(inital_data, "POST /wifi") != NULL) {
        handle_wifi_settings(conn, body_content);
    }
    else if (strstr(inital_data, "POST /set_pin_high") != NULL || strstr(inital_data, "POST /set_pin_low") != NULL) {
        handle_pin_set_state(conn, inital_data, body_content);
    }
    else if (strstr(inital_data, "POST /pin_mapping") != NULL) {
        handle_pin_mapping(conn, body_content);
    }
    else if (strstr(inital_data, "POST /new_duty") != NULL) {
        handle_new_duty(conn, body_content);
    }
    else if (strstr(inital_data, "POST /reboot") != NULL) {
        handle_rebooting(conn, body_content);
    } 
    else if (strstr(inital_data, "POST /clean_easyflash") != NULL) {
        handle_clean_easyflash(conn, body_content);
    } 
}
void httpd_handler(struct netconn *conn) {
    struct netbuf *inbuf;
    err_t error = netconn_recv(conn, &inbuf);
    if(error != ERR_OK) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Can't handle inital request";
        netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        return;
    }
    char* inital_data;
    u16_t inital_data_length;
    error = netbuf_data(inbuf, (void**)&inital_data, &inital_data_length);
    if(error != ERR_OK) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Can't get initial data";
        netconn_write(conn, response, strlen(response), NETCONN_NOCOPY);
        return;
    }
    if(strstr(inital_data, "GET") != NULL) {
        handle_get_requests(conn, inital_data);
    }
    else { // only post is left
        handle_post_requests(conn, inital_data, inital_data_length, &inbuf);
    }
    netbuf_delete(inbuf); // Closing last netbuf i got
}

// Start the HTTP server
void http_server(void *pvParameters) {
    vTaskDelay(2000);
    struct netconn *conn, *newconn;
    puts("[http_server] Starting...\r\n");
    // Create a new TCP connection
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, 80);
    netconn_listen(conn);
    init_pwm();
    puts("[http_pserver] Listening on port 80......\r\n");
    while (1) {
        // Accept new connections
        if (netconn_accept(conn, &newconn) == ERR_OK) {
            printf("Free heap size: %d\r\n", xPortGetFreeHeapSize());
            puts("[http_server] New tcp connection established\n");
            puts("[http_server] Handle request\n");
            httpd_handler(newconn);
            puts("[http_server] Close newconn\n");
            netconn_close(newconn);
            puts("[http_server] Delete newconn\n");
            netconn_delete(newconn);
            puts("[http_server] Tcp connection closed and deleted\n");
        }
    }
}