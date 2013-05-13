/*
 * control.h
 */
#ifndef CONTROL_H_
#define CONTROL_H_

void init_controls();
void poll_keys(int delay);
void send_command();

float get_fps();

#endif /* CONTROL_H_ */
