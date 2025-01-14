
#include <bl_pwm.h>

// 0   1   3  4
// 20  21  3  4
const int channel_pin_map[4][2] = {{0,20},{1,21},{3,3},{4,4}};
int color_to_channel_pin_map[4] = {-1, -1, -1, -1}; // r g b w 

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