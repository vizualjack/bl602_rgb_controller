#include <FreeRTOS.h>
#include <task.h>
#include <wifi_mgmr_ext.h>

#include "connection.h"
#include "main.h"
#include "persistence.h"


const char* WIFI_SSID_KEY = "wifi_ssid";
const char* WIFI_PASS_KEY = "wifi_pass";

static void connect_wifi(char *ssid, char *password)
{
    wifi_interface_t wifi_interface;
    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
}

void handle_connection(void *pvParameters) {
    vTaskDelay(500);
    while (1) {
        if(finished_init) {
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
        vTaskDelay(1000);
    }
    puts("Custom task end");
    vTaskDelete(NULL);
}