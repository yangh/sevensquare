#!/bin/sh

# Lookup 'POWER' key in the /system/usr/keylayout/*.kl
IDX=$1

# Find out the input device file
POWER=$2

# Send out the power key event
adb shell sendevent /dev/input/event$IDX 1 $POWER 1
adb shell sendevent /dev/input/event$IDX 1 $POWER 0
adb shell sendevent /dev/input/event$IDX 0 0 0

