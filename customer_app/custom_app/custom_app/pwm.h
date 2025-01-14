#ifndef _PWM_
#define _PWM_

extern const int channel_pin_map[4][2];
// Color to pwm channel map 
extern int color_to_channel_pin_map[4]; // r g b w 

extern void set_channel_duty(int channel_index, int duty);
extern void set_rgb_duty(int red_duty, int green_duty, int blue_duty);
extern void set_rgbw_duty(int red_duty, int green_duty, int blue_duty, int white_duty);

#endif