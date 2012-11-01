/*
 * adbfb.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QThread>

#include <stdint.h>
#include <zlib.h>

#include "adbfb.h"

Commander::Commander(const char *command)
{
	ret = -1;
	p = NULL;
	cmd = command;
}

void Commander::clear(void)
{
    args.clear();
    error.clear();
    output.clear();
    ret = -1;
    if (p != NULL) {
       p->close();
    }
}

int Commander::run(bool waitUntilFinished)
{
    if (p == NULL) {
        p = new QProcess();
    }

    //qDebug() << "Exec: " << cmd << " " << args;
    p->start(cmd, args);

    if (waitUntilFinished) {
        return wait();
    }

    return 0;
}

int Commander::wait(int msecs)
{
    p->waitForFinished(msecs);

    if (p->state() == QProcess::Running)
        return QProcess::Running;

    output = p->readAllStandardOutput();
    error = p->readAllStandardError();
    ret = p->exitCode();

    // FIXME: adb bug, converted '\n' (0x0A) to '\r\n' (0x0D0A)
    // while dump binary file from shell
    output.replace("\r\n", "\n");

    return QProcess::NotRunning;
}

ADB::ADB()
{
    delay = DELAY_FAST;
    connected = false;
}

ADB::~ADB()
{
    setDelay(0);
}

void ADB::loopDelay()
{
    mutex.lock();
    if (delay) {
        //DT_TRACE("DELAY" << delay);
        delayCond.wait(&mutex, delay);
    }
    mutex.unlock();
}

void ADB::setDelay(int d)
{
    mutex.lock();
    delay = d;
    delayCond.wakeAll();
    mutex.unlock();
}

FBEx::FBEx()
{
    doCompress = false;
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    fb_format = 1; //TODO:...
    bpp = FB_BPP_MAX;

    Commander cmd("which");
    cmd.addArg(MINIGZIP);
    cmd.run();
    setCompress(cmd.outputHas(MINIGZIP));
}

void FBEx::setCompress(bool value)
{
    DT_TRACE("Compressed data transfer" << value);

    if (doCompress != value) {
        doCompress = value;
        // Notify compress status changed.
    }

    if (doCompress) {
        gz.setFileName(GZ_FILE);
        gz.open(QIODevice::WriteOnly|QIODevice::Unbuffered);
        gz.resize(fb_width * fb_height * FB_BPP_MAX);
    } else {
        gz.close();
    }
}

void FBEx::setConnected(bool state)
{
    ADB::setConnected(state);

    if (state) {
	    emit newFBFound(fb_width, fb_height, fb_format, os_type);
    } else {
	    DT_TRACE("Device disconnected");
	    emit deviceDisconnected();
    }
}

static int bigEndianToInt32(const QByteArray &bytes)
{
    uint32_t v = 0;
    const char *buf = bytes.data();

    //FIXME: Assume that device and host
    // has same endianess
    bcopy(buf, &v, sizeof(uint32_t));

    return v;
}

int FBEx::AndrodDecompress(QByteArray &bytes)
{
    int ret;
    QProcess p;
    QStringList args;

    gz.seek(0);
    gz.write(bytes.data(), bytes.length());
    gz.flush();
    //DT_TRACE("DECOMP GZ TO FILE");

    args << "-d" << "-c" << GZ_FILE;
    p.start(MINIGZIP, args);
    p.waitForFinished();
    ret = p.exitCode();

#if 0
    QFile raw(RAW_FILE);
    raw.open(QIODevice::ReadOnly);
    raw.seek(0);
    bytes = raw.readAll();
    raw.close();
#endif
    bytes = p.readAllStandardOutput();

    return ret;
}

int FBEx::screenCap(QByteArray &bytes,
                    bool compress = false,
                    bool removeHeader = false)
{
    AdbExecutor adb;
    QStringList args;

    args << "shell" << "screencap";
    if (compress) {
        args << "|" << "gzip";
    }

    DT_TRACE("CAP");
    adb.run(args);
    DT_TRACE("CAP NEW FB");

    if (! adb.exitSuccess()) {
        adb.printErrorInfo();
        return adb.ret;
    }

    bytes = adb.output;

    if (compress) {
        AndrodDecompress(bytes);
    }

    if (removeHeader) {
        bytes = bytes.mid(FB_DATA_OFFSET);
    }

    return adb.ret;
}

int FBEx::convertRGBAtoRGB888(QByteArray &bytes, int offset)
{
    int x, y;
    char *p, *n;

    p = n = bytes.data() + offset;

    // RGBX32 -> RGB888
    for (y = 0; y < fb_height; y++) {
        for (x = 0; x < fb_width; x++) {
            *p++ = *n++;
            *p++ = *n++;
            *p++ = *n++;
            n++;
        }
    }

    return fb_width * fb_height * 3;
}

int FBEx::getScreenInfo(const QByteArray &bytes)
{
    int width, height, format;

    // FB header
    width = bigEndianToInt32(bytes.mid(0, 4));
    height = bigEndianToInt32(bytes.mid(4, 4));
    format = bigEndianToInt32(bytes.mid(8, 4));

    if (width <= 0 || height <= 0) {
        DT_ERROR("Failed to get screen info.");
        return -1;
    }

    fb_width = width;
    fb_height = height;
    fb_format = format;

    return 0;
}

int FBEx::getDeviceOSType(void)
{
    AdbExecutor adb;
    int os = ANDROID_ICS;

    adb.addArg("shell");
    adb.addArg("input");
    adb.run();

    if (adb.outputHas("swipe")) {
        os = ANDROID_JB;
    }
    //qDebug() << "OS type:" << os << adb.output;

    return os;
}

void FBEx::waitForDevice()
{
    if (! adbWaiter.isRunning()) {
        DT_TRACE("ADB Wait for device");
        adbWaiter.clear();
        adbWaiter.addArg("wait-for-device");
        adbWaiter.run(false);
    }

    adbWaiter.wait(500);

    if (adbWaiter.isRunning()) {
	    emit deviceWaitTimeout();
	    return;
    }

    if (adbWaiter.ret == 0) {
        DT_TRACE("ADB Found");
        emit deviceFound();
    } else {
        emit deviceWaitTimeout();
    }
}

void FBEx::sendNewFB(void)
{
    int len;

    if (bytes.length() < length()) {
        DT_ERROR("Invalid FB data len:" << bytes.length()
			<< "require" << length());
        setConnected(false);
	return;
    }

    //DT_TRACE("Send out FB");
    len = convertRGBAtoRGB888(bytes, FB_DATA_OFFSET);
    out = bytes.mid(FB_DATA_OFFSET, len);
    emit newFrame(&out);
}

void FBEx::readFrame()
{
    int ret;

    loopDelay();

    if (! isConnected())
	    return;

    ret = screenCap(bytes, doCompress, false);

    if (ret == 0) {
        sendNewFB();
    } else {
        setConnected(false);
    }
}

void FBEx::probeFBInfo()
{
    int ret;

    ret = screenCap(bytes, doCompress, false);
    if (ret != 0) {
        setConnected(false);
        return;
    }

    ret = getScreenInfo(bytes);
    if (ret != 0) {
        setConnected(false);
        return;
    }

    os_type = getDeviceOSType();

    // Only successfully probe means device connected;
    setConnected(true);

    // Also show this frame to user as soon as possible
    sendNewFB();
}
