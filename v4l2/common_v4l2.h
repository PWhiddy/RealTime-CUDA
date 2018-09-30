#ifndef COMMON_V4L2_H
#define COMMON_V4L2_H

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>

#include "libv4l2.h"
#include <linux/videodev2.h>

#define COMMON_V4L2_CLEAR(x) memset(&(x), 0, sizeof(x))
#define COMMON_V4L2_DEVICE "/dev/video0"

typedef struct {
    void *start;
    size_t length;
} CommonV4l2_Buffer;

typedef struct {
    int fd;
    CommonV4l2_Buffer *buffers;
    struct v4l2_buffer buf;
    unsigned int n_buffers;
} CommonV4l2;

void CommonV4l2_xioctl(int fh, unsigned long int request, void *arg)
{
    int r;
    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));
    if (r == -1) {
        fprintf(stderr, "error %d, %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void CommonV4l2_init(CommonV4l2 *context, char *dev_name, unsigned int x_res, unsigned int y_res) {
    enum v4l2_buf_type type;
    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    unsigned int i;

    context->fd = v4l2_open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (context->fd < 0) {
        perror("Cannot open device");
        exit(EXIT_FAILURE);
    }
    COMMON_V4L2_CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = x_res;
    fmt.fmt.pix.height      = y_res;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    CommonV4l2_xioctl(context->fd, VIDIOC_S_FMT, &fmt);
    if ((fmt.fmt.pix.width != x_res) || (fmt.fmt.pix.height != y_res))
        printf("Warning: driver is sending image at %dx%d\n",
            fmt.fmt.pix.width, fmt.fmt.pix.height);
    COMMON_V4L2_CLEAR(req);
    req.count = 2;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    CommonV4l2_xioctl(context->fd, VIDIOC_REQBUFS, &req);
    context->buffers = (CommonV4l2_Buffer*) calloc(req.count, sizeof(*context->buffers));
    for (context->n_buffers = 0; context->n_buffers < req.count; ++context->n_buffers) {
        COMMON_V4L2_CLEAR(context->buf);
        context->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        context->buf.memory = V4L2_MEMORY_MMAP;
        context->buf.index = context->n_buffers;
        CommonV4l2_xioctl(context->fd, VIDIOC_QUERYBUF, &context->buf);
        context->buffers[context->n_buffers].length = context->buf.length;
        context->buffers[context->n_buffers].start = v4l2_mmap(NULL, context->buf.length,
            PROT_READ | PROT_WRITE, MAP_SHARED, context->fd, context->buf.m.offset);
        if (MAP_FAILED == context->buffers[context->n_buffers].start) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < context->n_buffers; ++i) {
        COMMON_V4L2_CLEAR(context->buf);
        context->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        context->buf.memory = V4L2_MEMORY_MMAP;
        context->buf.index = i;
        CommonV4l2_xioctl(context->fd, VIDIOC_QBUF, &context->buf);
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    CommonV4l2_xioctl(context->fd, VIDIOC_STREAMON, &type);
}

void CommonV4l2_updateImage(CommonV4l2 *context) {
    fd_set fds;
    int r;
    struct timeval tv;

    do {
        FD_ZERO(&fds);
        FD_SET(context->fd, &fds);

        /* Timeout. */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(context->fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno == EINTR)));
    if (r == -1) {
        perror("select");
        exit(EXIT_FAILURE);
    }
    COMMON_V4L2_CLEAR(context->buf);
    context->buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    context->buf.memory = V4L2_MEMORY_MMAP;
    CommonV4l2_xioctl(context->fd, VIDIOC_DQBUF, &context->buf);
    CommonV4l2_xioctl(context->fd, VIDIOC_QBUF, &context->buf);
}

/* TODO must be called after updateImage? Or is init enough? */
void * CommonV4l2_getImage(CommonV4l2 *context) {
    return context->buffers[context->buf.index].start;
}

/* TODO must be called after updateImage? Or is init enough? */
size_t CommonV4l2_getImageSize(CommonV4l2 *context) {
    return context->buffers[context->buf.index].length;
}

void CommonV4l2_deinit(CommonV4l2 *context) {
    unsigned int i;
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    CommonV4l2_xioctl(context->fd, VIDIOC_STREAMOFF, &type);
    for (i = 0; i < context->n_buffers; ++i)
        v4l2_munmap(context->buffers[i].start, context->buffers[i].length);
    v4l2_close(context->fd);
    free(context->buffers);
}

#endif
