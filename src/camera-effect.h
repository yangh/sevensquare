/*
 * camera-effect.c header file for adding effect for camera raw data
 *
 * Copyright (C) 2010   Yang Hong <yanghong@thunderst.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 */

#ifndef _CAMERA_EFFECT_H_
#define _CAMERA_EFFECT_H_

#include <inttypes.h>

#ifdef __cplusplus__
extern "C" {
#endif

/* Supported image type */
typedef enum {
    YUV_422,    /* YUV422, YUYV... for 2 pixels */
    YUV_420LP   /* YUV420 in line packed format, YYYY...UVUV... */
} image_type_t;

#define YUV420LP_CRCB_OFFSET(width, height) ((width) * (height))

/* Effect error type */
typedef enum {
    EFF_OK,
    EFF_ERROR_OOM,
    EFF_ERROR_INVALID_ARG,
    EFF_ERROR_UNKNOW
} effect_error_t;

/* Image infomation passed to effect function */
typedef struct _image_info image_info_t;

struct _image_info {
    uint32_t width;
    uint32_t height;

    image_type_t format;
    uint8_t *yuv_buffer;
};

/* Effect types */
typedef enum {
    EFFECT_GRAY,
    EFFECT_SEPHIA,
    EFFECT_NEGATIVE,
    EFFECT_SOLARIZE,
    EFFECT_MAX
} effect_type_t;

typedef int (*effect_gray)      (image_info_t *img);
typedef int (*effect_sephia)    (image_info_t *img);
typedef int (*effect_negative)  (image_info_t *img);
typedef int (*effect_solarize)  (image_info_t *img);

/* Effect operations */
struct camera_effect_ops {
    effect_gray     gray;
    effect_sephia   sephia;
    effect_negative negative;
    effect_solarize  solarize;
};

#if 0
/* Return effect operations for given image type */
const struct camera_effect_ops * camera_effect_get_ops (image_type_t type);

/* Get image data size */
uint32_t image_get_data_size (image_info_t *img);
#endif

#ifdef __cplusplus__
}
#endif

#endif /* _CAMERA_EFFECT_H_ */
