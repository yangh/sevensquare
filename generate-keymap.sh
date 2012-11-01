#!/bin/sh

# Update the keycodes.h then run this script
# Format:
#    AKEYCODE_0               = 7,  /* Qt::Key_0 = 0x30, */
#    AKEYCODE_1               = 8,  /* Qt::Key_1 = 0x31, */
#    AKEYCODE_2               = 9,  /* Qt::Key_2 = 0x32, */

grep Qt::K keycodes.h | awk '{print "{ "$1 ",\t "$5" },"}' > keymap-generated.h
