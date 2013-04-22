/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "../connection.h"
#include "camera.h"

static int connection_established = 0;
static int camera_broken = 0;

void start_communicate()
{
	printf("==============Connection established\n");
	connection_established = 1;
}

void data_rx(char *data, unsigned int length)
{
	printf("==============Recieved: %.*s\n", length, data);
}

void destroy_connection(int signum)
{
	printf("INFO  %s() Destroy connection. signum=%d\n", __FUNCTION__, signum);
	release_camera(signum);
	icedemo_destroy_instance(signum);
}

int main(int argc, char *argv[])
{
	unsigned char frame[MAX_JPEG_IMAGE_SIZE];
	int size;

    // Register signal and signal handler
    signal(SIGINT, destroy_connection);

	if (init_camera() != 0)
		return 1;

    if (start_connecting(SIDE_SERVER) != 0)
    	return 1;

	printf("==============Main LOOP\n");
	while (1) {
		if (connection_established == 1) {
			send_data(1, "Client, where are you?");
			if (is_capture_aborted() != 0) {
				get_frame(frame, &size);
				camera_broken = 0;
			} else {
				camera_broken = 1;
			}
		}
		sleep(1);
	}
	printf("==============Exit from Main LOOP\n");

	destroy_connection(0);

	return 0;
}



