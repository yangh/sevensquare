# -------------------------------------------------
# Project created by QtCreator 2010-01-03T14:36:11
# -------------------------------------------------
#QT += opengl \
#    script \
#    svg
QT +=widgets
TARGET = seven-square
TEMPLATE = app
CONFIG += debug release
RESOURCES = seven-square.qrc
SOURCES += \
    src/main.cpp \
    src/adbfb.cpp \
    src/cubecellitem.cpp \
    src/fbcellitem.cpp \
    src/cubescene.cpp \
    src/utils.cpp
HEADERS += \
    src/cubecellitem.h \
    src/adbfb.h \
    src/input-event-types.h \
    src/keymap.h \
    src/keymap-generated.h \
    src/keycodes.h \
    src/adbfb.h \
    src/debug.h \
    src/fbcellitem.h \
    src/cubescene.h \
    src/utils.h
