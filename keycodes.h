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

#ifndef _ANDROID_KEYCODES_H
#define _ANDROID_KEYCODES_H

/******************************************************************
 *
 * IMPORTANT NOTICE:
 *
 *   This file is part of Android's set of stable system headers
 *   exposed by the Android NDK (Native Development Kit).
 *
 *   Third-party source AND binary code relies on the definitions
 *   here to be FROZEN ON ALL UPCOMING PLATFORM RELEASES.
 *
 *   - DO NOT MODIFY ENUMS (EXCEPT IF YOU ADD NEW 32-BIT VALUES)
 *   - DO NOT MODIFY CONSTANTS OR FUNCTIONAL MACROS
 *   - DO NOT CHANGE THE SIGNATURE OF FUNCTIONS IN ANY WAY
 *   - DO NOT CHANGE THE LAYOUT OR SIZE OF STRUCTURES
 */

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Key codes.
 */
enum {
    AKEYCODE_UNKNOWN         = 0,  /* Qt:: */
    AKEYCODE_SOFT_LEFT       = 1,
    AKEYCODE_SOFT_RIGHT      = 2,
    AKEYCODE_HOME            = 3,  /* Qt::Key_F1 = 0x01000030, */
    AKEYCODE_BACK            = 4,  /* Qt::Key_F2 = 0x01000031, */
    AKEYCODE_CALL            = 5,  /* Qt::Key_F3 = 0x01000032, */
    AKEYCODE_ENDCALL         = 6,  /* Qt::Key_F4 = 0x01000033, */
    AKEYCODE_0               = 7,  /* Qt::Key_0 = 0x30, */
    AKEYCODE_1               = 8,  /* Qt::Key_1 = 0x31, */
    AKEYCODE_2               = 9,  /* Qt::Key_2 = 0x32, */
    AKEYCODE_3               = 10, /* Qt::Key_3 = 0x33, */
    AKEYCODE_4               = 11, /* Qt::Key_4 = 0x34, */
    AKEYCODE_5               = 12, /* Qt::Key_5 = 0x35, */
    AKEYCODE_6               = 13, /* Qt::Key_6 = 0x36, */
    AKEYCODE_7               = 14, /* Qt::Key_7 = 0x37, */
    AKEYCODE_8               = 15, /* Qt::Key_8 = 0x38, */
    AKEYCODE_9               = 16, /* Qt::Key_9 = 0x39, */
    AKEYCODE_STAR            = 17, /* Qt:: */
    AKEYCODE_POUND           = 18, /* Qt:: */
    AKEYCODE_DPAD_UP         = 19, /* Qt::Key_Up = 0x01000013, */
    AKEYCODE_DPAD_DOWN       = 20, /* Qt::Key_Down = 0x01000015, */
    AKEYCODE_DPAD_LEFT       = 21, /* Qt::Key_Left = 0x01000012, */
    AKEYCODE_DPAD_RIGHT      = 22, /* Qt::Key_Right = 0x01000014, */
    AKEYCODE_DPAD_CENTER     = 23, /* Qt::Key_Return = 0x01000004, */
    AKEYCODE_VOLUME_UP       = 24, /* Qt:: */
    AKEYCODE_VOLUME_DOWN     = 25, /* Qt:: */
    AKEYCODE_POWER           = 26, /* Qt:: */
    AKEYCODE_CAMERA          = 27, /* Qt:: */
    AKEYCODE_CLEAR           = 28, /* Qt:: */
    AKEYCODE_A               = 29, /* Qt::Key_A = 0x41, */
    AKEYCODE_B               = 30, /* Qt::Key_B = 0x42, */
    AKEYCODE_C               = 31, /* Qt::Key_C = 0x43, */
    AKEYCODE_D               = 32, /* Qt::Key_D = 0x44, */
    AKEYCODE_E               = 33, /* Qt::Key_E = 0x45, */
    AKEYCODE_F               = 34, /* Qt::Key_F = 0x46, */
    AKEYCODE_G               = 35, /* Qt::Key_G = 0x47, */
    AKEYCODE_H               = 36, /* Qt::Key_H = 0x48, */
    AKEYCODE_I               = 37, /* Qt::Key_I = 0x49, */
    AKEYCODE_J               = 38, /* Qt::Key_J = 0x4a, */
    AKEYCODE_K               = 39, /* Qt::Key_K = 0x4b, */
    AKEYCODE_L               = 40, /* Qt::Key_L = 0x4c, */
    AKEYCODE_M               = 41, /* Qt::Key_M = 0x4d, */
    AKEYCODE_N               = 42, /* Qt::Key_N = 0x4e, */
    AKEYCODE_O               = 43, /* Qt::Key_O = 0x4f, */
    AKEYCODE_P               = 44, /* Qt::Key_P = 0x50, */
    AKEYCODE_Q               = 45, /* Qt::Key_Q = 0x51, */
    AKEYCODE_R               = 46, /* Qt::Key_R = 0x52, */
    AKEYCODE_S               = 47, /* Qt::Key_S = 0x53, */
    AKEYCODE_T               = 48, /* Qt::Key_T = 0x54, */
    AKEYCODE_U               = 49, /* Qt::Key_U = 0x55, */
    AKEYCODE_V               = 50, /* Qt::Key_V = 0x56, */
    AKEYCODE_W               = 51, /* Qt::Key_W = 0x57, */
    AKEYCODE_X               = 52, /* Qt::Key_X = 0x58, */
    AKEYCODE_Y               = 53, /* Qt::Key_Y = 0x59, */
    AKEYCODE_Z               = 54, /* Qt::Key_Z = 0x5a, */
    AKEYCODE_COMMA           = 55, /* Qt::Key_Comma = 0x2c, */
    AKEYCODE_PERIOD          = 56, /* Qt:: */
    AKEYCODE_ALT_LEFT        = 57, /* Qt:: */
    AKEYCODE_ALT_RIGHT       = 58, /* Qt:: */
    AKEYCODE_SHIFT_LEFT      = 59, /* Qt:: */
    AKEYCODE_SHIFT_RIGHT     = 60, /* Qt:: */
    AKEYCODE_TAB             = 61, /* Qt::Key_Tab = 0x01000001, */
    AKEYCODE_SPACE           = 62, /* Qt::Key_Space = 0x20, */
    AKEYCODE_SYM             = 63, /* Qt:: */
    AKEYCODE_EXPLORER        = 64, /* Qt:: */
    AKEYCODE_ENVELOPE        = 65, /* Qt:: */
    AKEYCODE_ENTER           = 66, /* Qt:: */
    AKEYCODE_DEL             = 67, /* Qt:: */
    AKEYCODE_GRAVE           = 68, /* Qt:: */
    AKEYCODE_MINUS           = 69, /* Qt::Key_Minus = 0x2d, */
    AKEYCODE_EQUALS          = 70, /* Qt::Key_Equal = 0x3d, */
    AKEYCODE_LEFT_BRACKET    = 71, /* Qt:: */
    AKEYCODE_RIGHT_BRACKET   = 72, /* Qt:: */
    AKEYCODE_BACKSLASH       = 73, /* Qt:: */
    AKEYCODE_SEMICOLON       = 74, /* Qt:: */
    AKEYCODE_APOSTROPHE      = 75, /* Qt:: */
    AKEYCODE_SLASH           = 76, /* Qt::Key_Slash = 0x2f, */
    AKEYCODE_AT              = 77, /* Qt:: */
    AKEYCODE_NUM             = 78, /* Qt:: */
    AKEYCODE_HEADSETHOOK     = 79,
    AKEYCODE_FOCUS           = 80,   // *Camera* focus
    AKEYCODE_PLUS            = 81, /* Qt::Key_Plus = 0x2b, */
    AKEYCODE_MENU            = 82, /* Qt::Key_Menu = 0x01000055, */
    AKEYCODE_NOTIFICATION    = 83,
    AKEYCODE_SEARCH          = 84,
    AKEYCODE_MEDIA_PLAY_PAUSE= 85,
    AKEYCODE_MEDIA_STOP      = 86,
    AKEYCODE_MEDIA_NEXT      = 87,
    AKEYCODE_MEDIA_PREVIOUS  = 88,
    AKEYCODE_MEDIA_REWIND    = 89,
    AKEYCODE_MEDIA_FAST_FORWARD = 90,
    AKEYCODE_MUTE            = 91,
    AKEYCODE_PAGE_UP         = 92, /* Qt::Key_Up = 0x01000013,    */
    AKEYCODE_PAGE_DOWN       = 93, /* Qt::Key_Right = 0x01000014, */
    AKEYCODE_PICTSYMBOLS     = 94,
    AKEYCODE_SWITCH_CHARSET  = 95,
    AKEYCODE_BUTTON_A        = 96,
    AKEYCODE_BUTTON_B        = 97,
    AKEYCODE_BUTTON_C        = 98,
    AKEYCODE_BUTTON_X        = 99,
    AKEYCODE_BUTTON_Y        = 100,
    AKEYCODE_BUTTON_Z        = 101,
    AKEYCODE_BUTTON_L1       = 102,
    AKEYCODE_BUTTON_R1       = 103,
    AKEYCODE_BUTTON_L2       = 104,
    AKEYCODE_BUTTON_R2       = 105,
    AKEYCODE_BUTTON_THUMBL   = 106,
    AKEYCODE_BUTTON_THUMBR   = 107,
    AKEYCODE_BUTTON_START    = 108,
    AKEYCODE_BUTTON_SELECT   = 109,
    AKEYCODE_BUTTON_MODE     = 110,
    AKEYCODE_ESCAPE          = 111,
    AKEYCODE_FORWARD_DEL     = 112,
    AKEYCODE_CTRL_LEFT       = 113,
    AKEYCODE_CTRL_RIGHT      = 114,
    AKEYCODE_CAPS_LOCK       = 115, /* Qt::Key_CapsLock = 0x01000024, */
    AKEYCODE_SCROLL_LOCK     = 116,
    AKEYCODE_META_LEFT       = 117,
    AKEYCODE_META_RIGHT      = 118,
    AKEYCODE_FUNCTION        = 119,
    AKEYCODE_SYSRQ           = 120,
    AKEYCODE_BREAK           = 121,
    AKEYCODE_MOVE_HOME       = 122,  /* Qt::Key_Home = 0x01000010, */
    AKEYCODE_MOVE_END        = 123,  /* Qt::Key_End = 0x01000011, */
    AKEYCODE_INSERT          = 124,
    AKEYCODE_FORWARD         = 125,
    AKEYCODE_MEDIA_PLAY      = 126,
    AKEYCODE_MEDIA_PAUSE     = 127,
    AKEYCODE_MEDIA_CLOSE     = 128,
    AKEYCODE_MEDIA_EJECT     = 129,
    AKEYCODE_MEDIA_RECORD    = 130,
    AKEYCODE_F1              = 131,
    AKEYCODE_F2              = 132,
    AKEYCODE_F3              = 133,
    AKEYCODE_F4              = 134,
    AKEYCODE_F5              = 135,
    AKEYCODE_F6              = 136,
    AKEYCODE_F7              = 137,
    AKEYCODE_F8              = 138,
    AKEYCODE_F9              = 139,
    AKEYCODE_F10             = 140,
    AKEYCODE_F11             = 141,
    AKEYCODE_F12             = 142,
    AKEYCODE_NUM_LOCK        = 143,
    AKEYCODE_NUMPAD_0        = 144,
    AKEYCODE_NUMPAD_1        = 145,
    AKEYCODE_NUMPAD_2        = 146,
    AKEYCODE_NUMPAD_3        = 147,
    AKEYCODE_NUMPAD_4        = 148,
    AKEYCODE_NUMPAD_5        = 149,
    AKEYCODE_NUMPAD_6        = 150,
    AKEYCODE_NUMPAD_7        = 151,
    AKEYCODE_NUMPAD_8        = 152,
    AKEYCODE_NUMPAD_9        = 153,
    AKEYCODE_NUMPAD_DIVIDE   = 154,
    AKEYCODE_NUMPAD_MULTIPLY = 155,
    AKEYCODE_NUMPAD_SUBTRACT = 156,
    AKEYCODE_NUMPAD_ADD      = 157,
    AKEYCODE_NUMPAD_DOT      = 158,
    AKEYCODE_NUMPAD_COMMA    = 159,
    AKEYCODE_NUMPAD_ENTER    = 160,
    AKEYCODE_NUMPAD_EQUALS   = 161,
    AKEYCODE_NUMPAD_LEFT_PAREN = 162,
    AKEYCODE_NUMPAD_RIGHT_PAREN = 163,
    AKEYCODE_VOLUME_MUTE     = 164,
    AKEYCODE_INFO            = 165,
    AKEYCODE_CHANNEL_UP      = 166,
    AKEYCODE_CHANNEL_DOWN    = 167,
    AKEYCODE_ZOOM_IN         = 168,
    AKEYCODE_ZOOM_OUT        = 169,
    AKEYCODE_TV              = 170,
    AKEYCODE_WINDOW          = 171,
    AKEYCODE_GUIDE           = 172,
    AKEYCODE_DVR             = 173,
    AKEYCODE_BOOKMARK        = 174,
    AKEYCODE_CAPTIONS        = 175,
    AKEYCODE_SETTINGS        = 176,
    AKEYCODE_TV_POWER        = 177,
    AKEYCODE_TV_INPUT        = 178,
    AKEYCODE_STB_POWER       = 179,
    AKEYCODE_STB_INPUT       = 180,
    AKEYCODE_AVR_POWER       = 181,
    AKEYCODE_AVR_INPUT       = 182,
    AKEYCODE_PROG_RED        = 183,
    AKEYCODE_PROG_GREEN      = 184,
    AKEYCODE_PROG_YELLOW     = 185,
    AKEYCODE_PROG_BLUE       = 186,
    AKEYCODE_APP_SWITCH      = 187,
    AKEYCODE_BUTTON_1        = 188,
    AKEYCODE_BUTTON_2        = 189,
    AKEYCODE_BUTTON_3        = 190,
    AKEYCODE_BUTTON_4        = 191,
    AKEYCODE_BUTTON_5        = 192,
    AKEYCODE_BUTTON_6        = 193,
    AKEYCODE_BUTTON_7        = 194,
    AKEYCODE_BUTTON_8        = 195,
    AKEYCODE_BUTTON_9        = 196,
    AKEYCODE_BUTTON_10       = 197,
    AKEYCODE_BUTTON_11       = 198,
    AKEYCODE_BUTTON_12       = 199,
    AKEYCODE_BUTTON_13       = 200,
    AKEYCODE_BUTTON_14       = 201,
    AKEYCODE_BUTTON_15       = 202,
    AKEYCODE_BUTTON_16       = 203,
    AKEYCODE_LANGUAGE_SWITCH = 204,
    AKEYCODE_MANNER_MODE     = 205,
    AKEYCODE_3D_MODE         = 206,
    AKEYCODE_CONTACTS        = 207,
    AKEYCODE_CALENDAR        = 208,
    AKEYCODE_MUSIC           = 209,
    AKEYCODE_CALCULATOR      = 210,
    AKEYCODE_ZENKAKU_HANKAKU = 211,
    AKEYCODE_EISU            = 212,
    AKEYCODE_MUHENKAN        = 213,
    AKEYCODE_HENKAN          = 214,
    AKEYCODE_KATAKANA_HIRAGANA = 215,
    AKEYCODE_YEN             = 216,
    AKEYCODE_RO              = 217,
    AKEYCODE_KANA            = 218,
    AKEYCODE_ASSIST          = 219,

    // NOTE: If you add a new keycode here you must also add it to several other files.
    //       Refer to frameworks/base/core/java/android/view/KeyEvent.java for the full list.
};

#ifdef __cplusplus
}
#endif

#endif // _ANDROID_KEYCODES_H
