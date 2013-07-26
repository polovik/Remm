/*
 * utils.h
 */

#ifndef UTILS_H_
#define UTILS_H_

struct timeval;

void add_timer(unsigned int msec, struct timeval *timer);
int is_timer_expired(struct timeval *timer);

#endif /* UTILS_H_ */
