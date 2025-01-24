#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <lwip/sys.h>

#include "connection.h"
#include "web_server.h"
#include "starter.h"

#define CONNECTION_TASK_STACK_SIZE REAL_STACK(2048)
#define UDP_SERVER_STACK_SIZE REAL_STACK(1024)
#define HTTP_SERVER_STACK_SIZE REAL_STACK(2048)
// #define HTTP_SERVER_PRIO 7

void my_starter() {
    // static StackType_t connection_task_stack[CONNECTION_TASK_STACK_SIZE];
    // static StaticTask_t connection_task;
    // static StackType_t http_server_task_stack[HTTP_SERVER_STACK_SIZE];
    // static StaticTask_t http_server_task;
    // static StackType_t udp_server_task_stack[UDP_SERVER_STACK_SIZE];
    // static StaticTask_t udp_server_task;
    // static StackType_t pwm_changer_task_stack[PWM_CHANGER_STACK_SIZE];
    // static StaticTask_t pwm_changer_task;
    // xTaskCreateStatic(handle_connection, (char*)"connection_task", CONNECTION_TASK_STACK_SIZE, NULL, 15, connection_task_stack, &connection_task);
    sys_thread_new("connection_task", handle_connection, NULL, CONNECTION_TASK_STACK_SIZE, MY_TASK_PRIO);
    puts("[Starter] Added connection task\r\n");
    // xTaskCreateStatic(http_server, (char*)"http_server", HTTP_SERVER_STACK_SIZE, NULL, HTTP_SERVER_PRIO, http_server_task_stack, &http_server_task);
    sys_thread_new("http_server", http_server, NULL, HTTP_SERVER_STACK_SIZE, MY_TASK_PRIO);
    puts("[Starter] Added http server task\r\n");
    // xTaskCreateStatic(udp_server, (char*)"udp_server", UDP_SERVER_STACK_SIZE, NULL, 15, udp_server_task_stack, &udp_server_task);
    // puts("[Starter] Added udp server task\r\n");
}