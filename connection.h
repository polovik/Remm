/*
 * connection.h
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

void icedemo_destroy_instance(int signum);
int start_connecting();
void send_data(unsigned comp_id, const char *data);


#endif /* CONNECTION_H_ */
