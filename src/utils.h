/*
 * utils.h
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#ifndef UTILS_H
#define UTILS_H

#ifdef cplusplus
extern "C" {
#endif

int convertRGBAtoRGB888(char *data, int width, int height, int offset);

int bigEndianStreamDataToInt32(const char *binary);
int littleEndianStreamDataToInt32(const char *binary);

#ifdef cplusplus
}
#endif

#endif // UTILS_H
