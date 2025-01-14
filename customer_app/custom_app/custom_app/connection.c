#include <FreeRTOS.h>
#include <task.h>
#include <easyflash.h>
#include <wifi_mgmr_ext.h>

#include "connection.h"
#include "main.h"

static void connect_wifi(char *ssid, char *password)
{
    wifi_interface_t wifi_interface;
    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
}

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

void handle_connection(void *pvParameters) {
    vTaskDelay(500);
    while (1) {
        if(finished_init) {
            // ef_print_env();
            char* ssid = get_saved_ssid();
            char* pass = get_saved_pass();
            // printf("SSID: %s\r\n", ssid);
            // printf("PASS: %s\r\n", pass);
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