/*
 * utils.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#include <stdint.h>
#include <unistd.h>
#include <strings.h>

int convertRGBAtoRGB888(char *data, int width, int height, int offset)
{
    int x, y;
    char *p, *n;

    if (data == NULL)
        return 0;

    p = n = data + offset;

    // RGBX32 -> RGB888
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            *p++ = *n++;
            *p++ = *n++;
            *p++ = *n++;
            n++; // skip alpha
        }
    }

    return width * height * 3;
}

int bigEndianStreamDataToInt32(const char *data)
{
    uint32_t v = 0;
    unsigned char * c = (unsigned char *) data;
    unsigned char * i = (unsigned char *) &v;

    if (data == NULL)
        return 0;

    i[0] = c[0];
    i[1] = c[1];
    i[2] = c[2];
    i[3] = c[3];

    return v;
}

int littleEndianStreamDataToInt32(const char *data)
{
    uint32_t v = 0;
    unsigned char * c = (unsigned char *) data;
    unsigned char * i = (unsigned char *) &v;

    if (data == NULL)
        return 0;

    i[0] = c[3];
    i[1] = c[2];
    i[2] = c[1];
    i[3] = c[0];

    return v;
}
