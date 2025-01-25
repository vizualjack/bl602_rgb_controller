#include <FreeRTOS.h>
#include <stdbool.h>
#include <bl_mtd.h>
#include <hal_boot2.h>
#include <lwip/api.h>
#include <lwip/err.h>
#include <string.h>
#include <hal_sys.h>

#include "pwm.h"


void udp_server(void *pvParameters) {
    struct netconn *conn;
    struct netbuf *netbuf;
    puts("[udp_server] Starting...\r\n");
    // Create a new TCP connection
    conn = netconn_new(NETCONN_UDP);
    netconn_bind(conn, IP_ADDR_ANY, 1337);

    puts("[udp_server] Listening on port 1337......\r\n");
    while (1) {
        if(uxQueueMessagesWaiting(conn->recvmbox) > 0) {
            puts("[udp_server] Udp packet in queue\r\n");
            if (netconn_recv(conn, &netbuf) == ERR_OK) {
                // puts("[udp_server] New udp packet\n");
                char *data;
                u16_t len;
                netbuf_data(netbuf, (void **)&data, &len);
                // printf("r: %i\r\n", data[0]);
                // printf("g: %i\r\n", data[1]);
                // printf("b: %i\r\n", data[2]);
                // printf("w: %i\r\n", data[3]);
                int *nums = (int *)data;

                // Convert from network byte order to host byte order
                int r = ntohl(nums[0]);
                int g = ntohl(nums[1]);
                int b = ntohl(nums[2]);
                int w = ntohl(nums[3]);

                // Print the received integers
                printf("r = %d, g = %d, b = %d, w = %d\n", r, g, b, w);

                float red, green, blue, white;
                red =  r / 255.f * 100;
                green = g / 255.f * 100;
                blue = b / 255.f * 100;
                white = w / 255.f * 100;
                printf("r = %f, g = %f, b = %f, w = %f\n", red, green, blue, white);
                set_rgbw_duty(red, green, blue, white);
                netbuf_delete(netbuf);
                // puts("[udp_server] Udp packet handled successfully\n");
            }
            else {
                puts("[udp_server] Couldn't receive udp packet\n");
            }
            puts("[udp_server] Udp packet in queue handled\r\n");
        }
        vTaskDelay(10);
    }
    netconn_delete(conn);
}