/*
 * camera.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <libv4l2.h>
#include <jpeglib.h>
#include "camera.h"
#include "../packet.h"
#include "utils.h"

#define CAMERA_DEV_NAME	 "/dev/video0"

typedef struct {
    void   *start;
    size_t length;
} buffer_s;

typedef struct {
	int fd;		//	File descriptor to CAMERA_DEV_NAME
	buffer_s *buffers;	//	MMAP buffers
	unsigned int buffers_count;	//	total MMAP buffers count
	int thread_aborted;
	pthread_t grabbing_thread;
    struct timeval send_frame_timer;
} camera_ctx_s;

static camera_ctx_s camera_ctx;
static camera_settings_s cur_settings;

/**	Wrap ioctl procedure to V4L2 logic.
 *  Return 1 on success, otherwise return 0.
 */
static int xioctl(int request, void *arg)
{
    int ret;

    if (camera_ctx.fd <= 0) {
    	printf("ERROR %s() Camera file descriptor is wrong(%d)\n", __FUNCTION__, camera_ctx.fd);
    	return 0;
    }

    do {
    	errno = 0;
        ret = v4l2_ioctl(camera_ctx.fd, request, arg);
        if ((ret == -1) && (errno == EINTR))
        	printf("ERROR %s() V4L2 ioctl(0x%08X) EINTR has been caught\n", __FUNCTION__, request);
		if ((ret == -1) && (errno == EAGAIN))
    		printf("ERROR %s() V4L2 ioctl(0x%08X) EAGAIN has been caught\n", __FUNCTION__, request);
    } while ((ret == -1) && ((errno == EINTR) || (errno == EAGAIN)));

    if (ret == -1) {
    	printf("ERROR %s() V4L2 ioctl(0x%08X) error %d, %s\n", __FUNCTION__,
    			request, errno, strerror(errno));
        ret = 0;
    } else
        ret = 1;

    return ret;
}

/**	Check V4L2 control accessible.
 *  Return 1 on success, otherwise return 0.
 */
static int is_v4l2_control(int control, struct v4l2_queryctrl *queryctrl)
{
    int err = 0;

    queryctrl->id = control;
    err = xioctl(VIDIOC_QUERYCTRL, queryctrl);
    if (err != 1) {
    	printf("ERROR %s() Can't query %d control\n", __FUNCTION__, control);
        return -1;
    }
    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED) {
    	printf("ERROR %s() Control %s is disabled\n", __FUNCTION__, (char *)queryctrl->name);
        return -1;
    }

    printf("INFO  %s() flags: 0x%X, type: %d, minimum: %d, maximum: %d, default: %d, name: %s\n",
    		__FUNCTION__, queryctrl->flags, queryctrl->type, queryctrl->minimum,
    		queryctrl->maximum, queryctrl->default_value, (char *)queryctrl->name);

    if (queryctrl->type == V4L2_CTRL_TYPE_MENU) {
        struct v4l2_querymenu query_menu;
        int i;
        query_menu.id = control;
        for (i = queryctrl->minimum; i <= queryctrl->maximum; i++) {
        	query_menu.index = i;
            err = xioctl(VIDIOC_QUERYMENU, &query_menu);
            if (err != 1) {
            	printf("ERROR %s() Can't query %d menu for %s\n",
            			__FUNCTION__, control, (char *)queryctrl->name);
            	continue;
            }
            printf("INFO  %s() Menu item %d (%s) for %s\n", __FUNCTION__, i,
            		(char *)query_menu.name, (char *)queryctrl->name);
        }
    }
    return 0;
}

static int get_v4l2_Control(int control)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control_s;
    int err;

    if (is_v4l2_control(control, &queryctrl) != 1)
        return -1;
    control_s.id = control;
    err = xioctl(VIDIOC_G_CTRL, &control_s);
    if (err != 1) {
    	printf("ERROR %s() Can't get %d control\n", __FUNCTION__, control);
        return -1;
    }
    printf("INFO  %s() value: %d, name: %s", __FUNCTION__, control_s.value, (char *)queryctrl.name);
    return control_s.value;
}

void send_frame(unsigned char *raw_frame, unsigned int size)
{
    extern void send_picture(unsigned char *picture, unsigned int length);
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    unsigned char *jpeg_mem = NULL;
    unsigned long jpeg_size = 0;

    /*  Skip some frames for holding constant FPS  */
    if (cur_settings.fps < 0.1) {
        return;
    }
    if (!is_timer_expired(&camera_ctx.send_frame_timer))
        return;

	/*	Init Jpeg compression to memory	*/
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo, &jpeg_mem, &jpeg_size);

    cinfo.image_width = cur_settings.width;
    cinfo.image_height = cur_settings.height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, cur_settings.quality, TRUE);

    /*	Start compression - load image data	*/
    jpeg_start_compress(&cinfo, TRUE);
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &raw_frame[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    /*	Finish compression	*/
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    /*	Send jpeg image	*/
    if (jpeg_size <= MAX_JPEG_IMAGE_SIZE) {
        send_picture(jpeg_mem, jpeg_size);
    } else {
        printf("ERROR %s() Size of encoded image (%ld) exceeded maximum buffer size (%d).\n",
				__FUNCTION__, jpeg_size, MAX_JPEG_IMAGE_SIZE);
    }

    unsigned int timeout = 1000. / cur_settings.fps;
    add_timer(timeout, &camera_ctx.send_frame_timer);

    /*	Release resources	*/
    free(jpeg_mem);
}

void *grab_pictures(void *arg)
{
    struct v4l2_buffer mmap_buf;
    enum v4l2_buf_type type;
    fd_set fds;
    struct timeval tv;
    int ret;
    unsigned int max_raw_frame_size;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(VIDIOC_STREAMON, &type);
    if (ret != 1) {
    	printf("ERROR %s() Can't start video streaming.\n", __FUNCTION__);
    	camera_ctx.thread_aborted = 1;
        return 0;
    }
    printf("INFO  %s() Start capturing.\n", __FUNCTION__);

	while (camera_ctx.thread_aborted == 0) {
        /* Perform select with timeout 1sec */
        do {
            memset(&fds, 0x00, sizeof(fds));
            FD_SET(camera_ctx.fd, &fds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            errno = 0;
            ret = select(camera_ctx.fd + 1, &fds, NULL, NULL, &tv);
            if ((ret == -1) && (errno == EINTR))
            	printf("ERROR %s() V4L2 select EINTR has been caught\n", __FUNCTION__);
        } while ((ret == -1) && (errno == EINTR));
        if (ret == -1) {
            perror("Camera select");
            printf("ERROR %s() Can't get access to camera\n", __FUNCTION__);
            break;
        }

        /*	Retreive frame	*/
        memset(&mmap_buf, 0x00, sizeof(mmap_buf));
        mmap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mmap_buf.memory = V4L2_MEMORY_MMAP;
        ret = xioctl(VIDIOC_DQBUF, &mmap_buf);
        if (ret != 1) {
            printf("ERROR %s() Can't query MMAP buffer.\n", __FUNCTION__);
            break;
        }

        max_raw_frame_size = cur_settings.width * cur_settings.height * 3;
		if (mmap_buf.bytesused != max_raw_frame_size) {
			printf("ERROR %s()Incorrect size of MMAP buffer = %d. Should be %d\n",
					__FUNCTION__, mmap_buf.bytesused, max_raw_frame_size);
			break;
		}
        /*  Try to send frame */
        send_frame((unsigned char *)camera_ctx.buffers[mmap_buf.index].start, mmap_buf.bytesused);

		/*	Release MMAP buffer	*/
        ret = xioctl(VIDIOC_QBUF, &mmap_buf);
        if (ret != 1) {
        	printf("ERROR %s() Can't release MMAP buffer.\n", __FUNCTION__);
        	break;
        }
	}
	printf("INFO  %s() Capturing is finished.\n", __FUNCTION__);
	camera_ctx.thread_aborted = 1;
	return 0;
}

int is_capture_aborted()
{
	return camera_ctx.thread_aborted;
}

/** Open /dev/video0 camera device. Configure it.
 *  Return 1 on successful initialization, otherwise return 0
 */
int init_camera(unsigned int width, unsigned int height)
{
	int ret;
	unsigned int i;
    struct v4l2_format image_format;
    struct v4l2_requestbuffers access_format;
    struct v4l2_buffer mmap_buf;

	memset(&camera_ctx, 0x00, sizeof(camera_ctx));
	camera_ctx.thread_aborted = 1;

    /*  Open camera */
	camera_ctx.fd = v4l2_open(CAMERA_DEV_NAME, O_RDWR | O_NONBLOCK, 0);
    if (camera_ctx.fd < 0) {
        perror("Open camera");
        printf("ERROR %s() Can't open camera %s.\n", __FUNCTION__, CAMERA_DEV_NAME);
        return 0;
    }

    /*  Set frames format   */
	memset(&image_format, 0x00, sizeof(image_format));
    image_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    image_format.fmt.pix.width       = width;
    image_format.fmt.pix.height      = height;
    image_format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    image_format.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    ret = xioctl(VIDIOC_S_FMT, &image_format);
    if (image_format.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
    	printf("ERROR %s() Libv4l didn't accept RGB24 format. Can't proceed.\n", __FUNCTION__);
        return 0;
    }
    if ((image_format.fmt.pix.width != width) || (image_format.fmt.pix.height != height)) {
    	printf("ERROR %s() V4L2 river is sending image only at %dx%d\n", __FUNCTION__,
    			image_format.fmt.pix.width, image_format.fmt.pix.height);
        return 0;
    }
    if (ret != 1)
        return 0;
    printf("INFO  %s() Frame size: %dx%d. Quality = %d\n", __FUNCTION__, width, height, cur_settings.quality);

//    struct v4l2_streamparm capture_param;
//    capture_param.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//    ret = xioctl(VIDIOC_G_PARM, &capture_param);
//    if (ret != 1)
//        return 0;
//
//    if (!(capture_param.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
//    	printf("ERROR %s() Can't set FPS to camera\n", __FUNCTION__);
//        return 0;
//    }
//    printf("INFO  %s() Get FPS: %d/%d\n", __FUNCTION__, capture_param.parm.capture.timeperframe.numerator, capture_param.parm.capture.timeperframe.denominator);
//
//    capture_param.parm.capture.timeperframe.denominator = 1;
//    ret = xioctl(VIDIOC_S_PARM, &capture_param);
//    if (ret != 1)
//        return 0;
//    printf("INFO  %s() Set FPS: %d/%d\n", __FUNCTION__, capture_param.parm.capture.timeperframe.numerator, capture_param.parm.capture.timeperframe.denominator);
//
//	get_v4l2_Control(V4L2_CID_EXPOSURE);
//	get_v4l2_Control(V4L2_CID_EXPOSURE_AUTO);
//	get_v4l2_Control(V4L2_CID_EXPOSURE_ABSOLUTE);
//	get_v4l2_Control(V4L2_CID_EXPOSURE_AUTO_PRIORITY);

    /*  Set MMAP access to camera frames    */
	memset(&access_format, 0x00, sizeof(access_format));
    camera_ctx.buffers_count = 2;
    access_format.count = camera_ctx.buffers_count;
    access_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    access_format.memory = V4L2_MEMORY_MMAP;
    ret = xioctl(VIDIOC_REQBUFS, &access_format);
    if (ret != 1)
        return 0;

    /*  Init MMAP buffers   */
    camera_ctx.buffers = (buffer_s *)calloc(camera_ctx.buffers_count, sizeof(buffer_s));
    for (i = 0; i < camera_ctx.buffers_count; ++i) {
    	memset(&mmap_buf, 0x00, sizeof(mmap_buf));
        mmap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mmap_buf.memory = V4L2_MEMORY_MMAP;
        mmap_buf.index = i;

        ret = xioctl(VIDIOC_QUERYBUF, &mmap_buf);
        if (ret != 1)
            return 0;

        camera_ctx.buffers[i].length = mmap_buf.length;
        camera_ctx.buffers[i].start = v4l2_mmap(NULL, mmap_buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
        										camera_ctx.fd, mmap_buf.m.offset);
        if (MAP_FAILED == camera_ctx.buffers[i].start) {
            perror("V4L2 mmap");
            printf("ERROR %s() V4L2 can't mmap to buffer %d", __FUNCTION__, i);
            return 0;
        }
    }

    for (i = 0; i < camera_ctx.buffers_count; ++i) {
    	memset(&mmap_buf, 0x00, sizeof(mmap_buf));
        mmap_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        mmap_buf.memory = V4L2_MEMORY_MMAP;
        mmap_buf.index = i;
        ret = xioctl(VIDIOC_QBUF, &mmap_buf);
        if (ret != 1)
            return 0;
    }

	ret = pthread_create(&camera_ctx.grabbing_thread, NULL, grab_pictures, NULL);
	if (ret != 0) {
		perror("Starting grabbing thread");
		return 0;
	}

	printf("INFO  %s() Camera is successfully inited.\n", __FUNCTION__);
	camera_ctx.thread_aborted = 0;
	return 1;
}

void release_camera(int signum)
{
    enum v4l2_buf_type type;
    int ret;
    unsigned int i;

	printf("INFO  %s() Release resources. signum=%d\n", __FUNCTION__, signum);
    camera_ctx.thread_aborted = 1;
    timerclear(&camera_ctx.send_frame_timer);
    if (camera_ctx.fd <= 0) {
        printf("INFO  %s() Resources has been already released.\n", __FUNCTION__);
        return;
    }

	if (pthread_join(camera_ctx.grabbing_thread, NULL) != 0) {
		perror("Joining to grabbing_thread");
	}

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = xioctl(VIDIOC_STREAMOFF, &type);
    if (ret != 1) {
    	printf("ERROR %s() Can't turn off camera streaming.\n", __FUNCTION__);
    }

    for (i = 0; i < camera_ctx.buffers_count; ++i)
        v4l2_munmap(camera_ctx.buffers[i].start, camera_ctx.buffers[i].length);
    v4l2_close(camera_ctx.fd);
    camera_ctx.fd = -1;

	printf("INFO  %s() Resources is released.\n", __FUNCTION__);
}

void update_camera_settings(struct camera_settings *settings)
{
    int isCorrect = 0;

    //  TODO add checking exposure settings
    cur_settings.exposure_type = settings->exposure_type;
    cur_settings.exposure_value = settings->exposure_value;

    /*  Do any actions only upon different settings */
    if (memcmp(&cur_settings, settings, sizeof(camera_settings_s)) == 0)
        return;

    /*  Sanity check    */
    isCorrect |= (settings->width == 160) && (settings->height == 120);
    isCorrect |= (settings->width == 320) && (settings->height == 240);
    isCorrect |= (settings->width == 640) && (settings->height == 480);
    isCorrect |= (settings->width == 960) && (settings->height == 720);
    isCorrect &= (settings->quality >= 0) && (settings->quality <= 100);
    isCorrect &= (settings->fps >= 0) && (settings->fps <= 30);
    if (isCorrect == 0) {
        printf("ERROR %s() Incorrect settings. Skip it.\n", __FUNCTION__);
        return;
    }

    if (cur_settings.quality != settings->quality) {
        cur_settings.quality = settings->quality;
    }

    if (cur_settings.width != settings->width) {
        cur_settings.width = settings->width;
        cur_settings.height = settings->height;

        release_camera(30);
        init_camera(cur_settings.width, cur_settings.height);
    }

    if (cur_settings.fps != settings->fps) {
        cur_settings.fps = settings->fps;
        printf("INFO  %s() Chosen new FPS rate=%f\n", __FUNCTION__, settings->fps);

        if (cur_settings.fps > 0) {
            unsigned int timeout = 1000. / cur_settings.fps;
            add_timer(timeout, &camera_ctx.send_frame_timer);
        } else {
            timerclear(&camera_ctx.send_frame_timer);
        }
    }
}
