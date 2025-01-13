/*
 * Copyright (c) 2020 Bouffalolab.
 *
 * This file is part of
 *     *** Bouffalolab Software Dev Kit ***
 *      (see www.bouffalolab.com).
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of Bouffalo Lab nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <vfs.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <cli.h>

#include <lwip/tcpip.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <lwip/tcp.h>
#include <lwip/err.h>
#include <http_client.h>
#include <netutils/netutils.h>

#include <bl602_glb.h>
#include <bl602_hbn.h>

#include <bl_uart.h>
#include <bl_chip.h>
#include <bl_wifi.h>
#include <hal_wifi.h>
#include <bl_sec.h>
#include <bl_cks.h>
#include <bl_irq.h>
#include <bl_dma.h>
#include <bl_timer.h>
#include <bl_gpio_cli.h>
#include <bl_wdt_cli.h>
#include <bl_sys_ota.h>
#include <hal_uart.h>
#include <hal_sys.h>
#include <hal_gpio.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <looprt.h>
#include <loopset.h>
#include <sntp.h>
#include <bl_sys_time.h>
#include <bl_sys_ota.h>
#include <bl_romfs.h>
#include <fdt.h>

#include <easyflash.h>
#include <bl60x_fw_api.h>
#include <wifi_mgmr_ext.h>
#include <utils_log.h>
#include <libfdt.h>
#include <blog.h>

#include <bl_mtd.h>
#include "lwip/api.h"
#include "page.h"
#include "bl_pwm.h"
#include "hal_pwm.h"

#define mainHELLO_TASK_PRIORITY     ( 20 )
#define UART_ID_2 (2)
#define WIFI_AP_PSM_INFO_SSID           "conf_ap_ssid"
#define WIFI_AP_PSM_INFO_PASSWORD       "conf_ap_psk"
#define WIFI_AP_PSM_INFO_PMK            "conf_ap_pmk"
#define WIFI_AP_PSM_INFO_BSSID          "conf_ap_bssid"
#define WIFI_AP_PSM_INFO_CHANNEL        "conf_ap_channel"
#define WIFI_AP_PSM_INFO_IP             "conf_ap_ip"
#define WIFI_AP_PSM_INFO_MASK           "conf_ap_mask"
#define WIFI_AP_PSM_INFO_GW             "conf_ap_gw"
#define WIFI_AP_PSM_INFO_DNS1           "conf_ap_dns1"
#define WIFI_AP_PSM_INFO_DNS2           "conf_ap_dns2"
#define WIFI_AP_PSM_INFO_IP_LEASE_TIME  "conf_ap_ip_lease_time"
#define WIFI_AP_PSM_INFO_GW_MAC         "conf_ap_gw_mac"
#define CLI_CMD_AUTOSTART1              "cmd_auto1"
#define CLI_CMD_AUTOSTART2              "cmd_auto2"

extern void ble_stack_start(void);
/* TODO: const */
volatile uint32_t uxTopUsedPriority __attribute__((used)) =  configMAX_PRIORITIES - 1;
static uint32_t time_main;

static wifi_conf_t conf =
{
    .country_code = "CN",
};
extern uint8_t _heap_start;
extern uint8_t _heap_size; // @suppress("Type cannot be resolved")
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size; // @suppress("Type cannot be resolved")
static HeapRegion_t xHeapRegions[] =
{
        { &_heap_start, (unsigned int) &_heap_size}, //set on runtime
        { &_heap_wifi_start, (unsigned int) &_heap_wifi_size },
        { NULL, 0 }, /* Terminates the array. */
        { NULL, 0 } /* Terminates the array. */
};
static wifi_interface_t wifi_interface;


bool finished_init = false;
static char* WIFI_SSID_KEY = "wifi_ssid";
static char* WIFI_PASS_KEY = "wifi_pass";

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName )
{
    puts("Stack Overflow checked\r\n");
    printf("Task Name %s\r\n", pcTaskName);
    while (1) {
        /*empty here*/
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("Memory Allocate Failed. Current left size is %d bytes\r\n",
        xPortGetFreeHeapSize()
    );
    while (1) {
        /*empty here*/
    }
}

void vApplicationIdleHook(void)
{
#if 0
    __asm volatile(
            "   wfi     "
    );
    /*empty*/
#else
    (void)uxTopUsedPriority;
#endif
}

static unsigned char char_to_hex(char asccode)
{
    unsigned char ret;

    if('0'<=asccode && asccode<='9')
        ret=asccode-'0';
    else if('a'<=asccode && asccode<='f')
        ret=asccode-'a'+10;
    else if('A'<=asccode && asccode<='F')
        ret=asccode-'A'+10;
    else
        ret=0;

    return ret;
}

static void _chan_str_to_hex(uint8_t *chan_band, uint16_t *chan_freq, char *chan)
{
    int i, freq_len, base=1;
    uint8_t band;
    uint16_t freq = 0;
    char *p, *q;

    /*should have the following format
     * 2412|0
     * */
    p = strchr(chan, '|') + 1;
    if (NULL == p) {
        return;
    }
    band = char_to_hex(p[0]);
    (*chan_band) = band;

    freq_len = strlen(chan) - strlen(p) - 1;
    q = chan;
    q[freq_len] = '\0';
    for (i=0; i< freq_len; i++) {
       freq = freq + char_to_hex(q[freq_len-1-i]) * base;
       base = base * 10;
    }
    (*chan_freq) = freq;
}

static void bssid_str_to_mac(uint8_t *hex, char *bssid, int len)
{
   unsigned char l4,h4;
   int i,lenstr;
   lenstr = len;

   if(lenstr%2) {
       lenstr -= (lenstr%2);
   }

   if(lenstr==0){
       return;
   }

   for(i=0; i < lenstr; i+=2) {
       h4=char_to_hex(bssid[i]);
       l4=char_to_hex(bssid[i+1]);
       hex[i/2]=(h4<<4)+l4;
   }
}

int check_dts_config(char ssid[33], char password[64])
{
    bl_wifi_ap_info_t sta_info;

    if (bl_wifi_sta_info_get(&sta_info)) {
        /*no valid sta info is got*/
        return -1;
    }

    strncpy(ssid, (const char*)sta_info.ssid, 32);
    ssid[31] = '\0';
    strncpy(password, (const char*)sta_info.psk, 64);
    password[63] = '\0';

    return 0;
}

static void _connect_wifi()
{
    puts("_connect_wifi IS IN USE");
    /*XXX caution for BIG STACK*/
    char pmk[66], bssid[32], chan[10];
    char ssid[33], password[66];
    char val_buf[66];
    char val_len = sizeof(val_buf) - 1;
    uint8_t mac[6];
    uint8_t band = 0;
    uint16_t freq = 0;

    wifi_interface = wifi_mgmr_sta_enable();
    printf("[APP] [WIFI] [T] %lld\r\n"
           "[APP]   Get STA %p from Wi-Fi Mgmr, pmk ptr %p, ssid ptr %p, password %p\r\n",
           aos_now_ms(),
           wifi_interface,
           pmk,
           ssid,
           password
    );
    memset(pmk, 0, sizeof(pmk));
    memset(ssid, 0, sizeof(ssid));
    memset(password, 0, sizeof(password));
    memset(bssid, 0, sizeof(bssid));
    memset(mac, 0, sizeof(mac));
    memset(chan, 0, sizeof(chan));

    memset(val_buf, 0, sizeof(val_buf));
    ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_SSID, val_buf, val_len, NULL);
    if (val_buf[0]) {
        /*We believe that when ssid is set, wifi_confi is OK*/
        strncpy(ssid, val_buf, sizeof(ssid) - 1);

        /*setup password ans PMK stuff from ENV*/
        memset(val_buf, 0, sizeof(val_buf));
        ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_PASSWORD, val_buf, val_len, NULL);
        if (val_buf[0]) {
            strncpy(password, val_buf, sizeof(password) - 1);
        }

        memset(val_buf, 0, sizeof(val_buf));
        ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_PMK, val_buf, val_len, NULL);
        if (val_buf[0]) {
            strncpy(pmk, val_buf, sizeof(pmk) - 1);
        }
        if (0 == pmk[0]) {
            printf("[APP] [WIFI] [T] %lld\r\n",
               aos_now_ms()
            );
            puts("[APP]    Re-cal pmk\r\n");
            /*At lease pmk is not illegal, we re-cal now*/
            //XXX time consuming API, so consider lower-prirotiy for cal PSK to avoid sound glitch
            wifi_mgmr_psk_cal(
                    password,
                    ssid,
                    strlen(ssid),
                    pmk
            );
            ef_set_env(WIFI_AP_PSM_INFO_PMK, pmk);
            ef_save_env();
        }
        memset(val_buf, 0, sizeof(val_buf));
        ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_CHANNEL, val_buf, val_len, NULL);
        if (val_buf[0]) {
            strncpy(chan, val_buf, sizeof(chan) - 1);
            printf("connect wifi channel = %s\r\n", chan);
            _chan_str_to_hex(&band, &freq, chan);
        }
        memset(val_buf, 0, sizeof(val_buf));
        ef_get_env_blob((const char *)WIFI_AP_PSM_INFO_BSSID, val_buf, val_len, NULL);
        if (val_buf[0]) {
            strncpy(bssid, val_buf, sizeof(bssid) - 1);
            printf("connect wifi bssid = %s\r\n", bssid);
            bssid_str_to_mac(mac, bssid, strlen(bssid));
            printf("mac = %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                    mac[0],
                    mac[1],
                    mac[2],
                    mac[3],
                    mac[4],
                    mac[5]
            );
        }
    } else if (0 == check_dts_config(ssid, password)) {
        /*nothing here*/
    } else {
        /*Won't connect, since ssid config is empty*/
        puts("[APP]    Empty Config\r\n");
        puts("[APP]    Try to set the following ENV with psm_set command, then reboot\r\n");
        puts("[APP]    NOTE: " WIFI_AP_PSM_INFO_PMK " MUST be psm_unset when conf is changed\r\n");
        puts("[APP]    env: " WIFI_AP_PSM_INFO_SSID "\r\n");
        puts("[APP]    env: " WIFI_AP_PSM_INFO_PASSWORD "\r\n");
        puts("[APP]    env(optinal): " WIFI_AP_PSM_INFO_PMK "\r\n");
        return;
    }

    printf("[APP] [WIFI] [T] %lld\r\n"
           "[APP]    SSID %s\r\n"
           "[APP]    SSID len %d\r\n"
           "[APP]    password %s\r\n"
           "[APP]    password len %d\r\n"
           "[APP]    pmk %s\r\n"
           "[APP]    bssid %s\r\n"
           "[APP]    channel band %d\r\n"
           "[APP]    channel freq %d\r\n",
           aos_now_ms(),
           ssid,
           strlen(ssid),
           password,
           strlen(password),
           pmk,
           bssid,
           band,
           freq
    );
    //wifi_mgmr_sta_connect(wifi_interface, ssid, pmk, NULL);
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, pmk, mac, band, freq);
}

static void wifi_sta_connect(char *ssid, char *password)
{
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
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


/////// CUSTOM CODE HERE
#define OTA_PROGRAM_SIZE (512)
static int http_rest_post_flash(struct netconn *conn, int content_length)
{
	int total = 0;
	int towrite = content_length;
	// char* writebuf = max_file_size;
	int writelen = content_length;
	int fsize = 0;

	// ADDLOG_DEBUG(LOG_FEATURE_OTA, "OTA post len %d", request->contentLength);

	int sockfd, i;
	int ret;
	struct hostent* hostinfo;
	uint8_t* recv_buffer;
	struct sockaddr_in dest;
	// iot_sha256_context ctx;
	uint8_t sha256_result[32];
	uint8_t sha256_img[32];
	bl_mtd_handle_t handle;

	ret = bl_mtd_open(BL_MTD_PARTITION_NAME_FW_DEFAULT, &handle, BL_MTD_OPEN_FLAG_BACKUP);
	if(ret)
	{
		// return http_rest_error(request, -20, "Open Default FW partition failed");
        puts("Open Default FW partition failed\r\n");
        return 1;
	}

	recv_buffer = pvPortMalloc(OTA_PROGRAM_SIZE);

	unsigned int buffer_offset, flash_offset, ota_addr;
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
		// rtos_delay_milliseconds(100);
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
                buflen -= (buf - before);
                printf("[OTA] Found data start, skipping %td bytes\r\n", buf - before);
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

// 0   1   3  4
// 20  21  3  4
const int pwm_to_pin[4][2] = {{0,20},{1,21},{3,3},{4,4}};
const int map[] = {0,1,2,3};

err_t httpd_handler(struct netconn *conn) {
    struct netbuf *inbuf;
    char *buf;
    u16_t buflen;
    static int pin = 0;

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
            else if (strncmp(buf, "POST /pwm_start", 15) == 0) {
                puts("[httpd_handler] Starting pwm\n");
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
                printf("[httpd_handler] body: %s\r\n", body);
                char* start = strstr(body, "name=\"pin_id\"\r\n\r\n");
                start += 14;
                int pin = atoi(start);
                printf("[httpd_handler] pwm pin: %d\r\n", pin);
                start = strstr(body, "name=\"duty\"\r\n\r\n");
                start += 15;
                int duty = atoi(start);
                float duty_as_float = (float)duty;
                int id = pwm_to_pin[pin][0];
                bl_pwm_set_duty(id, duty);
                netbuf_delete(inbuf);
            }  
            else if (strncmp(buf, "POST /pwm_edit_duty", 19) == 0) {
                // puts("[httpd_handler] Edit duty\n");
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
                // printf("[httpd_handler] body: %s\r\n", body);
                // char* end = strstr(start, "\r\n------");
                // bl_pwm_stop(id);
                // bl_pwm_init(id, pin, 60000);
                // bl_pwm_set_duty(id, value);
                // bl_pwm_start(id);
                // netbuf_delete(inbuf);
            }  
            else if (strncmp(buf, "POST /pwm_stop", 14) == 0) {
                puts("[httpd_handler] Stopping pwm\n");
                // bl_pwm_stop(id);
                netbuf_delete(inbuf);
            } 
            // else if (strncmp(buf, "POST /test", 10) == 0) {
            //     puts("[httpd_handler] Pwm spam...\n");
            //     uint8_t pin = 3;
            //     uint8_t id = 0;
            //     bl_pwm_init(id, pin, 60000);
            //     bl_pwm_set_duty(id, 25);
            //     bl_pwm_start(id);
            //     pin = 4;
            //     id = 1;
            //     bl_pwm_init(id, pin, 60000);
            //     bl_pwm_set_duty(id, 25);
            //     bl_pwm_start(id);
            //     pin = 21;
            //     id = 2;
            //     bl_pwm_init(id, pin, 60000);
            //     bl_pwm_set_duty(id, 25);
            //     bl_pwm_start(id);
            //     netbuf_delete(inbuf);
            // } 
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
void http_server(void) {
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
////////////////////////////////////////////////////////////////

static void startWebserver() {
    static bool started = false;
    if(started) return;
    puts("Start http server thread...\r\n");
    sys_thread_new("http_server", (lwip_thread_fn) &http_server, NULL, 4000, (( 32 ) - 2));
    started = true;
}

static void stopWebserver() {
    
}

static void event_cb_wifi_event(input_event_t *event, void *private_data)
{
    static char *ssid;
    static char *password;

    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE:
        {
            printf("[APP] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            wifi_mgmr_start_background(&conf);
        }
        break;
        case CODE_WIFI_ON_MGMR_DONE:
        {
            printf("[APP] [EVT] MGMR DONE %lld, now %lums\r\n", aos_now_ms(), bl_timer_now_us()/1000);
            _connect_wifi();
        }
        break;
        case CODE_WIFI_ON_MGMR_DENOISE:
        {
            printf("[APP] [EVT] Microwave Denoise is ON %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE:
        {
            printf("[APP] [EVT] SCAN Done %lld\r\n", aos_now_ms());
            wifi_mgmr_cli_scanlist();
        }
        break;
        case CODE_WIFI_ON_SCAN_DONE_ONJOIN:
        {
            printf("[APP] [EVT] SCAN On Join %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_DISCONNECT:
        {
            printf("[APP] [EVT] disconnect %lld, Reason: %s\r\n",
                aos_now_ms(),
                wifi_mgmr_status_code_str(event->value)
            );
        }
        break;
        case CODE_WIFI_ON_CONNECTING:
        {
            printf("[APP] [EVT] Connecting %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_CMD_RECONNECT:
        {
            printf("[APP] [EVT] Reconnect %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_CONNECTED:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_PRE_GOT_IP:
        {
            printf("[APP] [EVT] connected %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_GOT_IP:
        {
            printf("[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
            printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
            startWebserver();
        }
        break;
        case CODE_WIFI_ON_EMERGENCY_MAC:
        {
            printf("[APP] [EVT] EMERGENCY MAC %lld\r\n", aos_now_ms());
            hal_reboot();//one way of handling emergency is reboot. Maybe we should also consider solutions
        }
        break;
        case CODE_WIFI_ON_PROV_SSID:
        {
            printf("[APP] [EVT] [PROV] [SSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
            if (ssid) {
                vPortFree(ssid);
                ssid = NULL;
            }
            ssid = (char*)event->value;
        }
        break;
        case CODE_WIFI_ON_PROV_BSSID:
        {
            printf("[APP] [EVT] [PROV] [BSSID] %lld: %s\r\n",
                    aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
            if (event->value) {
                vPortFree((void*)event->value);
            }
        }
        break;
        case CODE_WIFI_ON_PROV_PASSWD:
        {
            printf("[APP] [EVT] [PROV] [PASSWD] %lld: %s\r\n", aos_now_ms(),
                    event->value ? (const char*)event->value : "UNKNOWN"
            );
            if (password) {
                vPortFree(password);
                password = NULL;
            }
            password = (char*)event->value;
        }
        break;
        case CODE_WIFI_ON_PROV_CONNECT:
        {
            // printf("[APP] [EVT] [PROV] [CONNECT] %lld\r\n", aos_now_ms());
            // printf("connecting to %s:%s...\r\n", ssid, password);
            // wifi_sta_connect(ssid, password);
        }
        break;
        case CODE_WIFI_ON_PROV_DISCONNECT:
        {
            printf("[APP] [EVT] [PROV] [DISCONNECT] %lld\r\n", aos_now_ms());
        }
        break;
        case CODE_WIFI_ON_AP_STA_ADD:
        {
            printf("[APP] [EVT] [AP] [ADD] %lld, sta idx is %lu\r\n", aos_now_ms(), (uint32_t)event->value);
            startWebserver();
        }
        break;
        case CODE_WIFI_ON_AP_STA_DEL:
        {
            printf("[APP] [EVT] [AP] [DEL] %lld, sta idx is %lu\r\n", aos_now_ms(), (uint32_t)event->value);
            stopWebserver();
        }
        break;
        default:
        {
            printf("[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}

static void cmd_stack_wifi(char *buf, int len, int argc, char **argv)
{
    /*wifi fw stack and thread stuff*/
    static uint8_t stack_wifi_init  = 0;


    if (1 == stack_wifi_init) {
        puts("Wi-Fi Stack Started already!!!\r\n");
        return;
    }
    stack_wifi_init = 1;

    printf("Start Wi-Fi fw @%lums\r\n", bl_timer_now_us()/1000);
    hal_wifi_start_firmware_task();
    /*Trigger to start Wi-Fi*/
    printf("Start Wi-Fi fw is Done @%lums\r\n", bl_timer_now_us()/1000);
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);

}

static int get_dts_addr(const char *name, uint32_t *start, uint32_t *off)
{
    uint32_t addr = hal_board_get_factory_addr();
    const void *fdt = (const void *)addr;
    uint32_t offset;

    if (!name || !start || !off) {
        return -1;
    }

    offset = fdt_subnode_offset(fdt, 0, name);
    if (offset <= 0) {
       printf("%s NULL.\r\n", name);
       return -1;
    }

    *start = (uint32_t)fdt;
    *off = offset;

    return 0;
}


static void event_loop(void *pvParameters)
{
    uint32_t fdt = 0, offset = 0;
    static StackType_t proc_stack_looprt[512];
    static StaticTask_t proc_task_looprt;

    /*Init bloop stuff*/
    looprt_start(proc_stack_looprt, 512, &proc_task_looprt);
    loopset_led_hook_on_looprt();

    easyflash_init();
    vfs_init();
    vfs_device_init();

    
    if (0 == get_dts_addr("uart", &fdt, &offset)) {
        vfs_uart_init(fdt, offset);
    }
    if (0 == get_dts_addr("gpio", &fdt, &offset)) {
        hal_gpio_init_from_dts(fdt, offset);
    }

    aos_loop_init();

    aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
    cmd_stack_wifi(NULL, 0, 0, NULL);
    finished_init = true;

    for(int i = 0; i < 4; i++) {
        int id = pwm_to_pin[i][0];
        int pin = pwm_to_pin[i][1];
        bl_pwm_init(id, pin, 60000);
        bl_pwm_start(id);
        bl_pwm_set_duty(id, 0);
    }

    aos_loop_run();
    puts("------------------------------------------\r\n");
    puts("+++++++++Critical Exit From Loop++++++++++\r\n");
    puts("******************************************\r\n");
    vTaskDelete(NULL);
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vAssertCalled(void)
{
    volatile uint32_t ulSetTo1ToExitFunction = 0;

    taskDISABLE_INTERRUPTS();
    while( ulSetTo1ToExitFunction != 1 ) {
        __asm volatile( "NOP" );
    }
}

static void system_init(void)
{
    vPortDefineHeapRegions(xHeapRegions);
    printf("Heap %u@%p, %u@%p\r\n",
            (unsigned int)&_heap_size, &_heap_start,
            (unsigned int)&_heap_wifi_size, &_heap_wifi_start
    );
    blog_init();
    bl_irq_init();
    bl_sec_init();
    bl_sec_test();
    bl_dma_init();
    hal_boot2_init();
    /* board config is set after system is init*/
    hal_board_cfg(0);
    tcpip_init(NULL, NULL);

}

int timeout = 1000;

static char* get_saved_ssid() {
    struct env_node_obj info_obj;
    if (!ef_get_env_obj(WIFI_SSID_KEY, &info_obj)) return NULL;
    printf("SSID length: %d\n", info_obj.value_len);
    if(info_obj.value_len <= 0) return NULL;
    char* ssid = malloc(info_obj.value_len);
    ef_get_env_blob(WIFI_SSID_KEY, ssid, info_obj.value_len, NULL);
    return ssid;
}

static char* get_saved_pass() {
    struct env_node_obj info_obj;
    if (!ef_get_env_obj(WIFI_PASS_KEY, &info_obj)) return NULL;
    if(info_obj.value_len <= 0) return NULL;
    char* pass = malloc(info_obj.value_len);
    ef_get_env_blob(WIFI_PASS_KEY, pass, info_obj.value_len, NULL);
    return pass;
}

static void custom_task_func(void *pvParameters) {
    vTaskDelay(1000);
    while (1) {
        if(finished_init) {
            ef_print_env();
            char* ssid = get_saved_ssid();
            char* pass = get_saved_pass();
            printf("SSID: %s\r\n", ssid);
            printf("PASS: %s\r\n", pass);
            if (ssid != NULL && pass != NULL) {
                puts("Connecting to wifi...");
                wifi_sta_connect(ssid, pass);
            }
            else {
                if(ssid != NULL) free(ssid);
                if(pass != NULL) free(pass);
                puts("Starting ap...");
                wifi_mgmr_ap_start(wifi_mgmr_ap_enable(), "Test this", 0, NULL, 1);
            }
            break;
        }
        vTaskDelay(3000);
    }
    puts("Custom task end");
    vTaskDelete(NULL);

}

void bfl_main()
{
    static StackType_t event_loop_stack[2048];
    static StaticTask_t event_loop_task;
    static StackType_t custom_task_stack[512];
    static StaticTask_t custom_task;
    
    time_main = bl_timer_now_us();
    
    bl_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
    printf("Boot2 consumed %lums\r\n", time_main / 1000);
    puts("Starting firmware now....\r\n");
    system_init();
    // Tasks
    xTaskCreateStatic(event_loop, (char*)"event_loop", 1024, NULL, 15, event_loop_stack, &event_loop_task);
    puts("[OS] Added event_loop task\r\n");
    xTaskCreateStatic(custom_task_func, (char*)"custom_task", 512, NULL, 15, custom_task_stack, &custom_task);
    puts("[OS] Starting OS Scheduler...\r\n");
    vTaskStartScheduler();
}