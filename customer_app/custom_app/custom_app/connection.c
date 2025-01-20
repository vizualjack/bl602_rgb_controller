#include <FreeRTOS.h>
#include <task.h>
#include <wifi_mgmr_ext.h>

#include "connection.h"
#include "main.h"
#include "persistence.h"


const char* WIFI_SSID_KEY = "wifi_ssid";
const char* WIFI_PASS_KEY = "wifi_pass";
const char* BOOT_COUNTER = "boot_counter";

static void connect_wifi(char *ssid, char *password)
{
    wifi_interface_t wifi_interface;
    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
}

void check_for_reset() {
    char* boot_counter = get_saved_value(BOOT_COUNTER);
    if(boot_counter == NULL) {
        set_saved_value(BOOT_COUNTER, "1");
    } else {
        int boot_count = atoi(boot_counter);
        boot_count++;
        if(boot_count >= 3) {
            clean_saved_value(WIFI_SSID_KEY);
            clean_saved_value(WIFI_PASS_KEY);
        }
        char boot_count_str[2];
        snprintf(boot_count_str, sizeof(boot_count_str), "%d", boot_count);
        set_saved_value(BOOT_COUNTER, boot_count_str);
    }
    vTaskDelay(2000);
    clean_saved_value(BOOT_COUNTER);
}

void handle_connection(void *pvParameters) {
    while (1) {
        if(finished_init) {
            check_for_reset();
            char* ssid = get_saved_value(WIFI_SSID_KEY);
            char* pass = get_saved_value(WIFI_PASS_KEY);
            if (ssid != NULL && pass != NULL) {
                puts("Connecting to wifi...\r\n");
                connect_wifi(ssid, pass);
            }
            else {
                if(ssid != NULL) free(ssid);
                if(pass != NULL) free(pass);
                puts("Starting ap...\r\n");
                wifi_mgmr_ap_start(wifi_mgmr_ap_enable(), "Need to setup", 0, NULL, 1);
            }
            break;
        }
        vTaskDelay(100);
    }
    puts("Custom task end");
    vTaskDelete(NULL);
}