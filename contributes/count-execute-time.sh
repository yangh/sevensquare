#!/bin/bash

echo "time adb shell screencap /dev/x"
time adb shell screencap /dev/x

echo "time adb shell 'screencap | gzip > /dev/x'"
time adb shell 'screencap | gzip > /dev/x'

echo "time adb shell screencap  > x"
time adb shell screencap  > x

echo "time adb shell 'screencap | gzip ' > x"
time adb shell 'screencap | gzip ' > x

echo "time adb shell 'screencap | gzip -f' > x"
time adb shell 'screencap | gzip -f' > x

echo "time adb shell 'screencap | gzip -h' > x"
time adb shell 'screencap | gzip -h' > x

echo "time adb shell 'screencap | gzip -r' > x"
time adb shell 'screencap | gzip -r' > x

