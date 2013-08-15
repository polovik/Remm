/*
 * main.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <pthread.h>
#include "../packet.h"
#include "camera.h"
#include "gpio.h"
#include "utils.h"
#include "gps.h"
#include "i2c.h"
#include "bmp085.h"
#include "hmc5883l.h"

#define STATUS_PACKET_TIMEOUT	500

typedef struct {
	int connection_established;
	int camera_broken;
	struct timeval send_status_timer;
	/*	Position	*/
	int height;		/**<	Height above ground */
	int direction; /**<		Direction accordingly to North Pole	*/
	int slope;		/**<	Slope accordingly to horizontal positions */
} server_ctx_s;

static server_ctx_s server_ctx;
static pthread_t command_rx_thread;
static int exit_thread = 0;
static struct sockaddr_in6 client_udp_ipv6_addr;
static int udp_ipv6_socket = -1;
static struct sockaddr_in client_udp_ipv4_addr;
static int udp_ipv4_socket = -1;

void *command_rx(void *arg)
{
	control_packet_s *control_packet;
	struct sockaddr_in6 udp_ipv6_addr, remote_udp_ipv6_addr;
	struct sockaddr_in udp_ipv4_addr, remote_udp_ipv4_addr;
	socklen_t addr_len;
	int bytes_read;
	unsigned char data[1024];
	int new_client = 0;

	/*	Create and init IPv6 socket	*/
    udp_ipv6_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_ipv6_socket < 0) {
        perror("Creating UDP IPv6 socket");
        exit_thread = 1;	//	Exit from program
        return 0;
    }
    memset(&udp_ipv6_addr, 0x00, sizeof(udp_ipv6_addr));
    udp_ipv6_addr.sin6_family = AF_INET6;
    udp_ipv6_addr.sin6_port = htons(9512);
    udp_ipv6_addr.sin6_addr = in6addr_any;
    if (bind(udp_ipv6_socket, (struct sockaddr *)&udp_ipv6_addr, sizeof(udp_ipv6_addr)) < 0) {
        perror("Binding UDP IPv6 socket");
        exit_thread = 1;	//	Exit from program
        return 0;
    }

	/*	Create and init IPv4 socket	*/
    udp_ipv4_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_ipv4_socket < 0) {
        perror("Creating UDP IPv4 socket");
        exit_thread = 1;	//	Exit from program
        return 0;
    }
    memset(&udp_ipv4_addr, 0x00, sizeof(udp_ipv4_addr));
    udp_ipv4_addr.sin_family = AF_INET;
    udp_ipv4_addr.sin_port = htons(9512);
    udp_ipv4_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(udp_ipv4_socket, (struct sockaddr *)&udp_ipv4_addr, sizeof(udp_ipv4_addr)) < 0) {
        perror("Binding UDP IPv4 socket");
//        exit_thread = 1;	//	Exit from program
//        return 0;
        udp_ipv4_socket = -1;
    }

    while (exit_thread == 0) {
		usleep(1000 * 1000);

		/*	Check IPv6 pending datagram	*/
		if (udp_ipv6_socket <= 0)
			goto ipv4_check;
		addr_len = sizeof(remote_udp_ipv6_addr);
		memset(data, 0x00, sizeof(data));
		errno = 0;
		//	TODO Add correct working with socket - add mutex and move to separate thread
		bytes_read = recvfrom(udp_ipv6_socket, data, sizeof(data), MSG_DONTWAIT,
							  (struct sockaddr *)&remote_udp_ipv6_addr, &addr_len);
		if (bytes_read <= 0) {
			if (errno == EAGAIN) {
//				printf("INFO  %s() Avoid blocking\n", __FUNCTION__);
				goto ipv4_check;
			} else {
				perror("Receive data from UDP IPv6 socket");
				continue;
			}
		}

		set_rgb_led_mode(RGB_GREEN_SINGLE_SHOT);
		if (bytes_read != sizeof(control_packet_s)) {
			printf("ERROR %s() Unexpected packet size %d: %.*s\n", __FUNCTION__, bytes_read, bytes_read, data);
			continue;
		}
		control_packet = (control_packet_s *)data;
		if (control_packet->magic != MAGIC_COMMAND) {
			printf("ERROR %s() Unexpected packet type %d: %.*s\n", __FUNCTION__, control_packet->magic, bytes_read, data);
			continue;
		}

		if (addr_len != sizeof(remote_udp_ipv6_addr)) {
			printf("ERROR %s() Received command non-IPv6 net\n", __FUNCTION__);
			continue;
		}

		new_client = memcmp(&remote_udp_ipv6_addr.sin6_addr, &client_udp_ipv6_addr.sin6_addr, sizeof(client_udp_ipv6_addr.sin6_addr)) != 0;
		new_client |= memcmp(&remote_udp_ipv6_addr.sin6_port, &client_udp_ipv6_addr.sin6_port, sizeof(client_udp_ipv6_addr.sin6_port)) != 0;
		if (new_client != 0) {
			printf("INFO  %s() Connection IPv6 established\n", __FUNCTION__);
			server_ctx.connection_established = 1;
			memcpy(&client_udp_ipv6_addr, &remote_udp_ipv6_addr, sizeof(client_udp_ipv6_addr));
			add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
		}
		goto do_cmd;

ipv4_check:
		if (udp_ipv4_socket <= 0)
			continue;
		addr_len = sizeof(remote_udp_ipv4_addr);
		memset(data, 0x00, sizeof(data));
		errno = 0;
		//	TODO Add correct working with socket - add mutex and move to separate thread
		bytes_read = recvfrom(udp_ipv4_socket, data, sizeof(data), MSG_DONTWAIT,
							  (struct sockaddr *)&remote_udp_ipv4_addr, &addr_len);
		if (bytes_read <= 0) {
			if (errno == EAGAIN) {
				continue;
			} else {
				perror("Receive data from UDP IPv4 socket");
				continue;
			}
		}

		set_rgb_led_mode(RGB_GREEN_SINGLE_SHOT);
		if (bytes_read != sizeof(control_packet_s)) {
			printf("ERROR %s() Unexpected packet size %d: %.*s\n", __FUNCTION__, bytes_read, bytes_read, data);
			continue;
		}
		control_packet = (control_packet_s *)data;
		if (control_packet->magic != MAGIC_COMMAND) {
			printf("ERROR %s() Unexpected packet type %d: %.*s\n", __FUNCTION__, control_packet->magic, bytes_read, data);
			continue;
		}

		if (addr_len != sizeof(remote_udp_ipv4_addr)) {
			printf("ERROR %s() Received command non-IPv4 net\n", __FUNCTION__);
			continue;
		}

		new_client = memcmp(&remote_udp_ipv4_addr.sin_addr.s_addr, &client_udp_ipv4_addr.sin_addr.s_addr, sizeof(client_udp_ipv4_addr.sin_addr.s_addr)) != 0;
		new_client |= memcmp(&remote_udp_ipv4_addr.sin_port, &client_udp_ipv4_addr.sin_port, sizeof(client_udp_ipv4_addr.sin_port)) != 0;
		if (new_client != 0) {
			printf("INFO  %s() Connection IPv4 established\n", __FUNCTION__);
			server_ctx.connection_established = 1;
			memcpy(&client_udp_ipv4_addr, &remote_udp_ipv4_addr, sizeof(client_udp_ipv4_addr));
			add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
		}

do_cmd:
		server_ctx.height = control_packet->height;
		server_ctx.direction = control_packet->direction;
		server_ctx.slope = control_packet->slope;
        update_camera_settings(&control_packet->camera);
    }
	printf("INFO  %s() Receiving commands is finished.\n", __FUNCTION__);
    exit_thread = 1;	//	Exit from program
    return 0;
}

static void send_datagram(void *data, size_t length)
{
	int sended_bytes;

	errno = 0;
	//	TODO Add correct working with socket - add mutex and move to separate thread
	if (udp_ipv6_socket > 0) {
		sended_bytes = sendto(udp_ipv6_socket, data, length, 0,
					   (struct sockaddr *)&client_udp_ipv6_addr, sizeof(client_udp_ipv6_addr));
	} else if (udp_ipv4_socket > 0) {
		sended_bytes = sendto(udp_ipv4_socket, data, length, 0,
					   (struct sockaddr *)&client_udp_ipv4_addr, sizeof(client_udp_ipv4_addr));
	} else {
		printf("ERROR %s() All UDP sockets are invalidated\n", __FUNCTION__);
	}

	if (sended_bytes < 0) {
		if (errno == EAGAIN) {
			printf("INFO  %s() Avoid blocking sending status through UDP socket\n", __FUNCTION__);
		} else {
			perror("Sending status through UDP socket");
		}
	}
}

void send_picture(unsigned char *picture, unsigned int length)
{
	static unsigned int picture_id = 0;
	unsigned int i = 0;
	unsigned int fragments_count;
	unsigned int last_fragment_size;
	picture_packet_s packet;

	printf("INFO  %s() Send picture. Size = %d.\n", __FUNCTION__, length);
//	printf("%d - %d - %d\n", sizeof(picture_packet_s), sizeof(raw_packet), sizeof(char *));

	memset(&packet, 0x00, sizeof(picture_packet_s));
	packet.magic = MAGIC_PICTURE;
	packet.picture_id = picture_id;
	packet.picture_size = length;
	fragments_count = length / FRAGMENT_SIZE;

	for (i = 0; i < fragments_count; i++) {
		packet.fragment_id = i;
		packet.fragment_size = FRAGMENT_SIZE;
		memcpy(packet.data, &picture[i * FRAGMENT_SIZE], FRAGMENT_SIZE);

		send_datagram(&packet, sizeof(picture_packet_s));
	}

	last_fragment_size = length - fragments_count * FRAGMENT_SIZE;
//	printf("%d - %d - %d\n", length, fragments_count, last_fragment_size);
	if (last_fragment_size != 0) {
		packet.fragment_id = i;
		packet.fragment_size = last_fragment_size;
		memcpy(packet.data, &picture[i * FRAGMENT_SIZE], last_fragment_size);

		send_datagram(&packet, sizeof(picture_packet_s));
	}

	printf("INFO  %s() Picture %d has sent.\n", __FUNCTION__, picture_id);
	picture_id++;
}

void destroy_connection(int signum)
{
	printf("INFO  %s() Destroy connection. signum=%d\n", __FUNCTION__, signum);
	release_camera(signum);
	release_leds(signum);
	release_gps(signum);
	release_bmp085(signum);
	exit_thread = 1;
	if (pthread_join(command_rx_thread, NULL) != 0) {
		perror("Joining to command_rx_thread");
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	status_packet_s status_packet;
	int ret;
	float pressure;
	float temperature;
	float altitude;
	int raw_temperature_reg;

	status_packet.magic = MAGIC_STATUS;
	memset(&server_ctx, 0x00, sizeof(server_ctx_s));
	timerclear(&server_ctx.send_status_timer);

    // Register signal and signal handler
    signal(SIGINT, destroy_connection);

    if (init_gpio() != 0)
    	return 1;

	init_gps();

    if (init_i2c() == 0) {
        axes_t axes;
        init_hmc5883l(5, 0, HMC5883L_MODE_CONTINUOUS_MEASUREMENT);
        while (1) {
            get_axes(&axes);
            get_heading(axes);
            usleep(500000);
        }
        exit(0);

        init_bmp085(BMP085_MODE_ULTRAHIGHRES);
        temperature = get_temperature(&raw_temperature_reg);
        pressure = get_pressure(raw_temperature_reg);
        altitude = get_altitude(pressure, temperature);
        printf("INFO  %s() Temperature = %f C\n", __FUNCTION__, temperature);
        printf("INFO  %s() Pressure = %f Pa\n", __FUNCTION__, pressure);
        printf("INFO  %s() Altitude = %f m\n", __FUNCTION__, altitude);
    }

	exit_thread = 0;
	ret = pthread_create(&command_rx_thread, NULL, command_rx, NULL);
	if (ret != 0) {
		perror("Starting command receiving thread");
		destroy_connection(22);
	}

    printf("INFO  %s() Enter in Main LOOP\n", __FUNCTION__);
	set_rgb_led_mode(RGB_RED_BLINKING);
	while (1) {
		usleep(100 * 1000);
		if (exit_thread == 1)
			break;

		if (server_ctx.connection_established != 1)
			continue;

		/*	Send status packet	*/
		if (is_timer_expired(&server_ctx.send_status_timer)) {
			double lat = 0.0;
			double lon = 0.0;
			status_packet.height = server_ctx.height;
			status_packet.direction = server_ctx.direction;
			if (get_gps_pos(&lat, &lon)) {
				status_packet.gps_latitude = lat;
				status_packet.gps_longitude = lon;
			}
			status_packet.slope = server_ctx.slope;
			status_packet.battery_charge = get_battery_charge();

			send_datagram(&status_packet, sizeof(status_packet_s));
			add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
		}
	}
    printf("INFO  %s() Exit from Main LOOP\n", __FUNCTION__);

	destroy_connection(0);

	return 0;
}

