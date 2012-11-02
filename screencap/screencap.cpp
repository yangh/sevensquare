/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <binder/IMemory.h>
#include <surfaceflinger/SurfaceComposerClient.h>

#include <SkImageEncoder.h>
#include <SkBitmap.h>
#include <SkStream.h>

using namespace android;

static void usage(const char* pname)
{
    fprintf(stderr,
            "usage: %s [-hp] [FILENAME]\n"
            "   -h: this message\n"
            "   -p: save the file as a png.\n"
            "If FILENAME ends with .png it will be saved as a png.\n"
            "If FILENAME is not given, the results will be printed to stdout.\n",
            pname
    );
}

static SkBitmap::Config flinger2skia(PixelFormat f)
{
    switch (f) {
        case PIXEL_FORMAT_A_8:
        case PIXEL_FORMAT_L_8:
            return SkBitmap::kA8_Config;
        case PIXEL_FORMAT_RGB_565:
            return SkBitmap::kRGB_565_Config;
        case PIXEL_FORMAT_RGBA_4444:
            return SkBitmap::kARGB_4444_Config;
        default:
            return SkBitmap::kARGB_8888_Config;
    }
}

static status_t vinfoToPixelFormat(const fb_var_screeninfo& vinfo,
        uint32_t* bytespp, uint32_t* f)
{

    switch (vinfo.bits_per_pixel) {
        case 16:
            *f = PIXEL_FORMAT_RGB_565;
            *bytespp = 2;
            break;
        case 24:
            *f = PIXEL_FORMAT_RGB_888;
            *bytespp = 3;
            break;
        case 32:
            // TODO: do better decoding of vinfo here
            *f = PIXEL_FORMAT_RGBX_8888;
            *bytespp = 4;
            break;
        default:
            return BAD_VALUE;
    }
    return NO_ERROR;
}

int convert_rgba32_rgb565(const void *src, off_t offset, off_t cnt, void *dst)
{
	int n = cnt;
	volatile uint8_t *s = (uint8_t *) src + offset;
	volatile uint16_t *d = (uint16_t *) dst;
	volatile uint16_t r, g, b;

	while (cnt-- > 0) {
		r = s[0];
		g = s[1];
		b = s[2];

		*d = (((r >> 3 << 11)  & 0xFB00) | ((g >> 2 << 5) & 0x07E0) | (b >> 3));
		d++;
		s += 4;
	}

	return cnt - n;
}

int convert_rgba32_rgb888(const void *src, off_t offset, off_t cnt, void *dst)
{
	int n = cnt;
	volatile uint8_t *s = (uint8_t *) src + offset;
	volatile uint8_t *d = (uint8_t *) dst;

	while (cnt-- > 0) {
		*d++ = *s++;
		*d++ = *s++;
		*d++ = *s++;
		s++;
	}

	return cnt - n;
}

int main(int argc, char** argv)
{
    const char* pname = argv[0];
    bool png = false;
    bool rgb565 = false;
    bool rgb888 = false;
    int c;

    while ((c = getopt(argc, argv, "phqs")) != -1) {
        switch (c) {
            case 'p':
                png = true;
                break;
            case 'q': /* Quality prefered, convert to rgb888 */
                rgb888 = true;
                break;
            case 's': /* Speed prefered, convert to rgb565 */
                rgb565 = true;
                break;
            case '?':
            case 'h':
                usage(pname);
                return 1;
        }
    }
    argc -= optind;
    argv += optind;

    int fd = -1;
    if (argc == 0) {
        fd = dup(STDOUT_FILENO);
    } else if (argc == 1) {
        const char* fn = argv[0];
        fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (fd == -1) {
            fprintf(stderr, "Error opening file: %s (%s)\n", fn, strerror(errno));
            return 1;
        }
        const int len = strlen(fn);
        if (len >= 4 && 0 == strcmp(fn+len-4, ".png")) {
            png = true;
        }
    }
    
    if (fd == -1) {
        usage(pname);
        return 1;
    }

    void const* mapbase = MAP_FAILED;
    ssize_t mapsize = -1;

    void const* base = 0;
    uint32_t w, h, f;
    size_t size = 0;

    ScreenshotClient screenshot;
    if (screenshot.update() == NO_ERROR) {
        base = screenshot.getPixels();
        w = screenshot.getWidth();
        h = screenshot.getHeight();
        f = screenshot.getFormat();
        size = screenshot.getSize();
    } else {
        const char* fbpath = "/dev/graphics/fb0";
        int fb = open(fbpath, O_RDONLY);
        if (fb >= 0) {
            struct fb_var_screeninfo vinfo;
            if (ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) == 0) {
                uint32_t bytespp;
                if (vinfoToPixelFormat(vinfo, &bytespp, &f) == NO_ERROR) {
                    size_t offset = (vinfo.xoffset + vinfo.yoffset*vinfo.xres) * bytespp;
                    w = vinfo.xres;
                    h = vinfo.yres;
                    size = w*h*bytespp;
                    mapsize = offset + size;
                    mapbase = mmap(0, mapsize, PROT_READ, MAP_PRIVATE, fb, 0);
                    if (mapbase != MAP_FAILED) {
                        base = (void const *)((char const *)mapbase + offset);
                    }
                }
            }
            close(fb);
        }
    }

    if (base) {
        if (png) {
            SkBitmap b;
            b.setConfig(flinger2skia(f), w, h);
            b.setPixels((void*)base);
            SkDynamicMemoryWStream stream;
            SkImageEncoder::EncodeStream(&stream, b,
                    SkImageEncoder::kPNG_Type, SkImageEncoder::kDefaultQuality);
            write(fd, stream.getStream(), stream.getOffset());
        } else {
            write(fd, &w, 4);
            write(fd, &h, 4);
	    if (rgb565) {
	        void *buf;
		size = w * h * 2;
		buf = malloc(size);
		convert_rgba32_rgb565(base, 0, w*h, buf);
		f = PIXEL_FORMAT_RGB_565;
		write(fd, &f, 4);
		write(fd, buf, size);
		free(buf);
	    } else if (rgb888) {
	        void *buf;
		size = w * h * 3;
		buf = malloc(size);
		convert_rgba32_rgb888(base, 0, w*h, buf);
		f = PIXEL_FORMAT_RGB_888;
		write(fd, &f, 4);
		write(fd, buf, size);
		free(buf);
	    }  else {
                write(fd, &f, 4);
		write(fd, base, size);
	    }
        }
    }
    close(fd);
    if (mapbase != MAP_FAILED) {
        munmap((void *)mapbase, mapsize);
    }
    return 0;
}
