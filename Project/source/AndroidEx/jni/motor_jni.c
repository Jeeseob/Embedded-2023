#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include "android/log.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>


#define LOG_TAG "jniTag"
#define LOGD(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define FPGA_STEP_MOTOR_DEVICE "/dev/fpga_step_motor"
#define VIDEO_DEVICE "/dev/video2"
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV

jint JNICALL Java_com_example_androidex_MainActivity_moveMotorRight(JNIEnv *env, jobject this){
	int dev;
	unsigned char motor_state[3] = {1, 0, 50};

	LOGD("Start SUCCESS");
	dev = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
	LOGD("DEV : %d", dev);

	if (dev<0) {
		return -1;
	}

	write(dev,motor_state,3);
	close(dev);
	
	return 1;
}

jint JNICALL Java_com_example_androidex_MainActivity_moveMotorLeft(JNIEnv *env, jobject this){

	int dev;
	unsigned char motor_state[3] = {1, 1, 50};

	LOGD("Start SUCCESS");
	dev = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
	LOGD("DEV : %d", dev);

	if (dev<0) {
		return -1;
	}

	write(dev,motor_state,3);
	close(dev);
	
	return 1;
}

jint JNICALL Java_com_example_androidex_MainActivity_stopMotor(JNIEnv *env, jobject this){
	int dev;
	unsigned char motor_state[3] = {0, 0, 0};

	LOGD("Start SUCCESS");
	dev = open(FPGA_STEP_MOTOR_DEVICE, O_WRONLY);
	LOGD("DEV : %d", dev);

	if (dev<0) {
		return -1;
	}

	write(dev,motor_state,3);
	close(dev);
	
	return 1;
}

JNIEXPORT jbyteArray JNICALL Java_com_example_androidex_MainActivity_capture(JNIEnv *env, jobject this){
    int video_fd;
    struct v4l2_format format;
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;

    // Open the video device
    video_fd = open(VIDEO_DEVICE, O_RDWR);
    if (video_fd == -1) {
    	LOGD("Failed to open video device");
        return NULL;
    }

    // Set video format
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = VIDEO_WIDTH;
    format.fmt.pix.height = VIDEO_HEIGHT;
    format.fmt.pix.pixelformat = VIDEO_FORMAT;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(video_fd, VIDIOC_S_FMT, &format) == -1) {
    	LOGD("Failed to set video format");
        close(video_fd);
        return NULL;
    }

    // Request video buffers
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 1;
    if (ioctl(video_fd, VIDIOC_REQBUFS, &reqbuf) == -1) {
    	LOGD("Failed to request video buffers");
        close(video_fd);
        return NULL;
    }

    // Map video buffers
    struct v4l2_buffer buffer_info;
    buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_info.memory = V4L2_MEMORY_MMAP;
    buffer_info.index = 0;
    if (ioctl(video_fd, VIDIOC_QUERYBUF, &buffer_info) == -1) {
    	LOGD("Failed to query video buffer");
        close(video_fd);
        return NULL;
    }

    void *buffer_start = mmap(NULL, buffer_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buffer_info.m.offset);
    if (buffer_start == MAP_FAILED) {
    	LOGD("Failed to map video buffer");
        close(video_fd);
        return NULL;
    }

    // Start video streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type) == -1) {
    	LOGD("Failed to start video streaming");
        close(video_fd);
        return NULL;
    }

    // Capture image
    if (ioctl(video_fd, VIDIOC_QBUF, &buffer_info) == -1) {
    	LOGD("Failed to enqueue video buffer");
        munmap(buffer_start, buffer_info.length);
        close(video_fd);
        return NULL;
    }

    if (ioctl(video_fd, VIDIOC_DQBUF, &buffer_info) == -1) {
        perror("Failed to dequeue video buffer");
        munmap(buffer_start, buffer_info.length);
        close(video_fd);
        return NULL;
    }

	// create byteArray
	jbyteArray bytes = (*env)->NewByteArray(env, buffer_info.length);
	(*env)->SetByteArrayRegion(env, bytes, 0, buffer_info.length, buffer_start);

    // Cleanup
    munmap(buffer_start, buffer_info.length);
    close(video_fd);

    return bytes;
}


