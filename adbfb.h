/*
 * adbfb.h
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#ifndef ADBFB_H
#define ADBFB_H

#include <QString>
#include <QStringList>
#include <QThread>
#include <QProcess>
#include <QMutex>
#include <QWaitCondition>
#include <QFile>

#include "debug.h"

#define DEFAULT_FB_WIDTH	320
#define DEFAULT_FB_HEIGHT	530

enum {
    ANDROID_ICS,
    ANDROID_JB,
    ANDROID_UNKNOWN
};

class Commander
{
public:
    Commander(const char *command = "");

    void addArg(const char *a)        { args << a; }
    void addArg(const QString &a)     { args << a; }
    void addArg(const QStringList &a) { args << a; }

    void clear(void);

    int wait(int msecs = 30000);

    int run(bool waitUntilFinished = true);

    int run(const QStringList &strs, bool w = true)
    {
        args.clear();
        args << strs;
        return run(w);
    }

    bool exitSuccess(void)            { return ret == 0; }

    bool isRunning() {
        return (p != NULL && p->state() == QProcess::Running);
    }

    void printErrorInfo() {
        DT_ERROR("CMD" << cmd << ret << error.simplified());
    }

    bool outputEqual (const char *str) {
        return output.startsWith(str);
    }

    bool outputHas (const char *str) {
        return (output.length() > 0
		&& output.indexOf(str) > 0);
    }

    QList<QByteArray> outputLines() {
        return output.split('\n');
    }

    QList<QByteArray> outputLineHas(const char *key,
                                    bool ignoreComment = true);

    QByteArray error;
    QByteArray output;
    int ret;

protected:
    QString cmd;
    QStringList args;

private:
    QProcess *p;
};

class AdbExecutor : public Commander
{
public:
    AdbExecutor() : Commander("adb") {}

    void printErrorInfo() {
        DT_ERROR("ADB" << args.join(" ") << ret << error.simplified());
    }

    QByteArray& outputFixNewLine(void) {
        // FIXME: adb bug, converted '\n' (0x0A) to '\r\n' (0x0D0A)
        // while transfer binary file from shell stdout
        return output.replace("\r\n", "\n");
    }
};

class DeviceKeyInfo
{
public:
    DeviceKeyInfo(const QString &name, int i, int code):
        keyLayout(name),
        eventDeviceIdx(i),
        powerKeycode(code),
        wakeSucessed(true) {}

    QString keyLayout;
    int eventDeviceIdx;
    int powerKeycode;
    bool wakeSucessed;
};

class AdbExecObject : public QObject
{
    Q_OBJECT

public:
    AdbExecObject() {
        lcdBrightness = 0;
    }

    int screenBrightness(void) { return lcdBrightness; }
    int screenIsOn()           { return lcdBrightness > 0; }

#define KEYLAYOUT_DIR       "/system/usr/keylayout/"
#define KEYLAYOUT_EXT       ".kl"
#define PROC_INPUT_DEVICES  "/proc/bus/input/devices"
#define INPUT_DEV_PREFIX    "/dev/input/event"
#define SYS_LCD_BACKLIGHT   "/sys/class/leds/lcd-backlight/brightness"
#define SYS_INPUT_NAME_LIST "/sys/class/input/input*/name"

    int getDeviceLCDBrightness();

    bool getKeyCodeFromKeyLayout(const QString &keylayout,
                                 const char *key,
                                 int &code);
    QStringList newKeyEventCommand(int deviceIdx,
                                   int type, int code, int value);
    QStringList newKeyEventCommandSequence(int deviceIdx, int code);
    void sendPowerKey(int deviceIdx, int code);

public slots:
    void execCommand(const QStringList cmds) {
        AdbExecutor adb;
        adb.run(cmds);
    }

    void probeDevicePowerKey(void);
    void wakeUpDevice(void);
    void updateDeviceBrightness(void);

signals:
    void screenTurnedOff(void);
    void screenTurnedOn(void);
    void error(QString *msg);

private:
    QList<DeviceKeyInfo> keyInfos;
    int lcdBrightness;
};

class ADB : public QObject
{
    Q_OBJECT

public:
    ADB();
    ~ADB();

    enum {
        DELAY_STEP      = 150,
        DELAY_MINI      = 100,
        DELAY_FAST      = 200,
        DELAY_NORMAL    = 400,
        DELAY_SLOW      = 800,
        DELAY_MAX       = 2000,
        DELAY_INFINITE  = ULONG_MAX
    };

    void loopDelay();
    void setDelay(int d);

    void setMiniDelay() { delay = DELAY_MINI; }
    void setMaxiDelay() { delay = DELAY_MAX; }

    int increaseDelay();

    bool isConnected(void)        { return connected; }
    void setConnected(bool state) { connected = state; }

private:
    QMutex mutex;
    QWaitCondition delayCond;
    unsigned long delay;
    bool connected;
};

class FBEx: public ADB
{
    Q_OBJECT

public:
    FBEx();

    // int w, h, h on header
    // Refer: frameworks/base/cmd/screencap/screencap.cpp
#define FB_DATA_OFFSET (12)
#define FB_BPP_MAX	4
#define GZ_FILE		"/dev/shm/android-fb.gz"
#define MINIGZIP	"minigzip"

    enum {
        PIXEL_FORMAT_RGBX_8888 = 1,
        PIXEL_FORMAT_RGB_888   = 3,
        PIXEL_FORMAT_RGBX_565  = 4
    };

    void setCompress(bool value);
    bool checkCompressSupport(void);
    bool supportCompress (void)    { return doCompress; }
    int  getBPP(void)              { return bpp; }

    int length() {
        return fb_width * fb_height * bpp;
    }

    void setConnected(bool state);
    void sendNewFB(void);

public slots:
    void waitForDevice();
    void probeFBInfo();
    void readFrame();

signals:
    void deviceFound(void);
    void deviceWaitTimeout(void);
    void deviceDisconnected(void);
    void newFBFound(int, int, int, int);
    void newFrame(QByteArray*);
    void error(QString *msg);

private:
    int minigzipDecompress(QByteArray &);
    int screenCap(QByteArray &bytes, int offset);
    int getScreenInfo(const QByteArray &);
    int getDeviceOSType(void);
    int convertRGBAtoRGB888(QByteArray &, int);

    AdbExecutor adbWaiter;

    QByteArray bytes;
    QByteArray out;
    QFile gz;
    bool doCompress;
    int fb_width;
    int fb_height;
    int fb_format;
    int os_type;
    int bpp;
};

#endif // ADBFB_H
