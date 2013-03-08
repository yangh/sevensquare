/*
 * keymap.h
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#ifndef KEYMAP_H_
#define KEYMAP_H_

// Android key list
// From: android/frameworks/native/include/android/keycodes.h
#include "keycodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Qt to Android key map
// Refer: /usr/include/qt4/Qt/qnamespace.h
struct keymap {
    int a;
    int q;
} keymaps[] = {

#include "keymap-generated.h"

};

#define KEY_NUM (sizeof(keymaps)/sizeof(keymaps[0]))

#ifdef __cplusplus
}
#endif

#endif /* KEYMAP_H_ */
