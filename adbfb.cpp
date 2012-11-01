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

void AdbExecutor::clear(void)
{
    args.clear();
    error.clear();
    output.clear();
    ret = -1;
}

int AdbExecutor::run(bool waitUntilFinished)
{
    //qDebug() << "Exec: " << cmd << " " << args;
    p.start(cmd, args);

    if (waitUntilFinished) {
        return wait();
    }

    return 0;
}

int AdbExecutor::wait(int msecs)
{
    p.waitForFinished(msecs);

    if (p.state() == QProcess::Running)
        return QProcess::Running;

    output = p.readAllStandardOutput();
    error = p.readAllStandardError();
    ret = p.exitCode();

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
        DT_TRACE("DELAY" << delay);
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
    do_compress = false;
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    fb_format = 1; //TODO:...
    bpp = FB_BPP_MAX;
}

void FBEx::setCompress(bool value)
{
    if (do_compress != value) {
        do_compress = value;
        // Notify compress status changed.
    }

    if (do_compress) {
        gz.setFileName(GZ_FILE);
        gz.open(QIODevice::WriteOnly|QIODevice::Unbuffered);
        gz.resize(fb_width * fb_height * FB_BPP_MAX);
    } else {
        gz.close();
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

    gz.seek(0);
    gz.write(bytes.data(), bytes.length());
    gz.flush();
    //DT_TRACE("DECOMP GZ TO FILE");

    p.start("minigzip", QStringList() << "-d" << "-c" << GZ_FILE);
    p.waitForFinished();
    ret = p.exitCode();

    bytes = p.readAllStandardOutput();
    //DT_TRACE("DECOMP FINISHED");
#if 0
    //TODO: Use zlib to uncompress data, instead external cmd
    // The follow code ret = 3, something is wrong here.
    uLongf len = dest.length();
    ret = uncompress ((Bytef*) dest.data(), &len,
                      (Bytef*) src.data(), src.length());
    qDebug() << "Uncompress ret:" << ret << len;
#endif
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
        qDebug() << "Failed to get screen info.";
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

    if (adb.output.indexOf("swipe") > 0) {
        os = ANDROID_JB;
    }
    //qDebug() << "OS type:" << os << adb.output;

    return os;
}

void FBEx::waitForDevice()
{
    AdbExecutor adb;
    adb.addArg("wait-for-device");
    adb.run();

    if (adb.ret == 0) {
        DT_TRACE("ADB found");
        emit deviceFound();
    }
}

void FBEx::readFrame()
{
    int ret, len;

    loopDelay();

    ret = screenCap(bytes, do_compress, false);

    if (ret == 0) {
        len = convertRGBAtoRGB888(bytes, FB_DATA_OFFSET);
        out = bytes.mid(FB_DATA_OFFSET, len);
        emit newFrame(&out);
    } else {
        setConnected(false);
        emit deviceDisconnected();
    }
}

void FBEx::probeFBInfo()
{
    int ret, os;

    ret = screenCap(bytes, do_compress, false);
    if (ret != 0) {
        setConnected(false);
        emit deviceDisconnected();
        return;
    }

    ret =  getScreenInfo(bytes);
    if (ret != 0) {
        return;
    }

    setConnected(true);
    os = getDeviceOSType();

    emit newFBFound(fb_width, fb_height, fb_format, os);
}
