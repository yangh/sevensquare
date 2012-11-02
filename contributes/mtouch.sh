# Unlock screen
#240 660 370 660

dev="/dev/input/event0"

adb shell sendevent $dev 3 53 $1
adb shell sendevent $dev 3 54 $2
adb shell sendevent $dev 1 330 1

adb shell sendevent $dev 3 0 $1
adb shell sendevent $dev 3 1 $2
adb shell sendevent $dev 0 0 0

adb shell sendevent $dev 3 53 $3
adb shell sendevent $dev 3 54 $4

adb shell sendevent $dev 3 0 $3
adb shell sendevent $dev 3 1 $4
adb shell sendevent $dev 0 0 0

adb shell sendevent $dev 1 330 0
adb shell sendevent $dev 0 0 0
