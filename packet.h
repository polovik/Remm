/*
 * packet.h
 */

#ifndef PACKET_H_
#define PACKET_H_

/*	Unique packet type identification	*/
typedef enum {
	MAGIC_PICTURE	=	0x50494354,
	MAGIC_STATUS	=	0x434F4D4D,
	MAGIC_COMMAND	=	0x53544154
} magic_e;

#define FRAGMENT_SIZE	(512 - 5 * 4)

/*=================================Raspberry Pi===============================*/
/**	Sends from Raspberry Pi host. Contain picture.
 *  Picture size is larger then UDP packet size(512, 548 = 576 - 20 - 8, 8192bytes, or 65507?), so need fragmentation. */
//	http://stackoverflow.com/questions/1098897/what-is-the-largest-safe-udp-packet-size-on-the-internet
//	http://stackoverflow.com/questions/900697/how-to-find-the-largest-udp-packet-i-can-send-without-fragmenting
//	PJSIP_UDP_SIZE_THRESHOLD	1300
#pragma pack(push, 1)
typedef struct {
	magic_e magic;
	int picture_id;
	int picture_size;	//	IplImage::imageSize
	int fragment_id;
	int fragment_size;
	unsigned char data[FRAGMENT_SIZE];	//	piece of IplImage::imageData
} picture_packet_s;

/**	Sends from Raspberry Pi host. Contain status info. */
typedef struct {
	magic_e magic;
	int height;		/**<	Height above ground */
	int direction; /**<		Direction accordingly to North Pole	*/
	double gps_latitude;
	double gps_longitude;
	int slope;		/**<	Slope accordingly to horizontal positions */
	int battery_charge;
	char info[100];
} status_packet_s;

/*=================================PC host====================================*/
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
	magic_e magic;
	int height;
	int direction;
	double gps_latitude;
	double gps_longitude;
	int slope;
	float capture_fps;	/**<	0(turn off camera), [0.1 to 2] step=0.1, (2 to 10] step=1 */
	autopilot_command_e command;
} control_packet_s;
#pragma pack(pop)

#endif /* PACKET_H_ */
