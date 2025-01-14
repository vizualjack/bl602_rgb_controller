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


#define OTA_PROGRAM_SIZE (512)

static int http_rest_post_flash(struct netconn *conn, int content_length)
{
	// int towrite = content_length;
	// char* writebuf = max_file_size;
	// int writelen = content_length;
	// int fsize = 0;

	// ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	// int sockfd, i;
	int ret;
	// struct hostent* hostinfo;
	uint8_t* recv_buffer;
	// struct sockaddr_in dest;
	// iot_sha256_context ctx;
	// uint8_t sha256_result[32];
	// uint8_t sha256_img[32];
	bl_mtd_handle_t handle;

	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &handle, BL_MTD_OPEN_FLAG_BACKUP);
	if(ret)
	{
		// return http_rest_error(request, -20, "Open Default FW partition failed");
        puts("Open Default FW partition failed\r\n");
        return 1;
	}

	recv_buffer = pvPortMalloc(OTA_PROGRAM_SIZE);

	unsigned int flash_offset, ota_addr;
	uint32_t bin_size;
	uint8_t activeID;
	HALPartition_Entry_Config ptEntry;

	activeID = hal_boot2_get_active_partition();

	// printf("Starting OTA test. OTA bin addr is %p, incoming len %i\r\n", recv_buffer, writelen);

	// printf("[OTA] [TEST] activeID is %u\r\n", activeID);

	if(hal_boot2_get_active_entries(BOOT2_PARTITION_TYPE_FW, &ptEntry))
	{
		vPortFree(recv_buffer);
		bl_mtd_close(handle);
		// return http_rest_error(request, -20, "PtTable_Get_Active_Entries fail");
        puts("PtTable_Get_Active_Entries fail\r\n");
        return 1;
	}
	ota_addr = ptEntry.Address[!ptEntry.activeIndex];
	bin_size = ptEntry.maxLen[!ptEntry.activeIndex];

	// part_size = ptEntry.maxLen[!ptEntry.activeIndex];
	// (void)part_size;
	/*XXX if you use bin_size is product env, you may want to set bin_size to the actual
	 * OTA BIN size, and also you need to splilt XIP_SFlash_Erase_With_Lock into
	 * serveral pieces. Partition size vs bin_size check is also needed
	 */

	// printf("Starting OTA test. OTA size is %lu\r\n", bin_size);

    if(content_length >= bin_size)
    {
        puts("BIN file is too big");
        return 1;
        // return http_rest_error(request, -20, "Too large bin");
    }

	// printf("[OTA] [TEST] activeIndex is %u, use OTA address=%08x\r\n", ptEntry.activeIndex, (unsigned int)ota_addr);

	printf("[OTA] Erasing flash (%lu)...\r\n", bin_size);
	hal_update_mfg_ptable();

	// Erase in chunks, because erasing everything at once is slow and causes issues with http connection
    // ERASING
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
		// printf("[OTA] Erased:  %lu / %lu \r\n", erase_offset, erase_len);
		erase_offset += erase_len;
        sys_msleep(100);
	}
	printf("[OTA] Flash erased\r\n");
    // === ERASING ===


    // TODO: Edit write logic code
    // If more data is expected, read it into a buffer
    // grab data from http post request and write it to flash
    
    
    
	printf("[OTA] Flashing new firmware\r\n");
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    int remaining = content_length;
    // char *data_start;
	int total = 0;
    char *data_end;
    bool first = true;
    flash_offset = 0;
    while (remaining > 0) {
        // char chunk[MAX_BUFFER_SIZE];
        // int to_read = remaining > MAX_BUFFER_SIZE ? MAX_BUFFER_SIZE : remaining;
        err_t recv_err = netconn_recv(conn, &inbuf);
        if (recv_err == ERR_OK) {
            netbuf_data(inbuf, (void **)&buf, &buflen);
            // memcpy(post_data + body_length, buf, buflen);
            // body_length += buflen;
            // printf("Buflen: %d\r\n", buflen);
            remaining -= buflen;
            // data transmission
            if (first) {
                char *before = buf;
                buf = strstr(buf, "\r\n\r\n");
                buf += 4; // Skip past "\r\n\r\n"
                first = false;
                int skipped_bytes = buf - before;
                if(skipped_bytes <= 0) {
                    puts("Something is totally wrong\r\n");
                    netbuf_delete(inbuf);
                    return ERR_ARG;
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
            // printf("Flash takes %i\r\n", buflen);
            // printf("remaining %i\r\n", remaining);
            // printf("total %i\r\n", total);
            netbuf_delete(inbuf);
        } else {
            puts("Error receiving additional data\n");
            // free(post_data);
            netbuf_delete(inbuf);
            return ERR_CLSD;
        }
    }
	printf("[OTA] Flashed new firmware\r\n");
    // flash write logic
	// buffer_offset = 0;
	// flash_offset = 0;
	// do
	// {
	// 	char* useBuf = writebuf;
	// 	int useLen = writelen;


	// 	if(useLen)
	// 	{
			
	// 		//ADDLOG_DEBUG(LOG_FEATURE_OTA, "%d bytes to write", writelen);
	// 		//add_otadata((unsigned char*)writebuf, writelen);

	// 		printf("Flash takes %i. ", useLen);
	// 		bl_mtd_write(handle, flash_offset, useLen, (u_int8_t*)useBuf);
	// 		flash_offset += useLen;
	// 	}

	// 	total += writelen;
	// 	towrite -= writelen;

	// } while((towrite > 0) && (writelen >= 0));
    // ==== flash write logic ====
	printf("[OTA] set OTA partition length\r\n");
	ptEntry.len = total;
	// printf("[OTA] [TCP] Update PARTITION, partition len is %lu\r\n", ptEntry.len);
	hal_boot2_update_ptable(&ptEntry);
	puts("[OTA] Rebooting...\r\n");
    sys_msleep(500);
	vPortFree(recv_buffer);
	bl_mtd_close(handle);
    hal_reboot();
    return ERR_OK;
}

static int http_rest_post_wifi(char* buf) {
    // SSID
    char* ssid_start = strstr(buf, "name=\"ssid\"\r\n\r\n");
    ssid_start += 15;
    if(ssid_start == NULL) {
        puts("Can't find SSID");
        return ERR_ARG;
    }
    char* ssid_end = strstr(ssid_start, "\r\n------");
    if(ssid_end == NULL) {
        puts("Can't find SSID end");
        printf("Start: %s\r\n", ssid_start);
        return ERR_ARG;
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
        return ERR_ARG;
    }
    pass_start += 15;
    char* pass_end = strstr(pass_start, "\r\n------");
    if(pass_end == NULL) {
        puts("Can't find PASS end\r\n");
        return ERR_ARG;
    }
    int pass_len = pass_end - pass_start;
    printf("PASS length: %d\r\n", pass_len);
    char pass[pass_len + 1];
    memcpy(pass, pass_start, pass_len);
    pass[pass_len] = '\0';
    printf("Wifi PASS: %s\r\n", pass);
    ef_set_env_blob(WIFI_SSID_KEY, (const char *)&ssid, ssid_len + 1);
    ef_set_env_blob(WIFI_PASS_KEY, (const char *)&pass, pass_len + 1);
    return ERR_OK;
}

err_t httpd_handler(struct netconn *conn) {
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;

    // Receive HTTP request
    if (netconn_recv(conn, &inbuf) == ERR_OK) {
        netbuf_data(inbuf, (void **)&buf, &buflen);
        
        printf("HTTP Length: %i\n", buflen);
        printf("HTTP Request: %s\n\n", buf);

        // Check if the request is a POST (for file upload)
        if (strncmp(buf, "POST ", 5) == 0) {
            puts("[httpd_handler] Handling POST request...\n");
            int content_length = 0;
            // Parse Content-Length header
            char *content_length_str = strstr(buf, "Content-Length:");
            if (content_length_str) {
                sscanf(content_length_str, "Content-Length: %d", &content_length);
                printf("Content-Length header found: %d\n", content_length);
            } else {
                puts("Content-Length header not found\n");
            }
            if(strncmp(buf, "POST /ota", 9) == 0) {
                netbuf_delete(inbuf);
                puts("[httpd_handler] Handling firmware upload...\n");
                // I KNOW I'M SENDING the firmware now
                // Parse and handle the firmware data
                if (http_rest_post_flash(conn, content_length) != ERR_OK) {
                    const char *error_response =
                        "HTTP/1.1 500 Internal Server Error\r\n"
                        "Connection: close\r\n\r\n"
                        "Failed to process firmware upload.";
                    netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
                } else {
                    const char *success_response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n\r\n"
                        "Firmware uploaded successfully.";
                    netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
                }
            } else if (strncmp(buf, "POST /wifi", 10) == 0) {
                puts("[httpd_handler] Handling wifi settings request...\n");
                char* body = malloc(content_length + 1);
                char* body_start = strstr(buf, "\r\n\r\n");
                body_start += 4;
                char* body_end = buf + buflen;
                int total_length = body_end - body_start;
                printf("Total length: %i\r\n", total_length);
                memcpy(body, body_start, total_length);
                netbuf_delete(inbuf);
                if (netconn_recv(conn, &inbuf) == ERR_OK) {
                    puts("Got additional data\r\n");
                    netbuf_data(inbuf, (void **)&buf, &buflen);
                    printf("Additional data length: %i\r\n", buflen);
                    memcpy(body + total_length, buf, buflen);
                }
                body[content_length] = '\0';
                printf("Body: %s\r\n", body);
                if (http_rest_post_wifi(body) != ERR_OK) {
                    const char *error_response =
                        "HTTP/1.1 500 Internal Server Error\r\n"
                        "Connection: close\r\n\r\n"
                        "Failed to process new wifi settings.";
                    netconn_write(conn, error_response, strlen(error_response), NETCONN_COPY);
                } else {
                    const char *success_response =
                        "HTTP/1.1 200 OK\r\n"
                        "Connection: close\r\n\r\n"
                        "Changed wifi settings.";
                    netconn_write(conn, success_response, strlen(success_response), NETCONN_COPY);
                }
                netbuf_delete(inbuf);
                free(body);
            }
            else if (strncmp(buf, "POST /set_pin_high", 18) == 0 || strncmp(buf, "POST /set_pin_low", 17) == 0) {
                char* body = malloc(content_length + 1);
                char* body_start = strstr(buf, "\r\n\r\n");
                body_start += 4;
                char* body_end = buf + buflen;
                int total_length = body_end - body_start;
                printf("Total length: %i\r\n", total_length);
                memcpy(body, body_start, total_length);
                body[content_length] = '\0';
                char* start = strstr(body, "name=\"pin\"\r\n\r\n");
                start += 14;
                int channel_index = atoi(start);
                bool setHigh = strncmp(buf, "POST /set_pin_high", 18) == 0;
                set_channel_duty(channel_index, setHigh ? 100 : 0);
                netbuf_delete(inbuf);
            }
            else if (strncmp(buf, "POST /pin_mapping", 17) == 0) {
                char* body = malloc(content_length + 1);
                char* body_start = strstr(buf, "\r\n\r\n");
                body_start += 4;
                char* body_end = buf + buflen;
                int total_length = body_end - body_start;
                printf("Total length: %i\r\n", total_length);
                memcpy(body, body_start, total_length);
                netbuf_delete(inbuf);
                if (netconn_recv(conn, &inbuf) == ERR_OK) {
                    puts("Got additional data\r\n");
                    netbuf_data(inbuf, (void **)&buf, &buflen);
                    printf("Additional data length: %i\r\n", buflen);
                    memcpy(body + total_length, buf, buflen);
                }
                body[content_length] = '\0';
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
                    if(color == 'r') color_to_channel_pin_map[0] = i;
                    if(color == 'g') color_to_channel_pin_map[1] = i;
                    if(color == 'b') color_to_channel_pin_map[2] = i;
                    if(color == 'w') color_to_channel_pin_map[3] = i;
                }
                netbuf_delete(inbuf);
            }
            else if (strncmp(buf, "POST /new_duty", 17) == 0) {
                char* body = malloc(content_length + 1);
                char* body_start = strstr(buf, "\r\n\r\n");
                body_start += 4;
                char* body_end = buf + buflen;
                int total_length = body_end - body_start;
                printf("Total length: %i\r\n", total_length);
                memcpy(body, body_start, total_length);
                // netbuf_delete(inbuf);
                // if (netconn_recv(conn, &inbuf) == ERR_OK) {
                //     puts("Got additional data\r\n");
                //     netbuf_data(inbuf, (void **)&buf, &buflen);
                //     printf("Additional data length: %i\r\n", buflen);
                //     memcpy(body + total_length, buf, buflen);
                // }
                body[content_length] = '\0';
                printf("Body: %s\r\n", body);
                // char colors[5] = {'-', '-', '-', '-', '\0'};
                // char* start = strstr(body, "name=\"p0\"\r\n\r\n");
                // start += 13;
                // if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[0] = start[0];
                // start = strstr(body, "name=\"p1\"\r\n\r\n");
                // start += 13;
                // if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[1] = start[0];
                // start = strstr(body, "name=\"p2\"\r\n\r\n");
                // start += 13;
                // if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[2] = start[0];
                // start = strstr(body, "name=\"p3\"\r\n\r\n");
                // start += 13;
                // if(start[0] == 'r' || start[0] == 'g' || start[0] == 'b' || start[0] == 'w') colors[3] = start[0];
                // for(int i = 0; i < 4; i++) {
                //     char color = colors[i];
                //     if(color == 'r') color_to_channel_pin_map[0] = i;
                //     if(color == 'g') color_to_channel_pin_map[1] = i;
                //     if(color == 'b') color_to_channel_pin_map[2] = i;
                //     if(color == 'w') color_to_channel_pin_map[3] = i;
                // }
                netbuf_delete(inbuf);
            }
            else if (strncmp(buf, "POST /reboot", 12) == 0) {
                puts("[httpd_handler] Rebooting...\n");
                netbuf_delete(inbuf);
                hal_reboot();
            } 
            else netbuf_delete(inbuf);
        } else {
            // Default response (serves an HTML page)
            const char *response =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n\r\n";
            netconn_write(conn, response, strlen(response), NETCONN_COPY);
            netconn_write(conn, html_page, strlen(html_page), NETCONN_COPY);
            netbuf_delete(inbuf);
        }
    }

    // Close and delete the connection
    netconn_close(conn);

    return ERR_OK;
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

    while (1) {
        // Accept new connections
        if (netconn_accept(conn, &newconn) == ERR_OK) {
            puts("[http_server] New tcp connection established\n");
            httpd_handler(newconn);
            netconn_delete(newconn);
            puts("[http_server] Tcp connection closed successfully\n");
        }
        else {
            puts("[http_server] Couldn't establish tcp connection\n");
        }
    }
}