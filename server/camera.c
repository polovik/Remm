/*
 * camera.c
 */

#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_JPEG_IMAGE_SIZE	1000000

typedef struct {
	CvCapture *capture;
	int image_length;
	unsigned char jpeg_image[MAX_JPEG_IMAGE_SIZE];
	int thread_aborted;
	pthread_t grabbing_thread;
	pthread_mutex_t image_copy_mutex;
} camera_ctx_s;

static camera_ctx_s camera_ctx;

void get_frame(unsigned char frame[MAX_JPEG_IMAGE_SIZE], int *size)
{
	pthread_mutex_lock(&camera_ctx.image_copy_mutex);
	*size = camera_ctx.image_length;
	if (*size > MAX_JPEG_IMAGE_SIZE) {
		printf("ERROR %s() Size of encoded image (%d) exceeded maximum buffer size (%d).\n",
				__FUNCTION__, *size, MAX_JPEG_IMAGE_SIZE);
		pthread_mutex_unlock(&camera_ctx.image_copy_mutex);
		return;
	}
	memcpy(frame, camera_ctx.jpeg_image, *size);
	pthread_mutex_unlock(&camera_ctx.image_copy_mutex);

	char filename[] = "image.jpg";
	FILE *file = fopen(filename, "wb");
	fwrite(frame, 1, *size, file);
	fclose(file);
}

void *grab_pictures(void *arg)
{
	IplImage* frame = 0;
	int counter=0;
	struct timeval tv;
	long long elapsed;
	int jpeg_params[] = { CV_IMWRITE_JPEG_QUALITY, 50, 0 };
	CvMat* encodedMat;

	printf("INFO  %s() Start capturing.\n", __FUNCTION__);
	while (camera_ctx.thread_aborted == 0) {
		gettimeofday(&tv, NULL);
		elapsed = tv.tv_sec * 1000000 + tv.tv_usec;
		frame = cvQueryFrame(camera_ctx.capture);
		if (frame == NULL) {
			printf("ERROR %s() Can't query frame from camera.\n", __FUNCTION__);
			break;
		}
		gettimeofday(&tv, NULL);
		elapsed = tv.tv_sec * 1000000 + tv.tv_usec - elapsed;
		printf("INFO  %s() Captured %d. Elapsed %lld us.\n", __FUNCTION__, counter, elapsed);

		encodedMat = cvEncodeImage(".jpeg", frame, jpeg_params);
		if (encodedMat == NULL) {
			printf("ERROR %s() Can't encode frame.\n", __FUNCTION__);
			break;
		}
		if (encodedMat->cols > MAX_JPEG_IMAGE_SIZE) {
			printf("ERROR %s() Size of encoded image (%d) exceeded maximum buffer size (%d).\n",
					__FUNCTION__, encodedMat->cols, MAX_JPEG_IMAGE_SIZE);
			break;
		}

		pthread_mutex_lock(&camera_ctx.image_copy_mutex);
		camera_ctx.image_length = encodedMat->cols;
		memcpy(camera_ctx.jpeg_image, encodedMat->data.ptr, camera_ctx.image_length);
		pthread_mutex_unlock(&camera_ctx.image_copy_mutex);
		cvReleaseMat(&encodedMat);

		counter++;
	}
	printf("INFO  %s() Capturing is finished.\n", __FUNCTION__);
	camera_ctx.thread_aborted = 1;
	return 0;
}

int init_camera()
{
	double width, height;
	int ret;

	memset(&camera_ctx, 0x00, sizeof(camera_ctx));
	camera_ctx.capture = cvCreateCameraCapture(CV_CAP_ANY); //cvCaptureFromCAM( 0 );
	if (camera_ctx.capture == NULL) {
		printf("ERROR %s() Can't open camera.\n", __FUNCTION__);
		return -1;
	}

	cvSetCaptureProperty(camera_ctx.capture, CV_CAP_PROP_FRAME_WIDTH, 1280);	//	640x480, 320x240, 160x120
	cvSetCaptureProperty(camera_ctx.capture, CV_CAP_PROP_FRAME_HEIGHT, 960);
//	cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, 30);

	width = cvGetCaptureProperty(camera_ctx.capture, CV_CAP_PROP_FRAME_WIDTH);
	height = cvGetCaptureProperty(camera_ctx.capture, CV_CAP_PROP_FRAME_HEIGHT);
	printf("INFO  %s() Frame size:%.0f x %.0f.\n", __FUNCTION__, width, height);

	ret = pthread_mutex_init(&camera_ctx.image_copy_mutex, NULL);
	if (ret != 0) {
		perror("Creating mutex on image copy");
		return -1;
	}
	ret = pthread_create(&camera_ctx.grabbing_thread, NULL, grab_pictures, NULL);
	if (ret != 0) {
		perror("Starting grabbing thread");
		return -1;
	}

	return 0;
}

void release_camera()
{
	pthread_mutex_destroy(&camera_ctx.image_copy_mutex);
	if (camera_ctx.capture != NULL)
		cvReleaseCapture(&camera_ctx.capture);
}

int main(int argc, char *argv[])
{
	unsigned char frame[MAX_JPEG_IMAGE_SIZE];
	int size;

	if (init_camera() != 0)
		exit(0);

	while(camera_ctx.thread_aborted == 0) {
		get_frame(frame, &size);
		sleep(1);
	}

	pthread_join(camera_ctx.grabbing_thread, NULL);
	release_camera();
	exit(0);
}
