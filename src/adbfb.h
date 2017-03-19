/*
 * adbfb.h
 *
 * Copyright 2012-2013 Yang Hong
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
#include <QPoint>
#include <QTimer>

#include "debug.h"
#include "input-event-types.h"

#define DEFAULT_FB_WIDTH	320
#define DEFAULT_FB_HEIGHT	530
#define IMPOSSIBLE_FB_WIDTH	5120

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

    int run(bool waitUntilFinished = true);

    /* Run given command */
    int run(const QStringList &str, bool waitUntilFinished = true);

    int wait(const int msecs = 30000);

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
                && output.indexOf(str) >= 0);
    }

    QList<QByteArray> outputLines(void) {
        return output.split('\n');
    }

    QList<QByteArray> outputLinesHas(const char *key,
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
#ifdef Q_OS_WIN32
        // adb on win32 add addtional '\r\n' after '\r'
        return output.replace("\r\n", "");
#endif
#ifdef Q_OS_UNIX
        // adb on linux add only a '\n'
        return output.replace("\r\n", "\n");
#endif
        return output;
    }
};

class ADBBase : public QObject
{
    Q_OBJECT

public:
    ADBBase();
    ~ADBBase();

    /* Add some delay between every screen capture action to
     * save both target and host from busy loop.
     */
    enum {
        DELAY_STEP      = 150,  /* ms */
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

signals:
    void deviceFound(void);
    void deviceWaitTimeout(void);
    void deviceDisconnected(void);
    void newPropmtMessae(const QString);

private:
    QMutex mutex;
    QWaitCondition delayCond;
    unsigned long delay;
    bool connected;
};

#define POWER_KEY_COMMON    116

class DeviceKeyInfo
{
public:
    DeviceKeyInfo(const QString &name, int i, int code):
        keyLayout(name),
        eventDeviceIdx(i),
        powerKeycode(code),
        evBit(0),
        wakeSucessed(true) {}

    DeviceKeyInfo():
        keyLayout(""),
        eventDeviceIdx(-1),
        powerKeycode(POWER_KEY_COMMON),
        evBit(0),
        wakeSucessed(true) {}

    QString keyLayout;
    int eventDeviceIdx;
    int powerKeycode;
    int evBit;

    // If this key has sucessfully wakeup device
    // If yes, we always use it
    bool wakeSucessed;
};

#define NO_SUCH_FILE        "No such"
#define SCREENCAP_EXEC      "/system/bin/screencap"
#define KEYLAYOUT_DIR       "/system/usr/keylayout/"
#define KEYLAYOUT_EXT       ".kl"
#define INPUT_DEV_PREFIX    "/dev/input/event"
#define SYS_LCD_BACKLIGHT   "/sys/class/leds/lcd-backlight/brightness"
#define SYS_INPUT_DIR       "/sys/class/input/"
#define SYS_INPUT_INDEX_OFFSET 22

#define EV_IS_TOUCHSCREEN(ev) ((ev & (1 <<EV_SYN)) && (ev & (1 << EV_ABS)) && (ev & (1 << EV_KEY)))
#define EV_IS_KEY(ev)	      ((ev == 0x03) || ((ev & (1 <<EV_KEY)) && (ev & (1 << EV_SW) || ev & (1 << EV_MSC))))
#define EV_IS_MOUSE(ev)	      (ev == 0x17) // EV_SYN | EV_KEY | EV_REL | EV_MSC

class ADBDevice : public ADBBase
{
    Q_OBJECT

public:
    ADBDevice();

#define SCREENON_WAIT_INTERVAL 1000 // ms

    bool screenIsOn(void);
    int screenBrightness(void) { return lcdBrightness; }
    int deviceOSType(void)     { return osType; }

private:
    void probeDeviceHasSysLCDBL(void);
    void probeInputDevices(void);
    int  probeDeviceOSType(void);

    int getDeviceLCDBrightness();

    bool getInputDeviceInfo(DeviceKeyInfo &info,
                            const QByteArray &sysPath);
    bool getKeyCodeFromKeyLayout(const QString &keylayout,
                                 const char *key,
                                 int &code);
    QStringList newKeyEventCommand(int deviceIdx,
                                   int type, int code, int value);
    QStringList newKeyEventCommandSequence(int deviceIdx, int code);
    void sendPowerKey(int deviceIdx, int code);

    QStringList newEventCmd (int type, int code, int value);
    void sendTap(QPoint pos, bool);
    void sendEvent(QPoint pos, bool, bool);
    void wakeUpDeviceViaPowerKey(void);

public slots:
    void execCommand(const QStringList cmds) {
        AdbExecutor adb;
        adb.run(cmds);
    }

    void probeDevice(void);
    void wakeUpDevice(void);
    void updateDeviceBrightness(void);

    void sendVirtualClick(QPoint pos, bool, bool);
    void sendVirtualKey(int key);

signals:
    void screenTurnedOff(void);
    void screenTurnedOn(void);
    void error(QString *msg);
    void newCommand(const QStringList cmds);

private:
    QList<DeviceKeyInfo> powerKeyInfos;
    QTimer screenOnWaitTimer;
    bool hasSysLCDBL;
    int lcdBrightness;
    int osType;

    // Used in jb to distinguish tap/swipe event
    QPoint posOfPress;
    DeviceKeyInfo touchPanel;
};

/* Header of the screencap output into fd,
 * int width, height, format
 * Refer: frameworks/base/cmds/screencap/screencap.cpp
 */
#define FB_DATA_OFFSET (12)
#define FB_BPP_MAX	4

// Temp file for compressed fb data
#define GZ_FILE		"/dev/shm/android-fb.gz"

/* The gzip command on the adb device is an minigzip from
 * external/zlib, we also need one on the host
 */
#define MINIGZIP	"minigzip"

/*
 *
 */
#define ADB_WAIT_INTERVAL_MIN 500
#define ADB_WAIT_INTERVAL_MAX 3*1000*1000

/*
 * Max invalide buffer we can accept before we consider
 * the device disconnected.
 */
#define INVALIDE_BUFFER_MAX 5

class ADBFrameBuffer: public ADBBase
{
    Q_OBJECT

public:
    ADBFrameBuffer();

    enum {
        PIXEL_FORMAT_RGBA_8888 = 1,
        PIXEL_FORMAT_RGBX_8888 = 2,
        PIXEL_FORMAT_RGB_888   = 3,
        PIXEL_FORMAT_RGBX_565  = 4
    };

    void setPaused(bool p);
    bool paused(void)               { return readPaused; }

    /* Check if we have minigzip on the host, so that
     * we can compress screen dump to save transfer time.
     */
    bool checkCompressSupport(void);

    void enableCompress(bool value);
    bool supportCompress (void)     { return doCompress; }

    /* Check if screencap command on the device support
     * -q (quality), -s (speed) option.
     */
    bool checkScreenCapOptions();

    int getBPP(void)                { return bpp; }
    int width()                     { return fb_width; }
    int height()                    { return fb_height; }

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
    void newFBProbed(void);
    void newFBFound(int, int, int);
    void newFBFormat(int);
    void newFrame(QByteArray*);
    void error(QString *msg);

private:
    int minigzipDecompress(QByteArray &);
    int screenCap(QByteArray &bytes, int offset = 0);
    int getScreenInfo(int &, int &, int &);

    AdbExecutor adbWaiter;

    QByteArray bytes;
    QByteArray out;
    QFile gz;
    bool doCompress;
    bool readPaused;
    bool screencapExists;
    bool screencapOptQuality;
    bool screencapOptSpeed;
    int fb_width;
    int fb_height;
    int fb_format;
    int bpp;
    int invalid_buffer_count;
};

#endif // ADBFB_H
