/*
 * control.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "../connection.h"
#include "../packet.h"
#include "control.h"
#include "display.h"

static control_packet_s control_packet;

void on_mouse(int event, int x, int y, int flags, void *param)
{
//	printf("INFO  %s() Mouse event %d is detected.\n", __FUNCTION__, event);
	switch (event) {
	case CV_EVENT_MOUSEMOVE:
		break;

	case CV_EVENT_LBUTTONDOWN:
		printf("INFO  %s() Mouse left button click was at %d x %d.\n", __FUNCTION__, x, y);
	break;

	case CV_EVENT_LBUTTONUP:
		break;
	}
}

void init_controls()
{
	memset(&control_packet, 0x00, sizeof(control_packet_s));
	control_packet.magic = MAGIC_COMMAND;
	control_packet.command = AUTOPILOT_OFF;
	control_packet.capture_fps = 0.5;

	cvSetMouseCallback(DISPLAY_WINDOW_NAME, on_mouse, NULL);
}

/**	Poll pressed keys.
 *  delay - maximal waiting interval in ms.
 */
void poll_keys(int delay)
{
	char key = cvWaitKey(delay);
	if (key < 0)
		return;

	printf("INFO  %s() Key %c(0x%02X) has been pressed.\n", __FUNCTION__, key, key);

	if (key == '+') {
		float fps = control_packet.capture_fps;
		if (fps < 2)
			fps += 0.1;
		else {
			fps += 1;
			if (fps > 10)
				fps = 10;
		}
		control_packet.capture_fps = fps;
	} else if (key == '-') {
		float fps = control_packet.capture_fps;
		if (fps < 3) {
			fps -= 0.1;
			if (fps < 0)
				fps = 0;
		} else {
			fps -= 1;
		}
		control_packet.capture_fps = fps;
	} else {
		//	Skip unmapped key
		return;
	}

	send_command();
}

void send_command()
{
	//control_packet.height = 0;
	control_packet.direction = 0;
	control_packet.gps_latitude = 16.22;
	control_packet.gps_longitude = 15.44;
	control_packet.slope = 0;
	control_packet.command = AUTOPILOT_OFF;
	send_data(1, (unsigned char *)&control_packet, sizeof(control_packet_s));
}

float get_fps()
{
	return control_packet.capture_fps;
}
