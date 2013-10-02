#ifndef UVCVIDEO_H
#define UVCVIDEO_H

#include <stdint.h>
#include "camera-effect.h"

#include <QString>
#include <QStringList>
#include <QThread>
#include <QProcess>
#include <QMutex>
#include <QWaitCondition>
#include <QFile>
#include <QPoint>
#include <QTimer>

#include "debug.h"
#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

/** Fast type for signed 16 bits. Use for parameter passing and local variables, not for storage
*/
typedef unsigned    U8CPU;

/** 32 bit ARGB color value, not premultiplied. The color components are always in
    a known order. This is different from SkPMColor, which has its bytes in a configuration
    dependent order, to match the format of kARGB32 bitmaps. SkColor is the type used to
    specify colors in SkPaint and in gradients.
*/
typedef uint32_t SkColor;

/** Return a SkColor value from 8 bit component values
*/
static inline SkColor SkColorSetARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    //SkASSERT(a <= 255 && r <= 255 && g <= 255 && b <= 255);

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

/** Return a SkColor value from 8 bit component values, with an implied value
    of 0xFF for alpha (fully opaque)
*/
#define SkColorSetRGB(r, g, b)  SkColorSetARGB(0xFF, r, g, b)

/* CLIPIT is define in camif */
#define CLIPIT(a) ( ((a<0)||(a>255)) \
                 ? ((a<0)?0:255) \
                 :a)

static const int16_t ycbcr_convert[6] = { 8, 25803, -3071, -7672, 30399, 12 };

inline SkColor
SkColorSetYCrCb(uint8_t y, uint8_t cr, uint8_t cb)
{
    int32_t rc, gc, bc;
    int32_t r, g, b;
    int8_t u, v;

    u = cr - 128;
    v = cb - 128;

    rc = ((ycbcr_convert[0] * v + ycbcr_convert[1] * u) << 2) | 0x8000;
    gc = ((ycbcr_convert[2] * v + ycbcr_convert[3] * u) << 2) | 0x8000;
    bc = ((ycbcr_convert[4] * v + ycbcr_convert[5] * u) << 2) | 0x8000;

    r = CLIPIT(y + (rc >> 16));
    g = CLIPIT(y + (gc >> 16));
    b = CLIPIT(y + (bc >> 16));

    return SkColorSetRGB(r, g, b);
}

static int
DrawYUV422 (image_info_t *img, uint32_t *rgb8888_buffer)
{
    int ret = 0;
    register uint8_t* data_in;
    register uint8_t cb, cr;
    register int32_t luma1, luma2;
    register int32_t r,g,b;
    register uint32_t w, h;
    register uint32_t* line_ptr;
    int32_t rc, gc, bc;
    uint32_t row, col;

    w = img->width;
    h = img->height;
    data_in = img->yuv_buffer;
    line_ptr = rgb8888_buffer;

    for (row = h; row; row--)
    {
        for (col = w >> 1; col; col--)
        {
            /*----------------------------------
              2 pixels once

              Y U Y V
              0 1 2 3
              ----------------------------------*/
            luma1 = *data_in++;
            cr    = *data_in++;
            luma2 = *data_in++;
            cb    = *data_in++;

            rc = (ycbcr_convert[0]*(cb-128) + ycbcr_convert[1]*(cr-128))*4+0x8000;
            gc = (ycbcr_convert[2]*(cb-128) + ycbcr_convert[3]*(cr-128))*4+0x8000;
            bc = (ycbcr_convert[4]*(cb-128) + ycbcr_convert[5]*(cr-128))*4+0x8000;

            r = CLIPIT(luma1 + (rc>>16));
            g = CLIPIT(luma1 + (gc>>16));
            b = CLIPIT(luma1 + (bc>>16));
            *line_ptr++ = SkColorSetRGB(r, g, b);

            r = CLIPIT(luma2 + (rc>>16));
            g = CLIPIT(luma2 + (gc>>16));
            b = CLIPIT(luma2 + (bc>>16));
            *line_ptr++ = SkColorSetRGB(r, g, b);
        }

    } /* end row loop */

    /* TODO: proccess odd width */
    return ret;
}

class UVCVideo : public QObject
{
    Q_OBJECT

public:
    UVCVideo();
    ~UVCVideo();

    int openDevice(const char *dev);
    int closeDevice(void);
    int readFrame(QByteArray &bytes, int w, int h, int bpp);
};

#endif // UVCVIDEO_H
