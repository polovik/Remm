/*
 * gpio.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "gpio.h"
#include "wiringPi.h"
#include "utils.h"

//	BUTTON_CONNECT connected to P5-04 pin (GPIO 29).
#define BUTTON_GPIO	29
//	RGB LED. Red: P5-03(GPIO 28); Green: P5-06(GPIO 31); Blue: P5-05(GPIO 30).
#define RGB_R_GPIO	28
#define RGB_G_GPIO	31
#define RGB_B_GPIO	30

static rgb_mode_e rgb_mode;
static pthread_t leds_thread;
static int exit_thread = 0;
static struct timeval expiry_time;
static int led_turned_on;

void *rgb_control(void *arg)
{
	rgb_mode_e prev_rgb_mode = RGB_OFF;

	printf("INFO  %s() Start controlling.\n", __FUNCTION__);
	while (exit_thread == 0) {
		if (prev_rgb_mode != rgb_mode) {
			prev_rgb_mode = rgb_mode;
			switch (prev_rgb_mode) {
			case RGB_OFF:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				timerclear(&expiry_time);
				break;
			case RGB_RED:
				digitalWrite(RGB_R_GPIO, LOW);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				timerclear(&expiry_time);
				break;
			case RGB_RED_BLINKING:
				digitalWrite(RGB_R_GPIO, LOW);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				add_timer(500, &expiry_time);
				led_turned_on = 1;
				break;
			case RGB_BLUE:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, LOW);
				timerclear(&expiry_time);
				break;
			case RGB_BLUE_BLINKING:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, LOW);
				led_turned_on = 1;
				add_timer(500, &expiry_time);
				break;
			case RGB_GREEN:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, LOW);
				digitalWrite(RGB_B_GPIO, HIGH);
				timerclear(&expiry_time);
				break;
			case RGB_GREEN_BLINKING:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, LOW);
				digitalWrite(RGB_B_GPIO, HIGH);
				led_turned_on = 1;
				add_timer(500, &expiry_time);
				break;
			case RGB_GREEN_SINGLE_SHOT:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				led_turned_on = 0;
				add_timer(20, &expiry_time);
				break;
			default:
				assert(0);
				break;
			}
		}
		if (is_timer_expired(&expiry_time) == 1) {
			if (prev_rgb_mode == RGB_RED_BLINKING) {
				digitalWrite(RGB_R_GPIO, led_turned_on ? HIGH : LOW);
				led_turned_on = !led_turned_on;
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				add_timer(500, &expiry_time);
			} else if (prev_rgb_mode == RGB_BLUE_BLINKING) {
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, led_turned_on ? HIGH : LOW);
				led_turned_on = !led_turned_on;
				add_timer(500, &expiry_time);
			} else if (prev_rgb_mode == RGB_GREEN_BLINKING) {
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, led_turned_on ? HIGH : LOW);
				led_turned_on = !led_turned_on;
				digitalWrite(RGB_B_GPIO, HIGH);
				add_timer(500, &expiry_time);
			} else if (prev_rgb_mode == RGB_GREEN_SINGLE_SHOT) {
				if (led_turned_on == 0) {
					digitalWrite(RGB_R_GPIO, HIGH);
					digitalWrite(RGB_G_GPIO, LOW);
					digitalWrite(RGB_B_GPIO, HIGH);
					led_turned_on = 1;
					add_timer(20, &expiry_time);
				} else {
					rgb_mode = RGB_OFF;
				}
			}
		}
		delayMicroseconds(10000);
	}
	printf("INFO  %s() Controlling is finished.\n", __FUNCTION__);
	exit_thread = 1;
	digitalWrite(RGB_R_GPIO, HIGH);
	digitalWrite(RGB_G_GPIO, HIGH);
	digitalWrite(RGB_B_GPIO, HIGH);
	rgb_mode = RGB_OFF;
	timerclear(&expiry_time);
	return 0;
}

int set_gpio_mode(int gpio, int mode)
{
	char cmd[200];

	snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/gpio/export", gpio);
	if (system(cmd) != 0) {
		printf("ERROR %s() Can't export GPIO %d.\n", __FUNCTION__, gpio);
		return -1;
	}

	snprintf(cmd, sizeof(cmd), "echo %s > /sys/class/gpio/gpio%d/direction",
			  mode == OUTPUT ? "out" : "in", gpio);
	if (system(cmd) != 0) {
		printf("ERROR %s() Can't set direction GPIO %d.\n", __FUNCTION__, gpio);
		return -1;
	}

	return 0;
}

void close_gpio(int gpio)
{
	char cmd[200];

	snprintf(cmd, sizeof(cmd), "echo %d > /sys/class/gpio/unexport", gpio);
	if (system(cmd) != 0) {
		printf("ERROR %s() Finish access to GPIO %d.\n", __FUNCTION__, gpio);
	}
}

void release_leds(int signum)
{
	printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
	exit_thread = 1;
	if (pthread_join(leds_thread, NULL) != 0) {
		perror("Joining to leds_thread");
	}
	close_gpio(RGB_R_GPIO);
	close_gpio(RGB_G_GPIO);
	close_gpio(RGB_B_GPIO);
	printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

//	https://projects.drogon.net/raspberry-pi/wiringpi/pins/
//	http://elinux.org/RPi_Low-level_peripherals
int init_gpio()
{
	int ret;

	if (set_gpio_mode(RGB_R_GPIO, OUTPUT) != 0)
		return -1;
	if (set_gpio_mode(RGB_G_GPIO, OUTPUT) != 0)
		return -1;
	if (set_gpio_mode(RGB_B_GPIO, OUTPUT) != 0)
		return -1;

	if (wiringPiSetupSys() != 0) {
		printf("ERROR %s() Can't initialize GPIO.\n", __FUNCTION__);
	    return -1;
	}

	exit_thread = 0;
	timerclear(&expiry_time);
	set_rgb_led_mode(RGB_OFF);
	ret = pthread_create(&leds_thread, NULL, rgb_control, NULL);
	if (ret != 0) {
		perror("Starting leds thread");
		return -1;
	}

	printf("INFO  %s() LEDS are successfully initiated.\n", __FUNCTION__);

	return 0;
}

int is_button_pressed(button_e button)
{
	return digitalRead(BUTTON_GPIO) == LOW;
}

void set_rgb_led_mode(rgb_mode_e mode)
{
	rgb_mode = mode;
}
