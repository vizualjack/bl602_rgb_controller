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
    const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Connection: close\r\n\r\n";
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
    netconn_write(conn, html_page, strlen(html_page), NETCONN_COPY);
}

// void handle_post_ota(struct netconn *conn, const char* buffer, int buffer_length) {
//     puts("[httpd_handler] Handling firmware upload...\r\n");
//     int content_length;
// 	int ret;
// 	bl_mtd_handle_t handle;
//     char *content_length_str = strstr(buffer, "Content-Length:");
//     if (content_length_str) {
//         sscanf(content_length_str, "Content-Length: %d", &content_length);
//         printf("[OTA] Content-Length header found: %d\n", content_length);
//     } else {
//         puts("Content-Length header not found\n");
//         const char *error_response =
//             "HTTP/1.1 500 Internal Server Error\r\n"
//             "Connection: close\r\n\r\n"
//             "Can't content-length header. Please try again!";
//         netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
//         return;
//     }
// 	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &handle, BL_MTD_OPEN_FLAG_BACKUP);
// 	if(ret) {
//         puts("Open Default FW partition failed\r\n");
//         const char *error_response =
//             "HTTP/1.1 500 Internal Server Error\r\n"
//             "Connection: close\r\n\r\n"
//             "Can't open partition. Please try again!";
//         netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
//         return;
// 	}
// 	unsigned int flash_offset, ota_addr;
// 	uint32_t bin_size;
// 	uint8_t activeID;
// 	HALPartition_Entry_Config ptEntry;
// 	activeID = hal_boot2_get_active_partition();
// 	if(hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &ptEntry))
// 	{
// 		bl_mtd_close(handle);
//         puts("PtTable_Get_Active_Entries fail\r\n");
//         const char *error_response =
//             "HTTP/1.1 500 Internal Server Error\r\n"
//             "Connection: close\r\n\r\n"
//             "Can't get active entries. Please try again!";
//         netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
//         return;
// 	}
// 	ota_addr = ptEntry.Address[!ptEntry.activeIndex];
// 	bin_size = ptEntry.maxLen[!ptEntry.activeIndex];
//     if(content_length >= bin_size)
//     {
//         puts("BIN file is too big");
//         const char *error_response =
//             "HTTP/1.1 500 Internal Server Error\r\n"
//             "Connection: close\r\n\r\n"
//             "File is too big";
//         netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
//         return;
//     }
// 	printf("[OTA] Erasing flash (%lu)...\r\n", bin_size);
// 	hal_update_mfg_ptable();
// 	uint32_t erase_offset = 0;
// 	uint32_t erase_len = 0;
// 	while(erase_offset < bin_size)
// 	{
// 		erase_len = bin_size - erase_offset;
// 		if(erase_len > 0x10000)
// 		{
// 			erase_len = 0x10000; //Erase in 64kb chunks
// 		}
// 		bl_mtd_erase(handle, erase_offset, erase_len);
// 		erase_offset += erase_len;
//         sys_msleep(500);
// 	}
// 	printf("[OTA] Flash erased\r\n");
// 	printf("[OTA] Flashing new firmware\r\n");
//     struct netbuf *inbuf;
//     char *buf;
//     u16_t buflen;
//     int remaining = content_length;
//     // char *data_start;
// 	int total = 0;
//     char *data_end;
//     bool first = true;
//     flash_offset = 0;

//     char* start = strstr(buffer, "\r\n\r\n");
//     start += 4;
//     if (start) {
//         int skipped_bytes = start - buffer;
//         buflen = buffer_length - skipped_bytes;
//         printf("[OTA] Found data start in initial buffer, skipping %td bytes\r\n", skipped_bytes);
//         bl_mtd_write(handle, flash_offset, buflen, (u_int8_t*)start);
//         flash_offset += buflen;
//         total += buflen;
//         remaining -= buffer_length;
//         first = false;
//     }
//     int max_packet_size = 0;
//     while (remaining > 0) {
//         if (netconn_recv(conn, &inbuf) == ERR_OK) {
//             netbuf_data(inbuf, (void **)&buf, &buflen);
//             remaining -= buflen;
//             // if(max_packet_size == 0) max_packet_size = buflen;
//             if (first) {
//                 char* start = strstr(buf, "\r\n\r\n");
//                 int skipped_bytes = 0;
//                 if(start)  {
//                     start += 4; // Skip "\r\n\r\n"
//                     skipped_bytes = start - buf;
//                     buf = start;
//                     buflen -= skipped_bytes;
//                     printf("[OTA] Found data start, skipping %td bytes\r\n", skipped_bytes);
//                     first = false;
//                 } else {
//                     puts("Something is totally wrong\r\n");
//                     printf("Last body: %s\r\n", buf);
//                     bl_mtd_close(handle);
//                     const char *error_response =
//                         "HTTP/1.1 500 Internal Server Error\r\n"
//                         "Connection: close\r\n\r\n"
//                         "Can't find start of request. Please try again!";
//                     netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
//                     netbuf_delete(inbuf);
//                     return;
//                 }
//             }
//             data_end = strstr(buf, "\r\n------");
//             if(data_end != NULL) {
//                 buflen = (data_end - buf);
//                 printf("[OTA] Found data end, last frame total bytes are %td\r\n", data_end - buf);
//             }
//             bl_mtd_write(handle, flash_offset, buflen, (u_int8_t*)buf);
//             flash_offset += buflen;
//             total += buflen;
//             netbuf_delete(inbuf);
//             printf("remaining: %d\r\n", remaining);
//             // if(max_packet_size != 0 && buflen < max_packet_size) break;
//         } else {
//             puts("Error receiving additional data\n");
// 	        bl_mtd_close(handle);
//             const char *error_response =
//                 "HTTP/1.1 500 Internal Server Error\r\n"
//                 "Connection: close\r\n\r\n"
//                 "Error while receiving request. Please try again!";
//             netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
//             return;
//         }
//     }
// 	printf("[OTA] Flashed new firmware\r\n");
// 	printf("[OTA] set OTA partition length\r\n");
// 	ptEntry.len = total;
// 	// ptEntry.len = content_length;
// 	hal_boot2_update_ptable(&ptEntry);
// 	puts("[OTA] Flashed\r\n");
// 	bl_mtd_close(handle);
//     const char *success_response =
//         "HTTP/1.1 200 OK\r\n"
//         "Connection: close\r\n\r\n"
//         "Firmware uploaded successfully.";
//     puts("[OTA] Sending response...\r\n");
//     netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
//     trigger_delayed_reboot();
//     // sys_msleep(1000);
//     // puts("[OTA] Rebooting...\r\n");
//     // sys_msleep(500);
//     // hal_reboot();
//     // // before stuff
//     // if (http_rest_post_flash(conn, content_length) != ERR_OK) {
// }

void handle_ota_update(struct netconn* conn, int content_length, const char* initial_body, struct netbuf** inbuf)
{
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
            // recv_err = ERR_OK;
            puts("[OTA] Set initial buffer to handling initial_body\n");
            buf = (char*) initial_body;
            buflen = strlen(initial_body);
            printf("[OTA] initial_body length: %d\n", buflen);
            initial_buffer_handled = true;
            char* file_start = strstr(buf, "\r\n\r\n");
            if(file_start == NULL) {
                puts("[OTA] inital_body has no file start, skip this\n");
                continue;
            }
        } else {
            puts("[OTA] Deleting current inbuf\n");
            netbuf_delete(*inbuf);
            puts("[OTA] Receiving next data\n");
            err_t recv_err = netconn_recv(conn, inbuf);
            if(recv_err == ERR_OK) recv_err = netbuf_data(*inbuf, (void **)&buf, &buflen);
            if(recv_err != ERR_OK) buf = NULL;
        }
        if (buf != NULL) {
            puts("=== Data part ===\n");
            printf("%s\n", buf);
            puts("=================\n");
            raw_total += buflen;
            // last_buffer_size = buflen;
            printf("raw_total: %d\n", raw_total);
            printf("content_length: %d\n", content_length);
            remaining -= buflen;
            printf("Remaining: %d\n", remaining);
            // data transmission
            if (first) {
                char *before = buf;
                buf = strstr(buf, "\r\n\r\n");
                buf += 4; // Skip past "\r\n\r\n"
                first = false;
                int skipped_bytes = buf - before;
                if(skipped_bytes < 0) {
                    puts("Something is totally wrong\r\n");
                    bl_mtd_close(handle);
                    const char *error_response =
                        "HTTP/1.1 500 Internal Server Error\r\n"
                        "Connection: close\r\n\r\n"
                        "Can't find start of request. Please try again!";
                    netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
                    // netbuf_delete(inbuf);
                    // if(initial_buffer_handled) netbuf_delete(inbuf);
                    return;
                }
                buflen -= skipped_bytes;
                printf("[OTA] Found data start, skipping %td bytes\r\n", skipped_bytes);
            }
            data_end = strstr(buf, "\r\n------");
            if(data_end != NULL) {
                buflen = (data_end - buf);
                printf("[OTA] Found data end, last frame total bytes are %td\r\n", buflen);
            }
            bl_mtd_write(handle, flash_offset, buflen, (u_int8_t*)buf);
            flash_offset += buflen;
            total += buflen;
            // if(initial_buffer_handled) {
            //     puts("[OTA] deleting buffer\n");
            //     netbuf_delete(inbuf);
            // }
            // else initial_buffer_handled = true;
        } else {
            puts("Error receiving additional data\n");
	        bl_mtd_close(handle);
            const char *error_response =
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Connection: close\r\n\r\n"
                "Error while receiving request. Please try again!";
            netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
            return;
        }
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
    // netbuf_delete(inbuf);
    // free(body);
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
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
}

void handle_rebooting(struct netconn *conn, const char* body) {
    puts("[httpd_handler] Rebooting...\n");
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Rebooting...";
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
    trigger_delayed_reboot();
}

bool handle_special_cases(struct netconn* conn, const char* inital_data, const char* inital_body, struct netbuf** inbuf, int content_length) {
    bool handled = false;
    if(strstr(inital_data, "POST /ota") != NULL) {
        handle_ota_update(conn, content_length, inital_body, inbuf);
        handled = true;
    }
    return handled;
}

#define MAX_BODY_SIZE 1024
void handle_post_requests(struct netconn* conn, const char* inital_data, struct netbuf** inbuf) {
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
        netconn_write(conn, response, strlen(response), NETCONN_COPY);
        return;
    } 
    int content_length = 0;
    sscanf(content_length_str, "Content-Length: %d", &content_length);
    // Got content length
    printf("Content-Length header found: %d\n", content_length);
    // Get initial body
    char* body = strstr(inital_data, "\r\n\r\n");
    if(!body) {
        netbuf_delete(*inbuf);
        if(netconn_recv(conn, inbuf) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Initial body request failed";
            netconn_write(conn, response, strlen(response), NETCONN_COPY);
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
            netconn_write(conn, response, strlen(response), NETCONN_COPY);
            return;
        }
        body = strstr(data, "\r\n\r\n");
    }
    if(!body) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "No body";
        netconn_write(conn, response, strlen(response), NETCONN_COPY);
        return;
    }
    body += 4;
    if(handle_special_cases(conn, inital_data, body, inbuf, content_length)) return;
    // Getting total body
    if(content_length > MAX_BODY_SIZE) {
        const char *response =
            "HTTP/1.1 500 Internal error\r\n"
            "Content-Type: text/plain\r\n"
            "Connection: close\r\n\r\n"
            "Body is too large";
        netconn_write(conn, response, strlen(response), NETCONN_COPY);
        return;
    }
    char body_content[MAX_BODY_SIZE];
    int current_body_size = strlen(body);
    memset(body_content, 0, MAX_BODY_SIZE);
    memcpy(body_content, body, current_body_size);
    while(current_body_size < content_length) {
        netbuf_delete(*inbuf);
        if(netconn_recv(conn, inbuf) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get receive additional data";
            netconn_write(conn, response, strlen(response), NETCONN_COPY);
            return;
        }
        char* data;
        u16_t datalen;
        if(netbuf_data(*inbuf, (void**)&data, &datalen) != ERR_OK) {
            const char *response =
                "HTTP/1.1 500 Internal error\r\n"
                "Content-Type: text/plain\r\n"
                "Connection: close\r\n\r\n"
                "Can't get additional data";
            netconn_write(conn, response, strlen(response), NETCONN_COPY);
            return;
        }
        memcpy(body_content + current_body_size, data, datalen);
        current_body_size += datalen;
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
        netconn_write(conn, response, strlen(response), NETCONN_COPY);
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
        netconn_write(conn, response, strlen(response), NETCONN_COPY);
        return;
    }
    if(strstr(inital_data, "GET") != NULL) {
        handle_get_requests(conn, inital_data);
    }
    else { // only post is left
        handle_post_requests(conn, inital_data, &inbuf);
    }
    netbuf_delete(inbuf); // Closing last netbuf i got
}

// Start the HTTP server
void http_server(void *pvParameters) {
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
            puts("[http_server] New tcp connection established\n");
            puts("[http_server] Handle request\n");
            httpd_handler(newconn);
            puts("[http_server] Close newconn\n");
            netconn_close(newconn);
            puts("[http_server] Delete newconn\n");
            netconn_delete(newconn);
        }
    }
}