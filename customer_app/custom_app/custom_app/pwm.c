
#include <bl_pwm.h>

// 0   1   3  4
// 20  21  3  4
const int channel_pin_map[4][2] = {{0,20},{1,21},{3,3},{4,4}};
int color_to_channel_pin_map[4] = {-1, -1, -1, -1}; // r g b w 

void set_channel_duty(int channel_index, int duty) {
    bl_pwm_set_duty(channel_pin_map[channel_index][0], duty);
}

void set_red_duty(int duty) {
    int index = color_to_channel_pin_map[0];
    if(index == -1) return;
    set_channel_duty(index, duty);
}

void set_green_duty(int duty) {
    int index = color_to_channel_pin_map[1];
    if(index == -1) return;
    set_channel_duty(index, duty);
}

void set_blue_duty(int duty) {
    int index = color_to_channel_pin_map[2];
    if(index == -1) return;
    set_channel_duty(index, duty);
}

void set_white_duty(int duty) {
    int index = color_to_channel_pin_map[3];
    if(index == -1) return;
    set_channel_duty(index, duty);
}

void set_rgb_duty(int red_duty, int green_duty, int blue_duty) {
    set_red_duty(red_duty);
    set_green_duty(green_duty);
    set_blue_duty(blue_duty);
}

void set_rgbw_duty(int red_duty, int green_duty, int blue_duty, int white_duty) {
    set_rgb_duty(red_duty, green_duty, blue_duty);
    set_white_duty(white_duty);
}