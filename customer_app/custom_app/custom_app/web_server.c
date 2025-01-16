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

void handle_get_requests(struct netconn *conn, const char* buffer) {
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

void handle_post_ota(struct netconn *conn, const char* buffer, int buffer_length)
{
    
    int content_length;
    char *content_length_str = strstr(buffer, "Content-Length:");
    if (content_length_str) {
        sscanf(content_length_str, "Content-Length: %d", &content_length);
        printf("[OTA] Content-Length header found: %d\n", content_length);
    } else {
        puts("Content-Length header not found\n");
        const char *error_response =
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Connection: close\r\n\r\n"
            "Can't content-length header. Please try again!";
        netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
        return;
    }

	int ret;
	bl_mtd_handle_t handle;

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
	uint8_t activeID;
	HALPartition_Entry_Config ptEntry;

	activeID = hal_boot2_get_active_partition();


	if(hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &ptEntry))
	{
		// vPortFree(recv_buffer);
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
		if(erase_len > 0x10000)
		{
			erase_len = 0x10000; //Erase in 64kb chunks
		}
		bl_mtd_erase(handle, erase_offset, erase_len);
		erase_offset += erase_len;
        sys_msleep(200);
	}
	printf("[OTA] Flash erased\r\n");
	printf("[OTA] Flashing new firmware\r\n");
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    int remaining = content_length;
	int total = 0;
    char *data_end;
    bool first = true;
    flash_offset = 0;
    while (remaining > 0) {
        err_t recv_err = netconn_recv(conn, &inbuf);
        if (recv_err == ERR_OK) {
            netbuf_data(inbuf, (void **)&buf, &buflen);
            remaining -= buflen;
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
                    netbuf_delete(inbuf);
                    return;
                }
                buflen -= skipped_bytes;
                printf("[OTA] Found data start, skipping %td bytes\r\n", skipped_bytes);
            }
            data_end = strstr(buf, "\r\n------");
            if(data_end != NULL) {
                buflen = (data_end - buf);
                printf("[OTA] Found data end, last frame total bytes are %td\r\n", data_end - buf);
            }
            bl_mtd_write(handle, flash_offset, buflen, (u_int8_t*)buf);
            flash_offset += buflen;
            total += buflen;
            netbuf_delete(inbuf);
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
    const char *success_response =
        "HTTP/1.1 200 OK\r\n"
        "Connection: close\r\n\r\n"
        "Firmware uploaded successfully.";
    puts("[OTA] Sending response...\r\n");
    netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
    trigger_delayed_reboot();
}

void handle_wifi_settings(struct netconn *conn, const char* buffer) {
    puts("[httpd_handler] Handling wifi settings request...\n");
    // SSID
    char* ssid_start = strstr(buffer, "name=\"ssid\"\r\n\r\n");
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

void handle_pin_set_state(struct netconn *conn, const char* buffer) {
    // char* body = malloc(content_length + 1);
    // char* body_start = strstr(buf, "\r\n\r\n");
    // body_start += 4;
    // char* body_end = buf + buflen;
    // int total_length = body_end - body_start;
    // printf("Total length: %i\r\n", total_length);
    // memcpy(body, body_start, total_length);
    // body[content_length] = '\0';
    char* start = strstr(buffer, "name=\"pin\"\r\n\r\n");
    start += 14;
    int channel_index = atoi(start);
    bool setHigh = strncmp(buffer, "POST /set_pin_high", 18) == 0;
    set_channel_duty(channel_index, setHigh ? 100 : 0);
}

void handle_pin_mapping(struct netconn *conn, const char* buffer) {
    // char* body = malloc(content_length + 1);
    // char* body_start = strstr(buf, "\r\n\r\n");
    // body_start += 4;
    // char* body_end = buf + buflen;
    // int total_length = body_end - body_start;
    // printf("Total length: %i\r\n", total_length);
    // memcpy(body, body_start, total_length);
    // netbuf_delete(inbuf);
    // if (netconn_recv(conn, &inbuf) == ERR_OK) {
    //     puts("Got additional data\r\n");
    //     netbuf_data(inbuf, (void **)&buf, &buflen);
    //     printf("Additional data length: %i\r\n", buflen);
    //     memcpy(body + total_length, buf, buflen);
    // }
    // body[content_length] = '\0';
    char colors[5] = {'-', '-', '-', '-', '\0'};
    char* start = strstr(buffer, "name=\"p0\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[0] = start[0];
    start = strstr(buffer, "name=\"p1\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[1] = start[0];
    start = strstr(buffer, "name=\"p2\"\r\n\r\n");
    start += 13;
    if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[2] = start[0];
    start = strstr(buffer, "name=\"p3\"\r\n\r\n");
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

void handle_new_duty(struct netconn *conn, const char* buffer) {
    // char* body = malloc(content_length + 1);
    // char* body_start = strstr(buf, "\r\n\r\n");
    // body_start += 4;
    // char* body_end = buf + buflen;
    // int total_length = body_end - body_start;
    // printf("Total length: %i\r\n", total_length);
    // if(total_length > 0) memcpy(body, body_start, total_length);
    // if (total_length == 0) {
    //     struct netbuf *inbuf;
    //     if (netconn_recv(conn, &inbuf) == ERR_OK) {
    //         puts("Got additional data\r\n");
    //         netbuf_data(inbuf, (void **)&buf, &buflen);
    //         printf("Additional data length: %i\r\n", buflen);
    //         memcpy(body + total_length, buf, buflen);
    //         netbuf_delete(inbuf);
    //     }
    // }
    // body[content_length] = '\0';
    // printf("Body: %s\r\n", body);
    int r, g, b, w;
    r = get_color_value_from_json(buffer, 'r');
    g = get_color_value_from_json(buffer, 'g');
    b = get_color_value_from_json(buffer, 'b');
    w = get_color_value_from_json(buffer, 'w');
    // printf("r = %d, g = %d, b = %d, w = %d\n", r, g, b, w);
    set_rgbw_duty(r, g, b, w);
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Successfully changed color";
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
    // free(body);
}

void handle_rebooting(struct netconn *conn, const char* buffer) {
    puts("[httpd_handler] Rebooting...\n");
    const char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n\r\n"
        "Rebooting...";
    netconn_write(conn, response, strlen(response), NETCONN_COPY);
    trigger_delayed_reboot();
}

void handle_post_requests(struct netconn *conn, const char* buffer) {
    puts("[httpd_handler] Handling POST request...\n");
    if (strncmp(buffer, "POST /wifi", 10) == 0) {
        handle_wifi_settings(conn, buffer);
    }
    else if (strncmp(buffer, "POST /set_pin_high", 18) == 0 || strncmp(buffer, "POST /set_pin_low", 17) == 0) {
        handle_pin_set_state(conn, buffer);
    }
    else if (strncmp(buffer, "POST /pin_mapping", 17) == 0) {
        handle_pin_mapping(conn, buffer);
    }
    else if (strncmp(buffer, "POST /new_duty", 14) == 0) {
        handle_new_duty(conn, buffer);
    }
    else if (strncmp(buffer, "POST /reboot", 12) == 0) {
        handle_rebooting(conn, buffer);
    } 
}

#define MAX_BUFFER_SIZE 1024
err_t httpd_handler(struct netconn *conn) {
    static char buffer[MAX_BUFFER_SIZE];
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    int total_length = 0;
    int content_length = 0;
    bool already_handled = false;
    err_t error = ERR_OK;
    // Clean the buffer
    memset(buffer, 0, sizeof(buffer));
    // Receive whole HTTP request
    while (netconn_recv(conn, &inbuf) == ERR_OK) {
        netbuf_data(inbuf, (void **)&buf, &buflen);
        memcpy(buffer + total_length, buf, buflen);
        total_length += buflen;
        // special case (ota cause of bigger data)
        if(strncmp(buffer, "POST /ota", 9) == 0) {
            handle_post_ota(conn, buffer, buflen);
            already_handled = true;
            netbuf_delete(inbuf);
            break;
        }
        if(content_length == 0) {
            char *content_length_str = strstr(buffer, "Content-Length:");
            if (content_length_str) {
                sscanf(content_length_str, "Content-Length: %d", &content_length);
                printf("Content-Length header found: %d\n", content_length);
                // printf("buffer: %s\n", buffer);
                // printf("buflen: %d\n", buflen);
                char* body_start = strstr(buffer, "\r\n\r\n");
                if(body_start) {
                    // puts("Body start found\r\n");
                    body_start += 4;
                    int skipped_bytes = body_start - (char*)&buffer;
                    int body_len = buflen - skipped_bytes;
                    if(body_len == content_length) break;
                }
            } else {
                puts("Content-Length header not found\n");
                netbuf_delete(inbuf);
                break;
            }
        }
        if(content_length > MAX_BUFFER_SIZE) {
            const char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n\r\n";
            netconn_write(conn, response, strlen(response), NETCONN_COPY);
            netbuf_delete(inbuf);
            error = ERR_ARG;
            break;
        }
        netbuf_delete(inbuf);
    }
    // buffer[total_length] = '\0'; / should be useless because of memset
    if (!already_handled && error == ERR_OK) {
        // Check if the request is a POST (for file upload)
        if (strncmp(buf, "POST ", 5) == 0) {
            handle_post_requests(conn, buffer);
        } else {
            handle_get_requests(conn, buffer);
        }
    }
    netbuf_delete(inbuf);
    return error;
}

// Start the HTTP server
void http_server(void *pvParameters) {
    struct netconn *conn, *newconn;
    puts("[http_server] Starting...\r\n");
    // Create a new TCP connection
    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, IP_ADDR_ANY, 80);
    netconn_listen(conn);

    puts("[http_server] Listening on port 80......\r\n");
    init_pwm();
    while (1) {
        // Accept new connections
        if (netconn_accept(conn, &newconn) == ERR_OK) {
            puts("[http_server] New tcp connection established\n");
            httpd_handler(newconn);
            netconn_close(newconn);
            netconn_delete(newconn);
            puts("[http_server] Tcp connection closed successfully\n");
        }
        else {
            puts("[http_server] Couldn't establish tcp connection\n");
        }
    }
}