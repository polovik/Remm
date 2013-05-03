/*
 * gpio.c
 */
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include "gpio.h"
#include "wiringPi.h"

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

void add_timer(unsigned int msec)
{
	struct timeval cur_time, timeout;
	gettimeofday(&cur_time, 0);

	timerclear(&expiry_time);
	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;
	printf("INFO  %s() Add timer %d:%d.\n", __FUNCTION__, (int)timeout.tv_sec, (int)timeout.tv_usec);
	timeradd(&cur_time, &timeout, &expiry_time);
}

/**	Return 1 only if timer was set and it is expired.
 */
int is_timer_expired()
{
	struct timeval cur_time;

	if (!timerisset(&expiry_time))
		return 0;

	gettimeofday(&cur_time, 0);

	if (timercmp(&cur_time, &expiry_time, <))
		return 0;

	timerclear(&expiry_time);
	printf("INFO  %s() Timer has expired.\n", __FUNCTION__);
	return 1;
}

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
				add_timer(500);
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
				add_timer(500);
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
				add_timer(500);
				break;
			case RGB_GREEN_SINGLE_SHOT:
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				led_turned_on = 0;
				add_timer(50);
				break;
			default:
				assert(0);
				break;
			}
		}
		if (is_timer_expired() == 1) {
			if (prev_rgb_mode == RGB_RED_BLINKING) {
				digitalWrite(RGB_R_GPIO, led_turned_on ? HIGH : LOW);
				led_turned_on = !led_turned_on;
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, HIGH);
				add_timer(500);
			} else if (prev_rgb_mode == RGB_BLUE_BLINKING) {
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, HIGH);
				digitalWrite(RGB_B_GPIO, led_turned_on ? HIGH : LOW);
				led_turned_on = !led_turned_on;
				add_timer(500);
			} else if (prev_rgb_mode == RGB_GREEN_BLINKING) {
				digitalWrite(RGB_R_GPIO, HIGH);
				digitalWrite(RGB_G_GPIO, led_turned_on ? HIGH : LOW);
				led_turned_on = !led_turned_on;
				digitalWrite(RGB_B_GPIO, HIGH);
				add_timer(500);
			} else if (prev_rgb_mode == RGB_GREEN_SINGLE_SHOT) {
				if (led_turned_on == 0) {
					digitalWrite(RGB_R_GPIO, HIGH);
					digitalWrite(RGB_G_GPIO, HIGH);
					digitalWrite(RGB_B_GPIO, LOW);
					led_turned_on = 1;
					add_timer(50);
				} else {
					prev_rgb_mode = RGB_GREEN;
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

void release_leds(int signum)
{
	printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
	exit_thread = 1;
	if (pthread_join(leds_thread, NULL) != 0) {
		perror("Joining to leds_thread");
	}
	printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

//	https://projects.drogon.net/raspberry-pi/wiringpi/pins/
//	http://elinux.org/RPi_Low-level_peripherals
int init_gpio()
{
	int ret;

	if (wiringPiSetupSys() != 0) {
//	if (wiringPiSetup() != 0) {
		printf("ERROR %s() Can't initialize GPIO.\n", __FUNCTION__);
	    return -1;
	}

/*	pinMode(BUTTON_GPIO, INPUT);
	pinMode(RGB_R_GPIO, OUTPUT);
	pinMode(RGB_G_GPIO, OUTPUT);
	pinMode(RGB_B_GPIO, OUTPUT);
*/
	exit_thread = 0;
	timerclear(&expiry_time);
	set_rgb_led_mode(RGB_OFF);
	ret = pthread_create(&leds_thread, NULL, rgb_control, NULL);
	if (ret != 0) {
		perror("Starting grabbing thread");
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
