/*
 * display.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "display.h"

int init_display()
{
	if (cvNamedWindow(DISPLAY_WINDOW_NAME, CV_WINDOW_AUTOSIZE) != 1) {
		printf("ERROR %s() Can't create window \"%s\".\n", __FUNCTION__, DISPLAY_WINDOW_NAME);
		return -1;
	}

	IplImage *image = cvLoadImage("background.jpg", CV_LOAD_IMAGE_COLOR);
	cvShowImage(DISPLAY_WINDOW_NAME, image);
	cvReleaseImage(&image);

	return 0;
}

void display_frame(unsigned char *data, unsigned int length)
{
	IplImage *image;
	CvMat encodedMat;

	encodedMat = cvMat(1, length, CV_8UC1, data);
	image = cvDecodeImage(&encodedMat, CV_LOAD_IMAGE_COLOR);
	if (image == NULL) {
		printf("ERROR %s() Can't decode frame.\n", __FUNCTION__);
		return;
	}

	cvShowImage(DISPLAY_WINDOW_NAME, image);

	cvReleaseImage(&image);
}

void release_display()
{
	cvDestroyWindow(DISPLAY_WINDOW_NAME);
}
