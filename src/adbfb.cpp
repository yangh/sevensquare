/*
 * adbfb.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#include <QThread>
#include <QMutexLocker>
#include <QRect>

#include <unistd.h>

#include "adbfb.h"
#include "utils.h"

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

ADBBase::ADBBase()
{
    delay = DELAY_FAST;
    connected = false;
}

ADBBase::~ADBBase()
{
    setDelay(0);
}

void ADBBase::loopDelay()
{
    QMutexLocker locker(&mutex);

    if (delay) {
        //DT_TRACE("DELAY wait" << delay);
        delayCond.wait(&mutex, delay);
        //DT_TRACE("DELAY wait end" << delay);
    }
}

void ADBBase::setDelay(int d)
{
    QMutexLocker locker(&mutex);

    delay = d;
    delayCond.wakeAll();
}

int ADBBase::increaseDelay()
{
    QMutexLocker locker(&mutex);

    if (delay < DELAY_MAX)
        delay += DELAY_STEP;

    return delay;
}

ADBDevice::ADBDevice()
{
    osType = ANDROID_JB;
    lcdBrightness = -1;
    hasSysLCDBL = false;

    screenOnWaitTimer.setInterval(SCREENON_WAIT_INTERVAL);
    QObject::connect(&screenOnWaitTimer, SIGNAL(timeout()),
                     this, SLOT(updateDeviceBrightness()));


    QObject::connect(this, SIGNAL(newCommand(QStringList)),
                     this, SLOT(execCommand(QStringList)));

    posOfPress = QPoint(-1, -1);
    touchPanel = DeviceKeyInfo();
}

bool ADBDevice::screenIsOn(void)
{
    if (! hasSysLCDBL) {
        return true;
    }

    return lcdBrightness > 0;
}

int ADBDevice::getDeviceLCDBrightness()
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

QStringList ADBDevice::newKeyEventCommand(int deviceIdx,
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

QStringList ADBDevice::newKeyEventCommandSequence(int deviceIdx, int code)
{
    QStringList cmds;

    cmds << newKeyEventCommand(deviceIdx, 1, code, 1);
    cmds << newKeyEventCommand(deviceIdx, 1, code, 0);
    cmds << newKeyEventCommand(deviceIdx, 0, 0, 0);

    return cmds;
}

void ADBDevice::sendPowerKey(int deviceIdx, int code)
{
    AdbExecutor adb;
    QStringList args;

    args << "shell";
    args << newKeyEventCommandSequence(deviceIdx, code);
    adb.run(args);
}

void ADBDevice::updateDeviceBrightness(void)
{
    int ret;
    int oldBrightness = lcdBrightness;

    if (! hasSysLCDBL)
        return;

    ret = getDeviceLCDBrightness();

    //DT_TRACE("Screen rightness" << oldBrightness << ret);
    if (ret == lcdBrightness)
        return;

    lcdBrightness = ret;

    if (oldBrightness == 0 && ret > 0) {
        DT_TRACE("Screen turned on");
        emit screenTurnedOn();
        return;
    }

    if (ret == 0) {
        DT_TRACE("Screen turned off");
        emit screenTurnedOff();
    }
}

bool ADBDevice::getKeyCodeFromKeyLayout(const QString &keyLayout,
                                        const char *key,
                                        int &code)
{
    AdbExecutor adb;
    QStringList args;
    QList<QByteArray> lines;

    args << "shell" << "cat";
    args << (QString(KEYLAYOUT_DIR) + keyLayout + KEYLAYOUT_EXT);
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

void ADBDevice::probeDevice(void)
{
    emit newPropmtMessae(tr("Probing device..."));

    probeDeviceOSType();
    probeInputDevices();
    probeDeviceHasSysLCDBL();
}

int ADBDevice::probeDeviceOSType(void)
{
    AdbExecutor adb;
    int os = ANDROID_ICS;

    adb.addArg("shell");
    adb.addArg("getprop");
    adb.addArg("ro.build.version.sdk");

    adb.run();

    if (adb.output.simplified().toInt() >= 16) {
        os = ANDROID_JB;
    }
    //qDebug() << "OS type:" << os << adb.output;

    osType = os;

    return os;
}

void ADBDevice::probeDeviceHasSysLCDBL(void)
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

    hasSysLCDBL = ! adb.outputHas(NO_SUCH_FILE);
    DT_TRACE("Device has sys LCD brightness API:" << hasSysLCDBL);

    if (hasSysLCDBL) {
        updateDeviceBrightness();
        screenOnWaitTimer.start();
    } else {
        screenOnWaitTimer.stop();
        emit screenTurnedOn();
    }
}

bool ADBDevice::getInputDeviceInfo(DeviceKeyInfo &info, const QByteArray &sysPath)
{
    AdbExecutor adb;
    QStringList args;
    bool ret;

    info.eventDeviceIdx = sysPath.mid(SYS_INPUT_INDEX_OFFSET).toInt(&ret);

    args << "shell" << "cat" << sysPath + "/name";
    adb.run(args);
    info.keyLayout = adb.output.simplified();

    args.clear();
    args << "shell" << "cat" << sysPath + "/capabilities/ev";
    adb.run(args);
    info.evBit = adb.output.simplified().toInt(&ret, 16);

    DT_TRACE("Input device info" << info.eventDeviceIdx
             << info.keyLayout << info.evBit << adb.output.simplified());

    return true;
}

void ADBDevice::probeInputDevices(void)
{
    AdbExecutor adb;
    QStringList args;
    QList<QByteArray> lines;

    args << "shell" << "ls -d" << QString(SYS_INPUT_DIR) + "input*";
    adb.run(args);

    if (! adb.exitSuccess()) {
        emit deviceDisconnected();
        return;
    }

    powerKeyInfos.clear();

    // List all input device
    lines = adb.outputLines();
    for (int i = 0; i < lines.size(); ++i) {
        QByteArray line = lines[i].simplified();
        DeviceKeyInfo info;
        int code;

        if (line.length() == 0)
            continue;

        //DT_TRACE("Found new input device" << line);
        getInputDeviceInfo(info, line);

        if (EV_IS_TOUCHSCREEN(info.evBit)) {
            DT_TRACE("Found touch panel" << info.keyLayout << info.eventDeviceIdx);
            touchPanel = info;
        } else if (EV_IS_KEY(info.evBit) && (! EV_IS_MOUSE(info.evBit))) {
            DT_TRACE("Found key input" << info.keyLayout << info.eventDeviceIdx);

            if (getKeyCodeFromKeyLayout(info.keyLayout, "POWER", code)) {
                DT_TRACE("Found POWER key define in" << info.keyLayout << code << i);
                info.powerKeycode = code;
                powerKeyInfos.append(info);

                // Also add a common power key for the device
                powerKeyInfos.append(DeviceKeyInfo(info.keyLayout,
                                                   info.eventDeviceIdx,
                                                   POWER_KEY_COMMON));
            } else {
                info.powerKeycode = POWER_KEY_COMMON;
                powerKeyInfos.append(info);
            }
        } else {
            DT_TRACE("Found unknown input" << info.keyLayout << info.eventDeviceIdx);
        }
    }
}

void ADBDevice::wakeUpDevice()
{
    int ret;

    if (powerKeyInfos.size() == 0) {
        DT_TRACE("Power key info not found");
        probeDevice();
    }

    if (!hasSysLCDBL) {
        DT_TRACE("LCD Brightness sys file not found");
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
}

void ADBDevice::wakeUpDeviceViaPowerKey(void)
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
            //DT_TRACE("Disable power key" << info.keyLayout << i);
            info.wakeSucessed = false;
        }
    }

    QList<DeviceKeyInfo> newKeyInfos;

    for (int i = 0; i < powerKeyInfos.size(); ++i) {
        DeviceKeyInfo &info = powerKeyInfos[i];
        if (info.wakeSucessed) {
            newKeyInfos.append(info);
        } else {
            DT_TRACE("Disable power key:" << i << info.keyLayout << info.powerKeycode);
        }
    }

    powerKeyInfos = newKeyInfos;

    // Device can't wakeuped automaticly
    if (powerKeyInfos.size() == 0) {
        DT_TRACE("Disable wakeup via click because target not support");
        hasSysLCDBL = false;
        screenOnWaitTimer.stop();
        emit screenTurnedOn();
    }
}

void ADBDevice::sendVirtualClick(QPoint pos,
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

void ADBDevice::sendTap(QPoint pos, bool press)
{
    QStringList cmds;
    bool isTap = false;

    if (press) {
        posOfPress = pos;
        return;
    }

    // Moved distance less than 2 point
    isTap = QRect(-1, -1, 2, 2).contains(pos - posOfPress);
    //qDebug() << "Tap as swipe" << isTap;

    cmds.clear();
    cmds << "shell";

    if (isTap) {
        cmds << "input tap";
    } else {
        cmds << "input swipe";
        cmds << QString::number(posOfPress.x());
        cmds << QString::number(posOfPress.y());
    }

    cmds << QString::number(pos.x());
    cmds << QString::number(pos.y());
    //qDebug() << cmds;

    emit newCommand(cmds);
}

QStringList ADBDevice::newEventCmd (int type, int code, int value)
{
    QStringList event;

    event.clear();
    //TODO: Use correct dev to send event
    event << "sendevent";
    event << (QString(INPUT_DEV_PREFIX) + QString::number(touchPanel.eventDeviceIdx));
    event << QString::number(type);
    event << QString::number(code);
    event << QString::number(value);
    event << ";";

    return event;
}

void ADBDevice::sendEvent(QPoint pos, bool press, bool release)
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

void ADBDevice::sendVirtualKey(int key)
{
    QStringList cmds;

    cmds.clear();
    cmds << "shell" << "input keyevent";
    cmds << QString::number(key);

    DT_TRACE("KEY" << key);
    emit newCommand(cmds);
}

ADBFrameBuffer::ADBFrameBuffer()
{
    readPaused = false;
    doCompress = false;
    screencapExists = false;
    screencapOptSpeed = false;
    screencapOptQuality = false;
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    fb_format = PIXEL_FORMAT_RGBA_8888;
    bpp = FB_BPP_MAX;
    invalid_buffer_count = 0;
}

void ADBFrameBuffer::setPaused(bool p)
{
    readPaused = p;

    if (! readPaused) {
        setDelay(0);
    }
}

bool ADBFrameBuffer::checkScreenCapOptions()
{
    AdbExecutor adb;
    QStringList args;

    args << "shell" << "ls" << SCREENCAP_EXEC;
    adb.run(args);
    screencapExists = ! adb.outputHas(NO_SUCH_FILE);

    if (! screencapExists) {
        DT_TRACE("Error: no screencap command on device!");
        return false;
    }

    args.clear();
    args << "shell";
    args << SCREENCAP_EXEC << "-h";
    adb.run(args);

    screencapOptQuality = adb.outputHas("-q");
    screencapOptSpeed = adb.outputHas("-s");

    DT_TRACE("Screencap on device options -q -s"
             << screencapOptQuality
             << screencapOptSpeed);

    return adb.exitSuccess();
}

bool ADBFrameBuffer::checkCompressSupport()
{
    bool ret;
    Commander cmd("which");

    cmd.addArg(MINIGZIP);
    cmd.run();
    ret = cmd.outputHas(MINIGZIP);

    enableCompress(ret);

    return cmd.exitSuccess();
}

void ADBFrameBuffer::enableCompress(bool value)
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

void ADBFrameBuffer::setConnected(bool state)
{
#if 0
    if (isConnected() == state) {
        return;
    }
#endif

    ADBBase::setConnected(state);

    if (state) {
        emit newFBFound(fb_width, fb_height, fb_format);
    } else {
        DT_TRACE("Device disconnected");
        emit deviceDisconnected();
    }
}

int ADBFrameBuffer::minigzipDecompress(QByteArray &bytes)
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

int ADBFrameBuffer::screenCap(QByteArray &bytes, int offset)
{
    AdbExecutor adb;
    QStringList args;
    qint64 mSecs;

    args << "shell" << SCREENCAP_EXEC;

    // TODO: Add UI config for this option
    if (screencapOptQuality) {
        args << "-q";
    }

    if (screencapOptSpeed) {
        args << "-s";
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

    //DT_TRACE("BUF Original len: " << adb.output.length());
    bytes = adb.outputFixNewLine();
    //DT_TRACE("BUF fixed new line: " << bytes.length());

    if (doCompress) {
        minigzipDecompress(bytes);
    }

    if (offset) {
        bytes = bytes.mid(offset);
    }

    return adb.ret;
}

int ADBFrameBuffer::getScreenInfo(int &w, int &h, int &f)
{
    int width, height, format;
    const char *data;

    // FB header
    data = bytes.data();

    width = bigEndianStreamDataToInt32(data);
    height = bigEndianStreamDataToInt32(data + 4);
    format = bigEndianStreamDataToInt32(data + 8);

    // Assume that screen width will less than 5120
    if (width > IMPOSSIBLE_FB_WIDTH) {
        // Maybe little ending
        DT_TRACE("Got an impossible fb info: " << width << height
                     << ", try different endianness");
        width = littleEndianStreamDataToInt32(data);
        height = littleEndianStreamDataToInt32(data + 4);
        format = littleEndianStreamDataToInt32(data + 8);
    }

    if (width > IMPOSSIBLE_FB_WIDTH || width <= 0 || height <= 0) {
        DT_ERROR("Failed to get screen info: " << width << height);
        return -1;
    }

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

    w = width;
    h = height;
    f = format;

    return 0;
}

void ADBFrameBuffer::waitForDevice()
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

    adbWaiter.wait(ADB_WAIT_INTERVAL_MIN);

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

void ADBFrameBuffer::sendNewFB(void)
{
    int len;
    int ret;
    int w, h, f;

    // screencap will output diff format in diff scene
    // so we always check the fb format
    ret = getScreenInfo(w, h, f);
    if (ret == 0) {
        if (fb_format != f) {
            DT_TRACE("New Remote screen FB format:" << fb_format << f);
            fb_format = f;
            emit newFBFormat(fb_format);
        }
    } else {
        setConnected(false);
        return;
    }

    if (bytes.length() < length()) {
        if (invalid_buffer_count < INVALIDE_BUFFER_MAX) {
            invalid_buffer_count++;
            DT_TRACE("Invalid FB data len:" << bytes.length()
                 << "require" << length());
        } else {
            invalid_buffer_count = 0;
            setConnected(false);
            return;
	}
    } else {
            invalid_buffer_count = 0;
    }

    //DT_TRACE("Send out FB");
    // QImage doesn't support RGBA/RGBX format
    if ((fb_format == PIXEL_FORMAT_RGBA_8888)
            || (fb_format == PIXEL_FORMAT_RGBX_8888))
    {
        len = convertRGBAtoRGB888(bytes.data(),
                                  fb_width, fb_height,
                                  FB_DATA_OFFSET);
    } else {
        len = length();
    }

    out = bytes.mid(FB_DATA_OFFSET, len);
    emit newFrame(&out);
}

void ADBFrameBuffer::readFrame()
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

void ADBFrameBuffer::probeFBInfo()
{
    int ret;

    checkCompressSupport();
    checkScreenCapOptions();

    if (! screencapExists) {
        DT_TRACE("Connect canceled");
        emit newPropmtMessae(tr("Error: no screencap"));

        DT_TRACE("Delay");
        usleep(ADB_WAIT_INTERVAL_MAX);

        setConnected(false);
        return;
    }

    ret = screenCap(bytes);
    if (ret != 0) {
        setConnected(false);
        return;
    }

    ret = getScreenInfo(fb_width, fb_height, fb_format);
    if (ret != 0) {
        setConnected(false);
        return;
    }

    emit newFBProbed();

    // Only successfully probe means device connected;
    setConnected(true);

    // Also show this frame to user as soon as possible
    sendNewFB();
}
