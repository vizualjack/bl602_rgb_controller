#ifndef _PWM_
#define _PWM_

extern const int channel_pin_map[4][2];

extern void set_channel_duty(int channel_index, float duty);
extern void update_channel_duty(int channel_index, float duty);
// extern void set_rgb_duty(int red_duty, int green_duty, int blue_duty);
extern void set_rgbw_duty(float red_duty, float green_duty, float blue_duty, float white_duty);
extern void update_rgbw_duties(float red, float green, float blue, float white);
extern void set_red_channel(int channel_index);
extern void set_green_channel(int channel_index);
extern void set_blue_channel(int channel_index);
extern void set_white_channel(int channel_index);
extern void save_channels();
extern void init_pwm();
extern void pwm_changer(void *pvParameters);

#endif