/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "../connection.h"
#include "../packet.h"
#include "control.h"
#include "display.h"

static int connection_established = 0;

void start_communicate()
{
	printf("INFO  %s() Connection is established.\n", __FUNCTION__);
	connection_established = 1;
}

void data_rx(unsigned char *data, unsigned int length)
{
	status_packet_s *status_packet;

	if (length != sizeof(status_packet_s)) {
		printf("ERROR %s() Unexpected packet size %d: %.*s\n", __FUNCTION__, length, length, data);
		return;
	}
	status_packet = (status_packet_s *)data;
	if (status_packet->magic != MAGIC_STATUS) {
		printf("ERROR %s() Unexpected packet type %d: %.*s\n", __FUNCTION__, status_packet->magic, length, data);
		return;
	}

	printf("INFO  %s() height=%d, direction=%d, gps_latitude=%f, gps_longitude=%f, "
			"slope=%d, battery_charge=%d, info=%s.\n", __FUNCTION__,
			status_packet->height, status_packet->direction, status_packet->gps_latitude,
			status_packet->gps_longitude, status_packet->slope, status_packet->battery_charge, status_packet->info);
}

void picture_rx(unsigned char *data, unsigned int length)
{
	printf("INFO  %s() Picture has just received.\n", __FUNCTION__);
	char filename[] = "captured.jpg";
	FILE *file = fopen(filename, "wb");
	fwrite(data, 1, length, file);
	fclose(file);
	display_frame(data, length);
}

void destroy_connection(int signum)
{
	printf("INFO  %s() Destroy connection. signum=%d\n", __FUNCTION__, signum);
	release_display();
	icedemo_destroy_instance(signum);
}

int main(int argc, char *argv[])
{
    // Register signal and signal handler
    signal(SIGINT, destroy_connection);

    if (init_display() != 0)
    	goto __DESTROY_APP__;

    init_controls();

    if (start_connecting(SIDE_CLIENT) != 0)
    	goto __DESTROY_APP__;

	printf("INFO  %s() Enter in Main LOOP.\n", __FUNCTION__);
	while (1) {
		if (connection_established == 1) {
			//	Poll every 100ms
			poll_keys(100);
			send_command();
		} else {
			sleep(1);
		}
	}
	printf("INFO  %s() Exit from Main LOOP.\n", __FUNCTION__);

__DESTROY_APP__:
	destroy_connection(0);

	return 0;
}
