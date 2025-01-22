
#include <bl_pwm.h>
#include "persistence.h"
#include <FreeRTOS.h>
#include <task.h>
#include <stdbool.h>

// 0   1   3  4
// 20  21  3  4
const int channel_pin_map[4][2] = {{0,20},{1,21},{3,3},{4,4}};
// Color to pwm channel map 
int color_to_channel_pin_map[4] = {-1, -1, -1, -1}; // r g b w 
const char* COLOR_TO_CHANNEL_MAP_KEY = "pwm_ctc";
bool need_to_update_rgbw_duties = false;
int current_pwm_duties[4] = {0, 0, 0, 0}; // r g b w 
bool need_to_update_channel_duty = false;
int current_channel = -1;
int current_channel_duty = -1;

void update_rgbw_duties(int red, int green, int blue, int white) {
    current_pwm_duties[0] = red;
    current_pwm_duties[1] = green;
    current_pwm_duties[2] = blue;
    current_pwm_duties[3] = white;
    need_to_update_rgbw_duties = true;
}

void update_channel_duty(int channel_index, int duty) {
    if(channel_index <= -1 || channel_index >= 4) return;
    if(duty < 0 || duty > 100) return;
    current_channel = channel_index;
    current_channel_duty = duty;
    need_to_update_channel_duty = true;
}

void set_channel_duty(int channel_index, int duty) {
    if(channel_index <= -1 || channel_index >= 4) return;
    if(duty < 0 || duty > 100) return;
    bl_pwm_set_duty(channel_pin_map[channel_index][0], duty);
}

void set_color_duty(int color_to_channel_index, int duty) {
    int index = color_to_channel_pin_map[color_to_channel_index];
    set_channel_duty(index, duty);
}

void set_red_duty(int duty) { set_color_duty(0, duty); }
void set_green_duty(int duty) { set_color_duty(1, duty); }
void set_blue_duty(int duty) { set_color_duty(2, duty); }
void set_white_duty(int duty) { set_color_duty(3, duty); }
void set_rgb_duty(int red_duty, int green_duty, int blue_duty) {
    set_red_duty(red_duty);
    set_green_duty(green_duty);
    set_blue_duty(blue_duty);
}
void set_rgbw_duty(int red_duty, int green_duty, int blue_duty, int white_duty) {
    set_rgb_duty(red_duty, green_duty, blue_duty);
    set_white_duty(white_duty);
}

void set_red_channel(int channel_index) { color_to_channel_pin_map[0] = channel_index; }
void set_green_channel(int channel_index) { color_to_channel_pin_map[1] = channel_index; }
void set_blue_channel(int channel_index) { color_to_channel_pin_map[2] = channel_index; }
void set_white_channel(int channel_index) { color_to_channel_pin_map[3] = channel_index; }
void save_channels() {
    char value[5];
    for (int i = 0; i < 4; i++) {
        value[i] = color_to_channel_pin_map[i] + 1;
    }
    value[4] = '\0';
    printf("[pwm_changer] Saved value: %s\r\n", value);
    set_saved_value(COLOR_TO_CHANNEL_MAP_KEY, value);
    puts("[pwm_changer] Channels saved\r\n");
}
void load_channels() {
    char* value = get_saved_value(COLOR_TO_CHANNEL_MAP_KEY);
    if(value == NULL) return;
    printf("[pwm_changer] Loaded value: %s\r\n", value);
    for (int i = 0; i < 4; i++) {
        color_to_channel_pin_map[i] = value[i] - 1;
    }
    puts("[pwm_changer] Channels loaded\r\n");
}

void init_pwm() {
    load_channels();
}

// Pwm changer task for changes via http request
void pwm_changer(void *pvParameters) {
    vTaskDelay(2000);
    puts("[pwm_changer] Initialise...\r\n");
    init_pwm();
    puts("[pwm_changer] Starting...\r\n");
    while (1) {
        if(need_to_update_rgbw_duties) {
            set_rgbw_duty(current_pwm_duties[0], current_pwm_duties[1], current_pwm_duties[2], current_pwm_duties[3]);
            need_to_update_rgbw_duties = false;
        }
        if(need_to_update_channel_duty) {
            set_channel_duty(current_channel, current_channel_duty);
            need_to_update_channel_duty = false;
        }
        vTaskDelay(500);
    }
}