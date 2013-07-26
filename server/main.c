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

#define STATUS_PACKET_TIMEOUT	500

typedef struct {
	int connection_established;
	int camera_broken;
	float capture_fps;
	struct timeval send_frame_timer;
	struct timeval send_status_timer;
	/*	Position	*/
	int height;		/**<	Height above ground */
	int direction; /**<		Direction accordingly to North Pole	*/
	int slope;		/**<	Slope accordingly to horizontal positions */
} server_ctx_s;

static server_ctx_s server_ctx;
static pthread_t command_rx_thread;
static int exit_thread = 0;
static struct sockaddr_in6 client_udp_addr;
static int udp_socket = -1;

void *command_rx(void *arg)
{
	control_packet_s *control_packet;
	struct sockaddr_in6 udp_addr, remote_udp_addr;
	socklen_t addr_len;
	int bytes_read;
	unsigned char data[1024];
	int new_client = 0;

    udp_socket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("Creating UDP socket");
        exit_thread = 1;	//	Exit from program
        return 0;
    }
    memset(&udp_addr, 0x00, sizeof(udp_addr));
    udp_addr.sin6_family = AF_INET6;
    udp_addr.sin6_port = htons(9512);
    udp_addr.sin6_addr = in6addr_any;
    if (bind(udp_socket, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("Binding UDP socket");
        exit_thread = 1;	//	Exit from program
        return 0;
    }

    while (exit_thread == 0) {
		usleep(1000 * 1000);
		addr_len = sizeof(remote_udp_addr);
		errno = 0;
		//	TODO Add correct working with socket - add mutex and move to separate thread
		bytes_read = recvfrom(udp_socket, data, sizeof(data),
							  MSG_DONTWAIT | MSG_TRUNC,
							  (struct sockaddr *)&remote_udp_addr, &addr_len);
		if (bytes_read <= 0) {
			if (errno == EAGAIN) {
				printf("INFO  %s() Avoid blocking\n", __FUNCTION__);
			} else {
				perror("Receive data from UDP socket");
				continue;
			}
		}

		if (bytes_read != sizeof(control_packet_s)) {
			printf("ERROR %s() Unexpected packet size %d: %.*s\n", __FUNCTION__, bytes_read, bytes_read, data);
			continue;
		}
		control_packet = (control_packet_s *)data;
		if (control_packet->magic != MAGIC_COMMAND) {
			printf("ERROR %s() Unexpected packet type %d: %.*s\n", __FUNCTION__, control_packet->magic, bytes_read, data);
			continue;
		}

		if (addr_len != sizeof(remote_udp_addr)) {
			printf("ERROR %s() Received command non-IPv6 net\n", __FUNCTION__);
			continue;
		}

		new_client = memcmp(&remote_udp_addr.sin6_addr, &client_udp_addr.sin6_addr, sizeof(client_udp_addr.sin6_addr)) != 0;
		new_client |= memcmp(&remote_udp_addr.sin6_port, &client_udp_addr.sin6_port, sizeof(client_udp_addr.sin6_port)) != 0;
		if (new_client != 0) {
			printf("INFO  %s() Connection established\n", __FUNCTION__);
			server_ctx.connection_established = 1;
			memcpy(&client_udp_addr.sin6_addr, &remote_udp_addr.sin6_addr, sizeof(client_udp_addr.sin6_addr));
			memcpy(&client_udp_addr.sin6_port, &remote_udp_addr.sin6_port, sizeof(client_udp_addr.sin6_port));
			unsigned int timeout = 1000. / server_ctx.capture_fps;
			add_timer(timeout, &server_ctx.send_frame_timer);
			add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
		}

		server_ctx.height = control_packet->height;
		server_ctx.direction = control_packet->direction;
		server_ctx.slope = control_packet->slope;

		float fps = control_packet->capture_fps;
		if (fps != server_ctx.capture_fps) {
			printf("INFO  %s() Chosen new FPS rate=%f\n", __FUNCTION__, fps);
			server_ctx.capture_fps = fps;
			if (server_ctx.capture_fps > 0) {
				unsigned int timeout = 1000. / server_ctx.capture_fps;
				add_timer(timeout, &server_ctx.send_frame_timer);
			}
		}
    }
	printf("INFO  %s() Receiving commands is finished.\n", __FUNCTION__);
    exit_thread = 1;	//	Exit from program
    return 0;
}

void send_picture(unsigned char *picture, unsigned int length)
{
	static unsigned int picture_id = 0;
	unsigned int i = 0;
	unsigned int fragments_count;
	unsigned int last_fragment_size;
	picture_packet_s packet;
	int sended_bytes;

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

		errno = 0;
		//	TODO Add correct working with socket - add mutex and move to separate thread
		sended_bytes = sendto(udp_socket, &packet, sizeof(picture_packet_s), 0,
					   (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr));
		if (sended_bytes < 0) {
			if (errno == EAGAIN) {
				printf("INFO  %s() Avoid blocking sending status\n", __FUNCTION__);
			} else {
				perror("Sending status UDP socket");
			}
		}
	}

	last_fragment_size = length - fragments_count * FRAGMENT_SIZE;
//	printf("%d - %d - %d\n", length, fragments_count, last_fragment_size);
	if (last_fragment_size != 0) {
		packet.fragment_id = i;
		packet.fragment_size = last_fragment_size;
		memcpy(packet.data, &picture[i * FRAGMENT_SIZE], last_fragment_size);

		errno = 0;
		//	TODO Add correct working with socket - add mutex and move to separate thread
		sended_bytes = sendto(udp_socket, &packet, sizeof(picture_packet_s), 0,
					   (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr));
		if (sended_bytes < 0) {
			if (errno == EAGAIN) {
				printf("INFO  %s() Avoid blocking sending status\n", __FUNCTION__);
			} else {
				perror("Sending status UDP socket");
			}
		}
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
	exit_thread = 1;
	if (pthread_join(command_rx_thread, NULL) != 0) {
		perror("Joining to command_rx_thread");
	}
	exit(0);
}

int main(int argc, char *argv[])
{
	unsigned char frame[MAX_JPEG_IMAGE_SIZE];
	int size;
	status_packet_s status_packet;
	int ret;

	status_packet.magic = MAGIC_STATUS;
	memset(&server_ctx, 0x00, sizeof(server_ctx_s));
	server_ctx.capture_fps = 0.5;
	timerclear(&server_ctx.send_frame_timer);
	timerclear(&server_ctx.send_status_timer);

    // Register signal and signal handler
    signal(SIGINT, destroy_connection);

    if (init_gpio() != 0)
    	return 1;

	init_gps();

	if (init_camera() != 0)
		server_ctx.camera_broken = 1;

	exit_thread = 0;
	ret = pthread_create(&command_rx_thread, NULL, command_rx, NULL);
	if (ret != 0) {
		perror("Starting command receiving thread");
		destroy_connection(22);
	}

    printf("INFO  %s() Enter in Main LOOP\n", __FUNCTION__);
	while (1) {
		usleep(100 * 1000);
		if (exit_thread == 1)
			break;

		if (server_ctx.connection_established != 1)
			continue;

		/*	Send captured frame	*/
		if (is_timer_expired(&server_ctx.send_frame_timer)) {
			if (is_capture_aborted() != 0) {
				server_ctx.camera_broken = 1;
				memset(status_packet.info, 0x00, sizeof(status_packet.info));
				sprintf(status_packet.info, "Camera is broken");
			} else if (server_ctx.capture_fps > 0) {
				get_frame(frame, &size);
				send_picture(frame, size);
				unsigned int timeout = 1000. / server_ctx.capture_fps;
				add_timer(timeout, &server_ctx.send_frame_timer);
			}
		}
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
			errno = 0;
			//	TODO Add correct working with socket - add mutex and move to separate thread
			ret = sendto(udp_socket, &status_packet, sizeof(status_packet_s), MSG_DONTWAIT,
						 (struct sockaddr *)&client_udp_addr, sizeof(client_udp_addr));
			if (ret < 0) {
				if (errno == EAGAIN) {
					printf("INFO  %s() Avoid blocking sending status\n", __FUNCTION__);
				} else {
					perror("Sending status UDP socket");
				}
				continue;
			}
			add_timer(STATUS_PACKET_TIMEOUT, &server_ctx.send_status_timer);
		}
	}
    printf("INFO  %s() Exit from Main LOOP\n", __FUNCTION__);

	destroy_connection(0);

	return 0;
}

