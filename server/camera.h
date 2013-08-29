/*
 * camera.h
 */

#ifndef CAMERA_H_
#define CAMERA_H_

#define MAX_JPEG_IMAGE_SIZE 1000000

void release_camera(int signum);
void update_camera_settings(struct camera_settings *settings);

#endif /* CAMERA_H_ */
