/*
 * camera.h
 */

#ifndef CAMERA_H_
#define CAMERA_H_

#define MAX_JPEG_IMAGE_SIZE	1000000

void get_frame(unsigned char frame[MAX_JPEG_IMAGE_SIZE], int *size);
int is_capture_aborted();
void stop_capturing();

void grab_picture();
int init_camera(unsigned int width, unsigned int height);
void release_camera(int signum);

#endif /* CAMERA_H_ */
