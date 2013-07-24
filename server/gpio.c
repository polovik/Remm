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
#include "../utils.h"

//	BUTTON_CONNECT connected to P5-04 pin (GPIO 29).
#define BUTTON_GPIO	29
//	RGB LED. Red: P5-03(GPIO 28); Green: P5-06(GPIO 31); Blue: P5-05(GPIO 30).
#define RGB_R_GPIO	28
#define RGB_G_GPIO	31
#define RGB_B_GPIO	30
//	Battery level checking
#define BATTERY_CHARGE_CAPACITY_GPIO	10	//	Output
#define BATTERY_CHECK_CAPACITY_GPIO	24	//	Input

static rgb_mode_e rgb_mode;
static pthread_t leds_thread;
static pthread_t battery_thread;
static int exit_thread = 0;
static struct timeval expiry_time;
static int led_turned_on;
static int battery_charge = 0;

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
					rgb_mode = RGB_GREEN;
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

void *battery_status(void *arg)
{
	struct timeval started_at, finished_at, elapsed;
	int charging_duration, discharging_duration;
	int i;

	printf("INFO  %s() Start battery level checking.\n", __FUNCTION__);
	return 0;

	while (exit_thread == 0) {
		/*	Charge	*/
		printf("INFO  %s() Charge capacity.\n", __FUNCTION__);
		digitalWrite(BATTERY_CHARGE_CAPACITY_GPIO, HIGH);
		gettimeofday(&started_at, NULL);
		for (i = 0; i < 5; i++) {
			sleep(1);
			if (exit_thread != 0)
				return 0;
		}
		digitalWrite(BATTERY_CHARGE_CAPACITY_GPIO, LOW);
		gettimeofday(&finished_at, NULL);
		timersub(&finished_at, &started_at, &elapsed);
		charging_duration = elapsed.tv_sec * 1000000 + elapsed.tv_usec;
		/*	Discharge	*/
		gettimeofday(&started_at, NULL);
		printf("INFO  %s() Capacity has been charged on %dusec.\n", __FUNCTION__, charging_duration);
		while (digitalRead(BATTERY_CHECK_CAPACITY_GPIO) == LOW) {
			usleep(10 * 1000);
		}
		gettimeofday(&finished_at, NULL);
		timersub(&finished_at, &started_at, &elapsed);
		discharging_duration = elapsed.tv_sec * 1000000 + elapsed.tv_usec;
		battery_charge = (discharging_duration - charging_duration);
		printf("INFO  %s() Capacity has been discharged on %dusec. Battery charge = %d\n", __FUNCTION__, discharging_duration, battery_charge);
		for (i = 0; i < 10; i++) {
			sleep(1);
			if (exit_thread != 0)
				return 0;
		}
	}
	return 0;
}

int get_battery_charge()
{
	return battery_charge;
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
	if (pthread_join(battery_thread, NULL) != 0) {
		perror("Joining to battery_thread");
	}
	digitalWrite(BATTERY_CHARGE_CAPACITY_GPIO, LOW);
	close_gpio(BATTERY_CHARGE_CAPACITY_GPIO);
	close_gpio(BATTERY_CHECK_CAPACITY_GPIO);
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
	if (set_gpio_mode(BATTERY_CHARGE_CAPACITY_GPIO, OUTPUT) != 0)
		return -1;
	if (set_gpio_mode(BATTERY_CHECK_CAPACITY_GPIO, INPUT) != 0)
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
	ret = pthread_create(&battery_thread, NULL, battery_status, NULL);
	if (ret != 0) {
		perror("Starting battery thread");
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
