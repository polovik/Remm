/*
 * connection.h
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

typedef enum {
	SIDE_SERVER	= 0,
	SIDE_CLIENT	= 1
} host_side_e;

void icedemo_destroy_instance(int signum);
int start_connecting(host_side_e side);
void send_data(unsigned comp_id, const unsigned char *data, unsigned int length);
void send_picture(unsigned char *picture, unsigned int length);

#endif /* CONNECTION_H_ */
