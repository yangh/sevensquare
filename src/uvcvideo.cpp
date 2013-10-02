
#include "uvcvideo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <getopt.h>		/* getopt_long() */

#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>		/* for videodev2.h */

#include <linux/videodev2.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#ifndef USE_STILL
#define USE_STILL           0
#endif

#define MAX_CAPTURE			10000

/* Still image format magic */
#define UVC_STILL_MAGIC			0xF03E5335

#define STILL_DUMP_FILE			"/tmp/x.yuv"

typedef enum {
    IO_METHOD_READ,
    IO_METHOD_MMAP,
    IO_METHOD_USERPTR
} io_method;

struct buffer {
    void *start;
    size_t length;
};

static char *dev_name = NULL;
static io_method io = IO_METHOD_MMAP;
static int fd = -1;
static struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;
static unsigned int capture_count = 100;
static unsigned int capture_interval = 10;
static unsigned int capture_idx = 0;
static unsigned int capture_fmt = V4L2_PIX_FMT_YUYV;
static unsigned int no_cap = 0;
static unsigned int save_raw_image = 0;

#define DEFAULT_PREVIEW_WIDTH		640
#define DEFAULT_PREVIEW_HEIGHT		480
static int pw = DEFAULT_PREVIEW_WIDTH;
static int ph = DEFAULT_PREVIEW_HEIGHT;

#define DEFAULT_STILL_WIDTH			1280
#define DEFAULT_STILL_HEIGHT		960
static int sw = DEFAULT_STILL_WIDTH;
static int sh = DEFAULT_STILL_HEIGHT;
struct buffer *still_buffers = NULL;
static unsigned int still_n_buffers = 0;

static int do_snapshot(void);

static void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));

    exit(EXIT_FAILURE);
}

static int xioctl(int fd, int request, void *arg)
{
    int r;

    do
        r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

static void dump_raw_to_file(void *data, int len, const char *suffix)
{
    int fd;
    int ret;
    char name[128];
    char *fcc;

    fcc = (char *)&capture_fmt;
    sprintf (name, "raw-%d%s.%c%c%c%c",
            capture_idx, suffix,
            fcc[0], fcc[1], fcc[2], fcc[3]);
    fd = open(name, O_CREAT|O_TRUNC|O_RDWR, 0660);

    //printf("Dumped %d bytes\n", len);
    while(len > 0)
    {
        ret = write (fd, data, len);
        if (ret < 0)
            break;
        len -= ret;
    }

    close(fd);
}

static void process_image(const struct v4l2_buffer *buf)
{
    if (save_raw_image)
        dump_raw_to_file(buffers[buf->index].start,
                         buf->bytesused, "");
    fputc('.', stdout);
    fflush(stdout);
}

static int do_snapshot(void)
{
    struct v4l2_buffer buf;

    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.flags = UVC_STILL_MAGIC;

    if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF STILL");
        }
    }

    /* process_image(...) */

    fputc('A', stdout);
    fflush(stdout);
    printf(" Size: %d %d\n", buf.bytesused, (capture_idx / capture_interval));
    if (save_raw_image)
        dump_raw_to_file(still_buffers[buf.index].start,
                         buf.bytesused, ".still");

    /* Ensure magic set */
    buf.flags = UVC_STILL_MAGIC;

    if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF STILL");

    return 0;
}

static int read_frame(char *data, int len)
{
    struct v4l2_buffer buf;
    unsigned int i;
    image_info_t img;

    switch (io) {
    case IO_METHOD_READ:

        break;

    case IO_METHOD_MMAP:
        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
            switch (errno) {
            case EAGAIN:
                return 0;

            case EIO:
                /* Could ignore EIO, see spec. */

                /* fall through */

            default:
                errno_exit("VIDIOC_DQBUF");
            }
        }

        assert(buf.index < n_buffers);

        //process_image(&buf);
        img.width = pw;
        img.height = ph;
        img.yuv_buffer = (uint8_t *) buffers[buf.index].start;
        img.format = YUV_422;
        DrawYUV422(&img, (uint32_t *)data);
        //memcpy(data, buffers[buf.index].start, MIN(buf.bytesused, len));
        capture_idx++;

        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit("VIDIOC_QBUF");

#if USE_STILL
        if ((capture_idx % capture_interval) == 0) {
            do_snapshot();
        }
#endif

        break;

    case IO_METHOD_USERPTR:
        break;
    }

    return 1;
}

static void stop_capturing(void)
{
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
            errno_exit("VIDIOC_STREAMOFF");

        break;
    }
}

static void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;

    switch (io) {
    case IO_METHOD_READ:
        /* Nothing to do. */
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }

#if USE_STILL
        for (i = 0; i < still_n_buffers; ++i) {
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            buf.flags = UVC_STILL_MAGIC;

            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                errno_exit("VIDIOC_QBUF");
        }
#endif

        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
            errno_exit("VIDIOC_STREAMON");

        break;

    case IO_METHOD_USERPTR:
        break;
    }
}

static void uninit_device(void)
{
    unsigned int i;

    switch (io) {
    case IO_METHOD_READ:
        free(buffers[0].start);
        break;

    case IO_METHOD_MMAP:
        for (i = 0; i < n_buffers; ++i)
            if (-1 == munmap(buffers[i].start, buffers[i].length))
                errno_exit("munmap");
#if USE_STILL
        /* STILL */
        for (i = 0; i < still_n_buffers; ++i)
            if (-1 == munmap(still_buffers[i].start, still_buffers[i].length))
                errno_exit("munmap still");
#endif
        break;

    case IO_METHOD_USERPTR:
        for (i = 0; i < n_buffers; ++i)
            free(buffers[i].start);
        break;
    }

    free(buffers);
}

static void init_read(unsigned int buffer_size)
{
    buffers = (struct buffer *)calloc(1, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    buffers[0].length = buffer_size;
    buffers[0].start = malloc(buffer_size);

    if (!buffers[0].start) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }
}

static void init_mmap_still(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 3;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    req.reserved[0] = UVC_STILL_MAGIC;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                "memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS STILL");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }

#if 0
    printf("Still reqbuf got: %d\n", req.count);
#endif

    still_buffers = (struct buffer *)calloc(req.count, sizeof(*still_buffers));
    memset(still_buffers, 0, sizeof still_buffers);

    if (!still_buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (still_n_buffers = 0; still_n_buffers < req.count; ++still_n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = still_n_buffers;
        buf.flags = UVC_STILL_MAGIC;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF STILL");

        still_buffers[still_n_buffers].length = buf.length;
        still_buffers[still_n_buffers].start = mmap(NULL /* start anywhere */ ,
                        buf.length,
                        PROT_READ | PROT_WRITE
                        /* required */ ,
                        MAP_SHARED /* recommended */ ,
                        fd, buf.m.offset);

#if 0
        printf("Still buffer: %p, len: %d, raw: 0x%x\n",
                still_buffers[still_n_buffers].start,
                (int)still_buffers[still_n_buffers].length,
                buf.m.offset);
#endif

        if (MAP_FAILED == still_buffers[still_n_buffers].start)
            errno_exit("mmap still");
    }
}

static void init_mmap(void)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s does not support "
                "memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if (req.count < 2) {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }

    buffers = (struct buffer *)calloc(req.count, sizeof(*buffers));

    if (!buffers) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
                        buf.length,
                        PROT_READ | PROT_WRITE
                        /* required */ ,
                        MAP_SHARED /* recommended */ ,
                        fd, buf.m.offset);

#if 0
        printf("Preview buffer: %p, len: %d, raw: 0x%x\n",
                buffers[n_buffers].start,
                (int)buffers[n_buffers].length,
                buf.m.offset);
#endif

        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

static void init_device(void)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    char *fcc;
    unsigned int min;

    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf(stderr, "%s is no V4L2 device\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        exit(EXIT_FAILURE);
    }

    switch (io) {
    case IO_METHOD_READ:
        if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
            fprintf(stderr, "%s does not support read i/o\n",
                dev_name);
            exit(EXIT_FAILURE);
        }

        break;

    case IO_METHOD_MMAP:
    case IO_METHOD_USERPTR:
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            fprintf(stderr, "%s does not support streaming i/o\n",
                dev_name);
            exit(EXIT_FAILURE);
        }

        break;
    }

    /* Select video input, video standard and tune here. */

    CLEAR(cropcap);

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;	/* reset to default */

        if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:
                /* Cropping not supported. */
                break;
            default:
                /* Errors ignored. */
                break;
            }
        }
    } else {
        /* Errors ignored. */
    }

    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = pw;
    fmt.fmt.pix.height = ph;
    fmt.fmt.pix.pixelformat = capture_fmt;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");

    /* Note VIDIOC_S_FMT may change width and height. */

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if (fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if (fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

#if USE_STILL
    CLEAR(fmt);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = sw;
    fmt.fmt.pix.height = sh;
    fmt.fmt.pix.pixelformat = capture_fmt;
    fmt.fmt.pix.priv = UVC_STILL_MAGIC;
    /* fmt.fmt.pix.field = V4L2_FIELD_INTERLACED; */

    fcc = (char *)&fmt.fmt.pix.pixelformat;
    printf("Set still format: %dx%d, format: %c%c%c%c\n",
            sw, sh,
            fcc[0], fcc[1], fcc[2], fcc[3]);

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT Still");
#endif

    switch (io) {
    case IO_METHOD_READ:
        break;

    case IO_METHOD_MMAP:
        init_mmap();
#if USE_STILL
        init_mmap_still();
#endif
        break;

    case IO_METHOD_USERPTR:
        break;
    }
}

static void close_device(void)
{
    if (-1 == close(fd))
        errno_exit("close");

    fd = -1;
}

static void open_device(void)
{
    struct stat st;

    if (-1 == stat(dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",
            dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (!S_ISCHR(st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }
#if 0
    fd = open(dev_name, O_RDWR /* required */  | O_NONBLOCK, 0);
#endif
    /* Use blocked IO */
    fd = open(dev_name, O_RDWR, 0);

    if (-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n",
            dev_name, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

UVCVideo::UVCVideo()
{

}

UVCVideo::~UVCVideo()
{

}

int UVCVideo::openDevice(const char *dev)
{
    DT_TRACE("Open camera");

    dev_name = "/dev/video1";

    open_device();
    init_device();
    start_capturing();
}

int UVCVideo::closeDevice(void)
{
    DT_TRACE("Close camera");
    stop_capturing();
    uninit_device();
    close_device();
}

int UVCVideo::readFrame(QByteArray &bytes, int w, int h, int bpp)
{
    read_frame(bytes.data(), w * h * bpp);
    return 0;
}
