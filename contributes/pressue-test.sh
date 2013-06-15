#!/bin/sh

while [ true ]; do
  echo -n "."

  # home
  adb shell input keyevent 3
  sleep 1

  # phone key
  adb shell input keyevent 5

  # start launcher
  #adb shell input tap 240 780
  sleep 1
done

