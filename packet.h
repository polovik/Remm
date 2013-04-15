/*
 * packet.h
 */

#ifndef PACKET_H_
#define PACKET_H_

/**	Sends from Raspberry Pi host. Contain picture.
 *  Picture size is larger then UDP packet size(512, 548 = 576 - 20 - 8, 8192bytes, or 65507?), so need fragmentation. */
//	http://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet
//	http://stackoverflow.com/questions/900697/how-to-find-the-largest-udp-packet-i-can-send-without-fragmenting
//	PJSIP_UDP_SIZE_THRESHOLD	1300
typedef struct {
	int picture_id;
	int fragment_id;
	int fragment_size;

	int picture_size;	//	IplImage::imageSize
	char data[512 - 4 * 4];	//	piece of IplImage::imageData
} picture_packet_s;

/**	Sends from Raspberry Pi host. Contain status info. */
typedef struct {
	int height;		/**<	Height above ground */
	int direction; /**<		Direction accordingly to North Pole	*/
	float gps_latitude;
	float grp_longitude;
	int slope;		/**<	Slope accordingly to horizontal positions */
	int battery_charge;
	char info[100];
} status_packet_s;

/**	Autopilots commands */
typedef enum {
	AUTOPILOT_OFF		= 0x00,
	AUTOPILOT_LANDING	= 0x03,
	AUTOPILOT_RETURN_TO_BASE	= 0x0F,
	AUTOPILOT_STAY_IN_3G_CELL	= 0x3F,
	AUTOPILOT_KEEP_HEIGHT		= 0xFF
} autopilot_command_e;

/**	Sends from PC host. Contain control commands. */
typedef struct {
	int height;
	int direction;
	float gps_latitude;
	float grp_longitude;
	int slope;
	autopilot_command_e command;
} control_packet_s;

#endif /* PACKET_H_ */
