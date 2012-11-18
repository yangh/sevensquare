#!/bin/sh
#
# This script used to generate key value map between Qt and Android
# key value defination. the map is used to translate Qt key value to
# to Android before send it to device.
#

# Update the keycodes.h then run this script
# Format:
#    AKEYCODE_0               = 7,  /* Qt::Key_0 = 0x30, */
#    AKEYCODE_1               = 8,  /* Qt::Key_1 = 0x31, */
#    AKEYCODE_2               = 9,  /* Qt::Key_2 = 0x32, */

SRC="src/keycodes.h"
DST="src/keymap-generated.h"

echo -n "Generate keymap header from $SRC..."

grep Qt::K $SRC | awk '{print "{ "$1 ",\t "$5" },"}' > $DST

N=`cat $DST | wc -l`

echo "$N key map generated in $DST."
