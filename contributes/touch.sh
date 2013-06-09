
# Usage: touch.sh  X Y

# TODO: need to find out touch device name
dev="/dev/input/event3"

adb shell sendevent $dev 3 53 $1
adb shell sendevent $dev 3 54 $2
adb shell sendevent $dev 1 330 1

adb shell sendevent $dev 3 0 $1
adb shell sendevent $dev 3 1 $2
adb shell sendevent $dev 0 0 0

adb shell sendevent $dev 1 330 0
adb shell sendevent $dev 0 0 0
