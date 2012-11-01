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
#define DEFAULT_FB_HEIGHT	533

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
    AdbExecutor() : Commander("adb") {};

    void printErrorInfo() {
        DT_ERROR("ADB" << args.join(" ") << ret << error.simplified());
    }
};

class ADB : public QObject
{
    Q_OBJECT

public:
    ADB();
    ~ADB();

    enum {
        DELAY_STEP      = 200,
        DELAY_FAST      = 200,
        DELAY_NORMAL    = 400,
        DELAY_SLOW      = 800,
        DELAY_MAX       = 2000,
        DELAY_INFINITE  = ULONG_MAX
    };

    void loopDelay();
    void setDelay(int d);

    void setMiniDelay() { delay = DELAY_FAST; }
    void setMaxiDelay() { delay = DELAY_MAX; }

    void increaseDelay() {
        if (delay < DELAY_MAX)
            delay += DELAY_STEP;
    }

    bool isConnected(void)      { return connected; }
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
#define RAW_FILE	"/dev/shm/android-fb"
#define MINIGZIP	"minigzip"

    void setCompress(bool value);
    bool supportCompress (void) { return doCompress; };

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
    int AndrodDecompress(QByteArray &);
    int screenCap(QByteArray &bytes, bool, bool);
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
