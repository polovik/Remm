/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "../connection.h"

static int connection_established = 0;

void start_communicate()
{
	printf("==============Connection established\n");
	connection_established = 1;
}

void data_rx(char *data, unsigned int length)
{
	printf("==============Recieved: %.*s\n", length, data);
}

int main(int argc, char *argv[]) {

    // Register signal and signal handler
    signal(SIGINT, icedemo_destroy_instance);

    if (start_connecting() != 0)
    	return 1;

	printf("==============Main LOOP\n");
	while (1) {
		if (connection_established == 1) {
			send_data(1, "Server, where are you?");
		}
		sleep(1);
	}
	printf("==============Exit from Main LOOP\n");

	icedemo_destroy_instance(0);

	return 0;
}
