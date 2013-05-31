/*
 * gpio.h
 */

#ifndef GPIO_H_
#define GPIO_H_

typedef enum {
	BUTTON_CONNECT	=	0x01
} button_e;

typedef enum {
	RGB_OFF	= 0,
	RGB_RED,
	RGB_RED_BLINKING,
	RGB_BLUE,
	RGB_BLUE_BLINKING,
	RGB_GREEN,
	RGB_GREEN_BLINKING,
	RGB_GREEN_SINGLE_SHOT
} rgb_mode_e;

void release_leds(int signum);
int init_gpio();
int is_button_pressed(button_e button);
void set_rgb_led_mode(rgb_mode_e mode);
int get_battery_charge();

#endif /* GPIO_H_ */
