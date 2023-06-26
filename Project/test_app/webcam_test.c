#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define VIDEO_DEVICE "/dev/video2"
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define VIDEO_FILE "captured_video.yuv"

int main() {
    int video_fd, file_fd;
    struct v4l2_format format;
    struct v4l2_requestbuffers reqbuf;
    struct v4l2_buffer buf;

    // Open the video device
    video_fd = open(VIDEO_DEVICE, O_RDWR);
    if (video_fd == -1) {
        perror("Failed to open video device");
        return -1;
    }

    // Set video format
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = VIDEO_WIDTH;
    format.fmt.pix.height = VIDEO_HEIGHT;
    format.fmt.pix.pixelformat = VIDEO_FORMAT;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(video_fd, VIDIOC_S_FMT, &format) == -1) {
        perror("Failed to set video format");
        close(video_fd);
        return -1;
    }

    // Request video buffers
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 1;
    if (ioctl(video_fd, VIDIOC_REQBUFS, &reqbuf) == -1) {
        perror("Failed to request video buffers");
        close(video_fd);
        return -1;
    }

    // Map video buffers
    struct v4l2_buffer buffer_info;
    buffer_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer_info.memory = V4L2_MEMORY_MMAP;
    buffer_info.index = 0;
    if (ioctl(video_fd, VIDIOC_QUERYBUF, &buffer_info) == -1) {
        perror("Failed to query video buffer");
        close(video_fd);
        return -1;
    }

    void *buffer_start = mmap(NULL, buffer_info.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buffer_info.m.offset);
    if (buffer_start == MAP_FAILED) {
        perror("Failed to map video buffer");
        close(video_fd);
        return -1;
    }

    // Start video streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type) == -1) {
        perror("Failed to start video streaming");
        close(video_fd);
        return -1;
    }

    // Open the output file
    file_fd = open(VIDEO_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (file_fd == -1) {
        perror("Failed to open output file");
        munmap(buffer_start, buffer_info.length);
        close(video_fd);
        return -1;
    }

    // Capture video frames and save to file
    if (ioctl(video_fd, VIDIOC_QBUF, &buffer_info) == -1) {
        perror("Failed to enqueue video buffer");
        close(file_fd);
        munmap(buffer_start, buffer_info.length);
        close(video_fd);
        return -1;
    }

    if (ioctl(video_fd, VIDIOC_DQBUF, &buffer_info) == -1) {
        perror("Failed to dequeue video buffer");
        close(file_fd);
        munmap(buffer_start, buffer_info.length);
        close(video_fd);
        return -1;
    }

    write(file_fd, buffer_start, buffer_info.length);

    // Cleanup
    close(file_fd);
    munmap(buffer_start, buffer_info.length);
    close(video_fd);

    return 0;
}
