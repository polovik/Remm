/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "../connection.h"
#include "../packet.h"

static int connection_established = 0;

void start_communicate()
{
	printf("==============Connection established\n");
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
}

int main(int argc, char *argv[])
{
	control_packet_s control_packet;
	control_packet.magic = MAGIC_COMMAND;

    // Register signal and signal handler
    signal(SIGINT, icedemo_destroy_instance);

    if (start_connecting(SIDE_CLIENT) != 0)
    	return 1;

	printf("==============Main LOOP\n");
	while (1) {
		if (connection_established == 1) {
			control_packet.height = 0;
			control_packet.direction = 0;
			control_packet.gps_latitude = 16.22;
			control_packet.gps_longitude = 15.44;
			control_packet.slope = 0;
			control_packet.command = AUTOPILOT_OFF;
			send_data(1, (unsigned char *)&control_packet, sizeof(control_packet_s));
		}
		sleep(1);
	}
	printf("==============Exit from Main LOOP\n");

	icedemo_destroy_instance(0);

	return 0;
}
