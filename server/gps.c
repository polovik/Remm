/*
 * gps.c
 */
#include <nmea/nmea.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include "gps.h"

static int exit_thread = 0;
static pthread_t gps_thread;
static nmeaINFO gps_info;
static int uart_fd = -1;

void trace(const char *str, int str_size)
{
    printf("Trace: ");
    write(1, str, str_size);
    printf("\n");
}
void error(const char *str, int str_size)
{
    printf("Error: ");
    write(1, str, str_size);
    printf("\n");
}

void *gps_polling(void *arg)
{
    nmeaPARSER parser;
    char buff[2048];
    int size;
    int it = 0;
    nmeaPOS dpos;

//    nmea_property()->trace_func = &trace;
    nmea_property()->error_func = &error;
    nmea_zero_INFO(&gps_info);
    nmea_parser_init(&parser);

    while (exit_thread == 0) {
    	size = (int)read(uart_fd, buff, sizeof(buff));
    	if (size == 0) {
    		sleep(1);
    		continue;
    	}
    	if (errno == EAGAIN) {
    		errno = 0;
    		sleep(1);
    		continue;
    	}
    	if (size < 0) {
            perror("Reading from UART");
            break;
    	}
    	nmea_parse(&parser, buff, size, &gps_info);

        nmea_info2pos(&gps_info, &dpos);

        printf("%03d, Lat: %f, Lon: %f, Sig: %d, Fix: %d\n",
        		it++, dpos.lat, dpos.lon, gps_info.sig, gps_info.fix);
    }

    nmea_parser_destroy(&parser);
	return 0;
}

int init_gps()
{
	int ret;

	uart_fd = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY | O_NONBLOCK);
	if (uart_fd < 0) {
		perror("Open UART");
		return uart_fd;
	}

    /*	Flush uart data	*/
    tcflush(uart_fd, TCIOFLUSH);

	exit_thread = 0;
	ret = pthread_create(&gps_thread, NULL, gps_polling, NULL);
	if (ret != 0) {
		perror("Starting gps polling thread");
		return -1;
	}

	printf("INFO  %s() GPS is successfully inited.\n", __FUNCTION__);

	return 0;
}

void release_gps(int signum)
{
	exit_thread = 1;
	if (pthread_join(gps_thread, NULL) != 0) {
		perror("Joining to gps_polling thread");
	}

	if (uart_fd > 0) {
		if (close(uart_fd) < 0)
			perror("Closing UART");
		uart_fd = -1;
	}

    printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

int get_gps_pos(double *lat, double *lon)
{
	nmeaPOS dpos;
	if (uart_fd <= 0)
		return -1;

	nmea_info2pos(&gps_info, &dpos);
	*lat = dpos.lat;
	*lon = dpos.lon;

	return 1;
}
