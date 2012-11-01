#ifndef KEYMAP_H_
#define KEYMAP_H_

// Android key list
#include "keycodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// Qt to Android key map
// /usr/include/qt4/Qt/qnamespace.h 
// android/frameworks/native/include/android/keycodes.h
//
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
