Seven Sqaure
============

Android screencast wroted in QT. There is one in java? forget it from now on.

A tool used to view a mirror screen of your Android device on the PC,
used for demostration, debuging, and so on. even you can click on the
mirror screen as you touch on the device panel.

[Screenshot](screenshots/seven-square-screenshot.png)

Tested host: Linux, Windows.
Tested device: QRD8x25/8x26, Linaro Panda board (OMAP4430), TCL Idol X (MTK6589)
Tested Android: 4.0, 4.1, 4.2

Features
--------

Major:

 * Click and swipe on the screen (so you can unlock screen by swipe)
 * Virtual menu/home/back key on the UI
 * QWERTY key support (just type via your keyboard)
 * Auto connect to your device when plug in
 * Click on window to wakeup the device (emulate power key press event)
 * Support both Android ICS and Jelly Bean. Froyo not full tested
 * Wrote in Qt 4.x, easily port to different desktop environment

Minor:

 * Rotate view windows on demand (Press F12)
 * Virtual cursor (pointer anchor) on the UI
 * Auto resize to fit the screen resolution of the device
 * Zoom window as your wish and keep the ratio
 * Support customized 'screencap' for speeding up (RGBA32, RGB888, RGB565)
 * Auto enable compressed data transfer if 'minigzip' found on the host
 * 1~2FPS on MSM8625 device (1.2G dual core, 512M memory, 800x480)

Non-feature:

 * Not support multi devices pluged in at the same time.
 * Not support screen recording as video
 * Not support screen shot as picture

Additional Key Map
==================

You can press F1/2/3/4 on you keyboard to emulate the following Android key.

 * For device:

        F1 AKEYCODE_HOME
        F2 AKEYCODE_BACK
        F3 AKEYCODE_CALL
        F4 AKEYCODE_ENDCALL

 * For application on PC:

        F12 Rotate view window (Landscape/Portrait)

Pre-requirements
================

Mandatory
---------

 * Linux, any distribution with Qt package.

 * Qt 4.x+ installed in your system

 * screencap command on the target device

        $> adb shell ls /system/bin/screencap
        /system/bin/screencap

   If the output indicates: 'No such file or direcotry', you
   can use the tool except install one by yourself.

   the 'screencap' source code presents in frameworks/base/cmds/screencap.

 * PC: adb command is availible in your search path, check it:

        $> which adb
        /usr/bin/adb

 * USB cable connected to your android device

Optional
--------

 * Android device in engineer mode (Rooted), So that you can get
   root permission in adb shell

        $> adb root
        $> adb shell id
        uid=0(root) gid=0(root)

 * minigzip installed on your PC (For compressed image transfer)
  
     There is a precompiled x86_64 minigzip binary in the contirbutes dir,
     just copy into your system path:

        $> sudo cp contributes/minigzip /usr/bin

     If you want to compile one for yourself, get it from android/external/zlib:

        $> cp -rf android/external/zlib zlib-pc
        $> cd zlib-pc
        $> ./configure
        $> make
        $> sudo cp minigzip /usr/bin

You'll be appreciated If you can help to add decompress code in to this
project to help to avoid runing external program to decompress frame buffer data.

Compile
=======

Install dependences and just make:

    $> apt-get install qt4-qmake libqt4-dev libqtcore4 libqtgui4
    $> make

Run from source tree:

    $> ./build/seven-square

Install into system wide:

    $> make install

Or you can create a short cut on your desktop.

Keypad support
==============

 If you want support more key define in the UI, please read
 the contributes/generate-keymap.sh for more info.

History
=======

'Seven Square' was a game I played when I was a child, and I wrote a QT version
for it, in the branch sevensquare-game, [screenshot]:(screenshots/Screenshot-sq.png).
And now it's the base code of this tool.

Author
======

 Yang Hong <yanghong@thundersoft.com>

 Copyright reserved.

 Released under GPL2.

Contributors
============

 Eric Wang, Helped to porting to Windows platform

References
==========

 * Android screencast (Java)

   http://code.google.com/p/androidscreencast/

 * Droid@Screen (Java)

   http://blog.ribomation.com/droid-at-screen/
