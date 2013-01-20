/*
 * adbfb.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QThread>
#include <QMutexLocker>
#include <QRect>

#include <stdint.h>
#include <unistd.h>
#include <zlib.h>

#include "adbfb.h"

Commander::Commander(const char *command)
{
    ret = -1;
    cmd = command;
    p = NULL;

    clear();
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
    // Create process when run enable commander
    // can run in any thread
    if (p == NULL) {
        p = new QProcess();
    }

    //DT_TRACE("CMD" << cmd << args.join(" "));
    p->start(cmd, args);

    if (waitUntilFinished) {
        return wait();
    }

    return 0;
}

int Commander::run(const QStringList &str, bool waitUntilFinished)
{
    clear();
    args << str;

    return run(waitUntilFinished);
}

int Commander::wait(const int msecs)
{
    p->waitForFinished(msecs);

    if (p->state() == QProcess::Running)
        return QProcess::Running;

    output = p->readAllStandardOutput();
    error = p->readAllStandardError();
    ret = p->exitCode();

    return QProcess::NotRunning;
}

QList<QByteArray> Commander::outputLinesHas(const char * key,
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
        //DT_TRACE("DELAY wait" << delay);
        delayCond.wait(&mutex, delay);
        //DT_TRACE("DELAY wait end" << delay);
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

AdbExecObject::AdbExecObject()
{
    lcdBrightness = 0;
    osType = ANDROID_JB;
    hasSysLCDBL = false;

    connect(this, SIGNAL(newCommand(QStringList)),
            SLOT(execCommand(QStringList)));
}

bool AdbExecObject::screenIsOn(void)
{
    if (! hasSysLCDBL) {
        return true;
    }

    return lcdBrightness > 0;
}

int AdbExecObject::getDeviceLCDBrightness()
{
    int ret;
    AdbExecutor adb;

    adb.run(QStringList() << "shell" << "cat" << SYS_LCD_BACKLIGHT);
    if (! adb.exitSuccess()) {
        emit deviceDisconnected();
        return -1;
    }

    ret = adb.output.simplified().toInt();
    //DT_TRACE("Screen brightness" << ret);

    return ret;
}

int AdbExecObject::getDeviceOSType(void)
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
    int ret;
    int oldBrightness = lcdBrightness;

    ret = getDeviceLCDBrightness();

    if (ret == lcdBrightness)
        return;

    lcdBrightness = ret;

    if (oldBrightness == 0 && ret > 0) {
        DT_TRACE("Screen is turned on");
        screenOnWaitTimer.stop();
        emit screenTurnedOn();
        return;
    }

    if (ret == 0) {
        DT_TRACE("Screen is turned off");
        screenOnWaitTimer.start();
        emit screenTurnedOff();
    }
}

bool AdbExecObject::getKeyCodeFromKeyLayout(const QString &keylayout,
                                            const char *key,
                                            int &code)
{
    AdbExecutor adb;
    QStringList args;
    QList<QByteArray> lines;

    args << "shell" << "cat";
    args << (QString(KEYLAYOUT_DIR) + keylayout + KEYLAYOUT_EXT);
    adb.run(args);

    lines = adb.outputLinesHas(key);
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray &line = lines[i];

        // make sure it's a key line
        if (line.indexOf("key") == 0) {
            QList<QByteArray> words = line.split(' ');
            if (words.size() > 1) {
                code = words[1].toInt();
                return true;
            }
        }
    }

    return false;
}

void AdbExecObject::probeDeviceHasSysLCDBL(void)
{
    AdbExecutor adb;
    QStringList args;

    args << "shell";
    args << "ls" << SYS_LCD_BACKLIGHT;

    adb.run(args);
    if (! adb.exitSuccess()) {
        emit deviceDisconnected();
        return;
    }

    hasSysLCDBL = adb.outputHas(SYS_LCD_BACKLIGHT);
}

void AdbExecObject::probeDevicePowerKey(void)
{
    int code;
    QList<QByteArray> lines;
    AdbExecutor adb;
    QStringList args;

    emit newPropmtMessae(tr("Probing device..."));

    probeDeviceHasSysLCDBL();

    if (!hasSysLCDBL) {
        return;
    } else {
        screenOnWaitTimer.setInterval(1000);
        QObject::connect(&screenOnWaitTimer, SIGNAL(timeout()),
                         this, SLOT(updateDeviceBrightness()));

    }

    osType = getDeviceOSType();

    powerKeyInfos.clear();

    // Find POWER key in the key layout files
    args << "shell" << "cat" << SYS_INPUT_NAME_LIST;
    adb.run(args);
    if (! adb.exitSuccess()) {
        emit deviceDisconnected();
        return;
    }

    // List all input device
    lines = adb.outputLines();
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i].simplified();

        if (line.length() > 0) {
            DT_TRACE("Found new input device" << line);
            powerKeyInfos.append(DeviceKeyInfo(line, i, 0));
        }
    }

    // Lookup power key define in the keylayout file with same
    // name as the input device
    for (int i = 0; i < powerKeyInfos.size(); i++) {
        DeviceKeyInfo &info = powerKeyInfos[i];

        if (getKeyCodeFromKeyLayout(info.keyLayout, "POWER", code)) {
            DT_TRACE("Found POWER key define in" << info.keyLayout << code << i);
            info.powerKeycode = code;
        } else {
            // Assign a default common power key
            DT_TRACE("Assign default POWER key define in" << info.keyLayout);
            info.powerKeycode = POWER_KEY_COMMON;
        }
    }

    // Add common power key code 116
    QList<DeviceKeyInfo> infos = powerKeyInfos;
    for (int i = 0; i < infos.size(); i++) {
        DeviceKeyInfo &info = infos[i];

        if (info.powerKeycode != POWER_KEY_COMMON) {
            DT_TRACE("ADD a dummy common key define for" << info.keyLayout);
            powerKeyInfos.append(DeviceKeyInfo(info.keyLayout,
                                               info.eventDeviceIdx,
                                               POWER_KEY_COMMON));
        }
    }

    return;
}

void AdbExecObject::wakeUpDevice()
{
    int ret;

    if (powerKeyInfos.size() == 0) {
        DT_TRACE("Power key info not found");
        probeDevicePowerKey();
    }

    if (!hasSysLCDBL) {
        return;
    }

    ret = getDeviceLCDBrightness();

    if (ret > 0) {
        // Alway notify screen state to avoid
        // can't un-freeze the screen when user pressed
        // physical key to waked up the screen
        lcdBrightness = ret;
        emit screenTurnedOn();
        return;
    }

    emit newPropmtMessae(tr("Waking up device..."));
    wakeUpDeviceViaPowerKey();

    screenOnWaitTimer.start();
}

void AdbExecObject::wakeUpDeviceViaPowerKey(void)
{
    int ret;

    // Send power key to wake up screen
    for (int i = 0; i < powerKeyInfos.size(); ++i) {
        DeviceKeyInfo &info = powerKeyInfos[i];

        DT_TRACE("Wake up screen via" << info.keyLayout
                 << info.powerKeycode << info.eventDeviceIdx);
        sendPowerKey(info.eventDeviceIdx, info.powerKeycode);

        int cnt = 3;
        while (cnt-- > 0) {
            //DT_TRACE("Wait screen on" << cnt);
            ret = getDeviceLCDBrightness();
            if (ret > 0) {
                lcdBrightness = ret;
                emit screenTurnedOn();
                break;
            }
            usleep(300*1000);
        }

        if (screenIsOn()) {
            DT_TRACE("Screen waked up by power key" << info.keyLayout << i);
            break;
        } else {
            DT_TRACE("Disable power key" << info.keyLayout << i);
            info.wakeSucessed = false;
        }
    }

    for (int i = 0; i < powerKeyInfos.size(); ++i) {
        DeviceKeyInfo &info = powerKeyInfos[i];
        if (! info.wakeSucessed) {
            powerKeyInfos.removeAt(i);
        }
    }
}

void AdbExecObject::sendVirtualClick(QPoint pos,
                                     bool press, bool release)
{
    DT_TRACE("CLICK" << pos.x() << pos.y() << press << release);

    switch(osType) {
    case ANDROID_ICS:
        sendEvent(pos, press, release);
        break;
    case ANDROID_JB:
        // Mouse move, ignored.
        // Both true is impossible
        if (press || release) {
            sendTap(pos, press);
        }
        break;
    default:
        qDebug() << "Unknown OS type, click dropped.";
    }
}

void AdbExecObject::sendTap(QPoint pos, bool press)
{
    QStringList cmds;
    bool isTap = false;

    if (press) {
        posPress = pos;
        return;
    }

    isTap = QRect(-1, -1, 2, 2).contains(pos - posPress);
    //qDebug() << "Tap as swipe" << isTap;

    cmds.clear();
    cmds << "shell";

    if (isTap) {
        cmds << "input tap";
    } else {
        cmds << "input swipe";
        cmds << QString::number(posPress.x());
        cmds << QString::number(posPress.y());
    }

    cmds << QString::number(pos.x());
    cmds << QString::number(pos.y());
    //qDebug() << cmds;

    emit newCommand(cmds);
}

QStringList AdbExecObject::newEventCmd (int type, int code, int value)
{
    QStringList event;

    event.clear();
    //TODO: Use correct dev to send event
    event << "sendevent" << "/dev/input/event0";
    event << QString::number(type);
    event << QString::number(code);
    event << QString::number(value);
    event << ";";

    return event;
}

void AdbExecObject::sendEvent(QPoint pos, bool press, bool release)
{
    QStringList cmds;

    cmds.clear();
    cmds << "shell";

    cmds << newEventCmd(3, 0x35, pos.x());
    cmds << newEventCmd(3, 0x36, pos.y());
    if (press) {
        cmds << newEventCmd(1, 0x14a, 1);
    }

    cmds << newEventCmd(3, 0, pos.x());
    cmds << newEventCmd(3, 1, pos.y());
    cmds << newEventCmd(0, 0, 0);

    if (release) {
        cmds << newEventCmd(1, 0x14a, 0);
        cmds << newEventCmd(0, 0, 0);
    }
    //DT_TRACE("Send ICS mouse event" << pos << press << release);

    emit newCommand(cmds);
}

void AdbExecObject::sendVirtualKey(int key)
{
    QStringList cmds;

    cmds.clear();
    cmds << "shell" << "input keyevent";
    cmds << QString::number(key);

    DT_TRACE("KEY" << key);
    emit newCommand(cmds);
}

FBEx::FBEx()
{
    readPaused = false;
    doCompress = false;
    screencapOptSpeed = false;
    screencapOptQuality = false;
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    fb_format = PIXEL_FORMAT_RGBA_8888;
    bpp = FB_BPP_MAX;
}

void FBEx::setPaused(bool p)
{
    readPaused = p;

    if (! readPaused) {
        setDelay(0);
    }
}

bool FBEx::checkScreenCapOptions()
{
    AdbExecutor adb;
    QStringList args;

    args << "shell";
    args << "screencap" << "-h";
    adb.run(args);

    screencapOptQuality = adb.outputHas("-q");
    screencapOptSpeed = adb.outputHas("-s");

    DT_TRACE("screencap on device options -q -s"
                << screencapOptQuality
                << screencapOptSpeed);

    return adb.exitSuccess();
}

bool FBEx::checkCompressSupport()
{
    bool ret;
    Commander cmd("which");

    cmd.addArg(MINIGZIP);
    cmd.run();
    ret = cmd.outputHas(MINIGZIP);

    enableCompress(ret);

    return cmd.exitSuccess();
}

void FBEx::enableCompress(bool value)
{
    DT_TRACE("Compressed data transfer" << value);

    if (doCompress != value) {
        doCompress = value;
        // Notify compress status changed.
    }

    if (doCompress) {
        gz.close();
        gz.setFileName(GZ_FILE);
        gz.open(QIODevice::WriteOnly|QIODevice::Unbuffered);
        gz.resize(fb_width * fb_height * FB_BPP_MAX);
    } else {
        gz.close();
    }
}

void FBEx::setConnected(bool state)
{
#if 0
    if (isConnected() == state) {
        return;
    }
#endif

    ADB::setConnected(state);

    if (state) {
        emit newFBFound(fb_width, fb_height, fb_format);
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

int FBEx::minigzipDecompress(QByteArray &bytes)
{
    Commander cmd(MINIGZIP);
    QStringList args;

    gz.seek(0);
    gz.write(bytes.data(), bytes.length());
    gz.flush();
    //DT_TRACE("DECOMP GZ TO FILE");

    args << "-d" << "-c" << GZ_FILE;
    cmd.run(args);

    bytes = cmd.output;

    return cmd.ret;
}

int FBEx::screenCap(QByteArray &bytes, int offset)
{
    AdbExecutor adb;
    QStringList args;
    qint64 mSecs;

    args << "shell" << "screencap";

    // TODO: Add UI config for this option
    if (screencapOptQuality) {
        args << "-q";
    }

    if (doCompress) {
        args << "|" << "gzip";
    }

    mSecs = QDateTime::currentMSecsSinceEpoch();
    adb.run(args);
    mSecs = QDateTime::currentMSecsSinceEpoch() - mSecs;
    DT_TRACE("CAP FB in" << mSecs << "ms");

    if (! adb.exitSuccess()) {
        adb.printErrorInfo();
        return adb.ret;
    }

    bytes = adb.outputFixNewLine();

    if (doCompress) {
        minigzipDecompress(bytes);
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
    case PIXEL_FORMAT_RGBA_8888:
    case PIXEL_FORMAT_RGBX_8888:
        bpp = 4; // RGBA32
        break;
    default:
        DT_ERROR("Unknown fb format " << format);
        return -1;
    }

    return 0;
}

void FBEx::waitForDevice()
{
    // Ignore request if is connected already
    if (isConnected()) {
        return;
    }

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
    if ((fb_format == PIXEL_FORMAT_RGBA_8888)
        || (fb_format == PIXEL_FORMAT_RGBX_8888)) {
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

    if (! isConnected() || paused())
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

    checkCompressSupport();
    checkScreenCapOptions();

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

    // Only successfully probe means device connected;
    setConnected(true);

    // Also show this frame to user as soon as possible
    sendNewFB();
}
