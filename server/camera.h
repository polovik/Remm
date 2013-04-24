/*
 * camera.h
 */

#ifndef CAMERA_H_
#define CAMERA_H_

#define MAX_JPEG_IMAGE_SIZE	1000000

void get_frame(unsigned char frame[MAX_JPEG_IMAGE_SIZE], int *size);
int is_capture_aborted();
void stop_capturing();

int init_camera();
void release_camera();

#endif /* CAMERA_H_ */