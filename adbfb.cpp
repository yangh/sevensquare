/*
 * adbfb.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QThread>
#include <QMutexLocker>

#include <stdint.h>
#include <unistd.h>
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

    qDebug() << "Exec: " << cmd << " " << args;
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

QList<QByteArray> Commander::outputLineHas(const char * key,
                                           bool ignoreComment)
{
    QList<QByteArray> lines;
    QList<QByteArray> matches;

    if (output.length() == 0)
            return matches;

    lines = output.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i];

        if (ignoreComment && line[0] == '#')
            continue;

        if (line.indexOf(key) > 0) {
                matches.append(line);
        }
    }

    return matches;
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
    QMutexLocker locker(&mutex);

    if (delay) {
        //DT_TRACE("DELAY" << delay);
        delayCond.wait(&mutex, delay);
    }
    mutex.unlock();
}

void ADB::setDelay(int d)
{
    QMutexLocker locker(&mutex);

    delay = d;
    delayCond.wakeAll();
    mutex.unlock();
}

int ADB::increaseDelay()
{
    QMutexLocker locker(&mutex);

    if (delay < DELAY_MAX)
        delay += DELAY_STEP;

    return delay;
}

int AdbExecObject::getDeviceLCDBrightness()
{
    AdbExecutor adb;

    adb.run(QStringList() << "shell" << "cat" << SYS_LCD_BACKLIGHT);
    if (! adb.exitSuccess())
        return -1;

    lcdBrightness = adb.output.simplified().toInt();
    //DT_TRACE("Screen brightness" << lcdBrightness);

    return lcdBrightness;
}

QStringList AdbExecObject::newKeyEventCommand(int deviceIdx,
                                              int type,
                                              int code,
                                              int value)
{
    QStringList event;

    event << "sendevent";
    event << (QString(INPUT_DEV_PREFIX) + QString::number(deviceIdx));
    event << QString::number(type);
    event << QString::number(code);
    event << QString::number(value);
    event << ";";

    return event;
}

QStringList AdbExecObject::newKeyEventCommandSequence(int deviceIdx, int code)
{
    QStringList cmds;

    cmds << newKeyEventCommand(deviceIdx, 1, code, 1);
    cmds << newKeyEventCommand(deviceIdx, 1, code, 0);
    cmds << newKeyEventCommand(deviceIdx, 0, 0, 0);

    return cmds;
}

void AdbExecObject::sendPowerKey(int deviceIdx, int code)
{
    AdbExecutor adb;
    QStringList args;

    args << "shell";
    args << newKeyEventCommandSequence(deviceIdx, code);
    adb.run(args);
}


void AdbExecObject::updateDeviceBrightness(void)
{
    getDeviceLCDBrightness();

    if (lcdBrightness == 0) {
        DT_TRACE("Screen is turned off");
        emit screenTurnedOff();
    }
}

bool AdbExecObject::getKeyCodeFromKeyLayout(const QString &keylayout,
                                            const char *key,
                                            int &code)
{
    AdbExecutor adb;
    QList<QByteArray> lines;

    adb.run(QStringList() << "shell"
            << "cat" << (QString(KEYLAYOUT_DIR) + keylayout));

    lines = adb.outputLineHas(key);
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray &line = lines[i];

        // make sure it's a key line
        if (line.indexOf("key") == 0) {
            QList<QByteArray> words = line.split(' ');
            code = words[1].toInt();
            return true;
        }
    }

    return false;
}

void AdbExecObject::probeDevicePowerKey(void)
{
    int code;
    QList<QByteArray> lines;
    AdbExecutor adb;

    // Get it as soon as possible
    getDeviceLCDBrightness();

    // Find POWER key in the key layout files
    adb.run(QStringList() << "shell" << "ls" << KEYLAYOUT_DIR);
    if (! adb.exitSuccess())
        return;

    lines = adb.output.split('\n');

    for (int i = 0; i < lines.size(); ++i) {
        QByteArray &line = lines[i];

        if (getKeyCodeFromKeyLayout(line, "POWER", code)) {
            QString layout = line.mid(0, line.length() - 3).data();

            DT_TRACE("Found POWER key define in" << line << code);
            keyInfos.append(DeviceKeyInfo(layout,
                                          DeviceKeyInfo::DEVICE_IDX_MAX,
                                          code));
        }
    }

    // Find input device index from /proc
    adb.run(QStringList() << "shell" << "cat" << PROC_INPUT_DEVICES);
    if (! adb.exitSuccess())
        return;

    lines = adb.outputLineHas("Name=");

    for (int idx = 0; idx < keyInfos.size(); idx++) {
        DeviceKeyInfo &info = keyInfos[idx];

        for (int i = 0; i < lines.size(); ++i) {
            QByteArray &line = lines[i];

            if (line.indexOf(info.keyLayout) > 0) {

                info.eventDeviceIdx = i;
                DT_TRACE("Power key from" << info.keyLayout <<
                         info.powerKeycode << info.eventDeviceIdx);
                continue;
            }
        }
    }

    // Delete power key code without device found
    for (int idx = 0; idx < keyInfos.size(); idx++) {
        DeviceKeyInfo &info = keyInfos[idx];

        if (info.eventDeviceIdx == DeviceKeyInfo::DEVICE_IDX_MAX) {
            keyInfos.removeAt(idx);
        }
    }

    // Wake up on probe
    if (keyInfos.size() > 0){
        wakeUpDevice();
    }

    return;
}

void AdbExecObject::wakeUpDevice()
{
    int ret;

    if (keyInfos.size() == 0) {
        DT_TRACE("Power key info not found");
        return;
    }

    ret = getDeviceLCDBrightness();

    if (ret > 0) {
        // Alway notify screen state to avoid
        // can't un-freeze the screen when user pressed
        // physcle key to waked up the screen
        emit screenTurnedOn();
        return;
    }

    // Send power key to wake up screen
    for (int i = 0; i < keyInfos.size(); ++i) {
        DeviceKeyInfo &info = keyInfos[i];

        DT_TRACE("Wake up screen via" << info.keyLayout
                 << info.powerKeycode << info.eventDeviceIdx);
        sendPowerKey(info.eventDeviceIdx, info.powerKeycode);

        usleep(300 * 1000);
        if (getDeviceLCDBrightness() > 0) {
            emit screenTurnedOn();
            break;
        }
    }
}

FBEx::FBEx()
{
    doCompress = false;
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    fb_format = PIXEL_FORMAT_RGBX_8888;
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

    bytes = p.readAllStandardOutput();

    return ret;
}

int FBEx::screenCap(QByteArray &bytes, int offset = 0)
{
    AdbExecutor adb;
    QStringList args;

    args << "shell" << "screencap -s";
    if (doCompress) {
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

    if (doCompress) {
        AndrodDecompress(bytes);
    }

    if (offset) {
        bytes = bytes.mid(offset);
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

    switch(format) {
    case PIXEL_FORMAT_RGBX_565:
        bpp = 2; // RGB565
	break;
    case PIXEL_FORMAT_RGB_888:
        bpp = 3; // RGB888
	break;
    case PIXEL_FORMAT_RGBX_8888:
        bpp = 4; // RGBA32
        break;
    default:
        DT_ERROR("Unknown fb format " << format);
        return -1;
    }

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
    if (fb_format == PIXEL_FORMAT_RGBX_8888) {
	    len = convertRGBAtoRGB888(bytes, FB_DATA_OFFSET);
    } else {
	    len = length();
    }

    out = bytes.mid(FB_DATA_OFFSET, len);
    emit newFrame(&out);
}

void FBEx::readFrame()
{
    int ret;

    loopDelay();

    if (! isConnected())
	    return;

    ret = screenCap(bytes);

    if (ret == 0) {
        sendNewFB();
    } else {
        setConnected(false);
    }
}

void FBEx::probeFBInfo()
{
    int ret;

    ret = screenCap(bytes);
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
