/*
 * display.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "display.h"

static unsigned char *picture_data;
static unsigned int picture_length;
static pthread_mutex_t display_mutex;

int init_display()
{
	picture_data = NULL;
	picture_length = 0;
	if (pthread_mutex_init(&display_mutex, NULL) != 0) {
		perror("Creating mutex on display frame");
		return -1;
	}

	if (cvNamedWindow(DISPLAY_WINDOW_NAME, CV_WINDOW_AUTOSIZE) != 1) {
		printf("ERROR %s() Can't create window \"%s\".\n", __FUNCTION__, DISPLAY_WINDOW_NAME);
		return -1;
	}

	IplImage *image = cvLoadImage("background.jpg", CV_LOAD_IMAGE_COLOR);
	cvShowImage(DISPLAY_WINDOW_NAME, image);
	cvReleaseImage(&image);

	return 0;
}

/**	This procedure is called from PJSIP thread, so showing picture isn't possible now (OpenCV restriction)
 */
void picture_rx(unsigned char *data, unsigned int length)
{
	printf("INFO  %s() Picture has just received.\n", __FUNCTION__);

	if ((length == 0) || (data == NULL)) {
		printf("ERROR %s() Incorrect arguments data=0x%X, length=%d\n", __FUNCTION__, (unsigned int)data, length);
		return;
	}

	char filename[] = "captured.jpg";
	FILE *file = fopen(filename, "wb");
	fwrite(data, 1, length, file);
	fclose(file);

	pthread_mutex_lock(&display_mutex);
	picture_length = length;
	if (picture_data != NULL)
		free(picture_data);
	picture_data = malloc(picture_length);
	memcpy(picture_data, data, picture_length);
	pthread_mutex_unlock(&display_mutex);
}

void display_frame()
{
	IplImage *image;
	CvMat encodedMat;

	pthread_mutex_lock(&display_mutex);
	if ((picture_length == 0) || (picture_data == NULL)) {
		pthread_mutex_unlock(&display_mutex);
		return;
	}

	encodedMat = cvMat(1, picture_length, CV_8UC1, picture_data);
	image = cvDecodeImage(&encodedMat, CV_LOAD_IMAGE_COLOR);
	if (image == NULL) {
		printf("ERROR %s() Can't decode frame.\n", __FUNCTION__);
		pthread_mutex_unlock(&display_mutex);
		return;
	}

	cvShowImage(DISPLAY_WINDOW_NAME, image);
	cvReleaseImage(&image);

	pthread_mutex_unlock(&display_mutex);
}

void release_display()
{
	cvDestroyWindow(DISPLAY_WINDOW_NAME);
	if (pthread_mutex_destroy(&display_mutex) != 0) {
		perror("Destroying mutex display_mutex");
	}
}
