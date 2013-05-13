/*
 * utils.c
 */
#include <sys/time.h>
#include "utils.h"

void add_timer(unsigned int msec, struct timeval *timer)
{
	struct timeval cur_time, timeout;
	gettimeofday(&cur_time, 0);

	timerclear(timer);
	timeout.tv_sec = msec / 1000;
	timeout.tv_usec = (msec % 1000) * 1000;
//	printf("INFO  %s() Add timer %d:%d.\n", __FUNCTION__, (int)timeout.tv_sec, (int)timeout.tv_usec);
	timeradd(&cur_time, &timeout, timer);
}

/**	Return 1 only if timer was set and it is expired.
 */
int is_timer_expired(struct timeval *timer)
{
	struct timeval cur_time;

	if (!timerisset(timer))
		return 0;

	gettimeofday(&cur_time, 0);

	if (timercmp(&cur_time, timer, <))
		return 0;

	timerclear(timer);
//	printf("INFO  %s() Timer has expired.\n", __FUNCTION__);
	return 1;
}

