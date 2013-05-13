/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include "../connection.h"
#include "../packet.h"
#include "camera.h"
#include "gpio.h"
#include "../utils.h"

#define STATUS_PACKET_TIMEOUT	500

typedef struct {
	int connection_established;
	int camera_broken;
	int battery_charge;
	float capture_fps;
	struct timeval send_frame_timer;
	struct timeval send_status_timer;
	/*	Position	*/
	int height;		/**<	Height above ground */
	int direction; /**<		Direction accordingly to North Pole	*/
	float gps_latitude;
	float gps_longitude;
	int slope;		/**<	Slope accordingly to horizontal positions */
} server_ctx_s;

static server_ctx_s server_ctx;

void start_communicate()
{
	printf("INFO  %s() Connection established\n", __FUNCTION__);
	server_ctx.connection_established = 1;
	unsigned int timeout = 1000. / server_ctx.capture_fps;
	add_timer(timeout, &server_ctx.send_frame_timer);
	add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
}

void data_rx(unsigned char *data, unsigned int length)
{
	control_packet_s *control_packet;

	if (length != sizeof(control_packet_s)) {
		printf("ERROR %s() Unexpected packet size %d: %.*s\n", __FUNCTION__, length, length, data);
		return;
	}
	control_packet = (control_packet_s *)data;
	if (control_packet->magic != MAGIC_COMMAND) {
		printf("ERROR %s() Unexpected packet type %d: %.*s\n", __FUNCTION__, control_packet->magic, length, data);
		return;
	}

	server_ctx.height = control_packet->height;
	server_ctx.direction = control_packet->direction;
	server_ctx.gps_latitude = control_packet->gps_latitude;
	server_ctx.gps_longitude = control_packet->gps_longitude;
	server_ctx.slope = control_packet->slope;

	float fps = control_packet->capture_fps;
	if (fps != server_ctx.capture_fps) {
		printf("INFO  %s() Chosen new FPS rate=%f\n", __FUNCTION__, fps);
		server_ctx.capture_fps = fps;
		if (server_ctx.capture_fps > 0) {
			unsigned int timeout = 1000. / server_ctx.capture_fps;
			add_timer(timeout, &server_ctx.send_frame_timer);
		}
	}
}

void destroy_connection(int signum)
{
	printf("INFO  %s() Destroy connection. signum=%d\n", __FUNCTION__, signum);
	release_camera(signum);
	icedemo_destroy_instance(signum);
	release_leds(signum);
	exit(0);
}

int main(int argc, char *argv[])
{
	unsigned char frame[MAX_JPEG_IMAGE_SIZE];
	int size;
	status_packet_s status_packet;

	status_packet.magic = MAGIC_STATUS;
	memset(&server_ctx, 0x00, sizeof(server_ctx_s));
	server_ctx.capture_fps = 2.;
	timerclear(&server_ctx.send_frame_timer);
	timerclear(&server_ctx.send_status_timer);

    // Register signal and signal handler
    signal(SIGINT, destroy_connection);

    if (init_gpio() != 0)
    	return 1;

	if (init_camera() != 0)
		server_ctx.camera_broken = 1;

    if (start_connecting(SIDE_SERVER) != 0)
    	return 1;

    printf("INFO  %s() Enter in Main LOOP\n", __FUNCTION__);
	while (1) {
		if (server_ctx.connection_established == 1) {
			/*	Send captured frame	*/
			if (is_timer_expired(&server_ctx.send_frame_timer)) {
				if (is_capture_aborted() != 0) {
					server_ctx.camera_broken = 1;
					memset(status_packet.info, 0x00, sizeof(status_packet.info));
					sprintf(status_packet.info, "Camera is broken");
				} else if (server_ctx.capture_fps > 0) {
					get_frame(frame, &size);
					send_picture(frame, size);
					unsigned int timeout = 1000. / server_ctx.capture_fps;
					add_timer(timeout, &server_ctx.send_frame_timer);
				}
			}
			/*	Send status packet	*/
			if (is_timer_expired(&server_ctx.send_status_timer)) {
				status_packet.height = server_ctx.height;
				status_packet.direction = server_ctx.direction;
				status_packet.gps_latitude = server_ctx.gps_latitude;
				status_packet.gps_longitude = server_ctx.gps_longitude;
				status_packet.slope = server_ctx.slope;
				status_packet.battery_charge = 100;
				send_data(1, (unsigned char *)&status_packet, sizeof(status_packet_s));
				add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
			}
		}
		usleep(100 * 1000);
	}
    printf("INFO  %s() Exit from Main LOOP\n", __FUNCTION__);

	destroy_connection(0);

	return 0;
}



