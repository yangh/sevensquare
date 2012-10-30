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
	ANDROID_UNKNOWN,
};

class AdbExecutor
{
public:
	AdbExecutor() : ret(-1), cmd("adb") {};
	AdbExecutor(const char *a) { args << a; };

	void addArg(const char *a)	{ args << a; };
	void addArg(const QString &a)	{ args << a; };
	void addArg(const QStringList &a) { args << a; };

	bool exitSuccess(void)		{ return ret == 0; };

	void clear(void);

	int wait(int msecs = 30000);

	int run(bool waitUntilFinished = true);

	int run(const QStringList &strs) {
		args << strs;
		return run();
	}

	bool isRunning() { return (p.state() == QProcess::Running); }

	void printErrorInfo() {
		qDebug() << "Process error:" << ret;
		qDebug() << error;
	}

	bool outputEqual (const char *str) {
		return output.startsWith(str);
	}

	QByteArray error;
	QByteArray output;
	int ret;

private:
	QProcess p;
	QString cmd;
	QStringList args;
};

class ADB : public QThread
{
	Q_OBJECT
public:
	ADB();
	~ADB();

	enum {
		DELAY_STEP	= 200,
		DELAY_FAST	= 200,
		DELAY_NORMAL	= 400,
		DELAY_SLOW	= 800,
		DELAY_MAX	= 2000,
		DELAY_INFINITE	= ULONG_MAX,
	};

	void stop();
	void loopDelay();
	void setDelay(int d);

	void setMiniDelay() { delay = DELAY_FAST; };
	void setMaxiDelay() { delay = DELAY_INFINITE; };

	void increaseDelay() {
		if (delay < DELAY_MAX)
			delay += DELAY_STEP;
	};

	bool isStopped(void)	{ return stopped; };
	bool isConnected(void)	{ return connected; };

	void disconnect(void) {
		connected = false;
		emit deviceDisconnected();
	}

	int getDeviceOSType(void);

protected:
	bool stopped;
	bool connected;
	virtual void run();

signals:
	void deviceFound(void);
	void deviceDisconnected(void);

private:
	QMutex mutex;
	QWaitCondition delayCond;
	unsigned long delay;
};

class FBReader : public ADB
{
	Q_OBJECT
public:
	FBReader();
	~FBReader() {
		adbInstance.stop();
	}

	// int w, h, h on header
	// Refer: frameworks/base/cmd/screencap/screencap.cpp
#define FB_DATA_OFFSET (12)
#define FB_BPP_MAX	4
#define GZ_FILE		"/dev/shm/android-fb.gz"

	void startRead(void) {
		DT_TRACE("Start reader...");
		start();
		DT_TRACE("Start adb waiter...");
		adbInstance.start();
		DT_TRACE("Start adb waiter end...");
	}

	void stopRead() {
		stop();
		adbInstance.stop();
	}

	bool supportCompress();
	void setCompress(bool value);
	int  getScreenInfo(int &, int &, int &);

	int length() {
		return fb_width * fb_height * bpp;
	}

protected:
	int AndrodDecompress(QByteArray &);
	int screenCap(QByteArray &bytes, bool, bool);
	void run();
	ADB adbInstance;

signals:
	void newFbReceived(QByteArray *bytes);
	void newFBFound(int, int, int, int);

public slots:
	void deviceConnected(void);

private:
	QFile gz;
	uchar *mmbuf;
	bool mmaped;
	bool do_compress;
	int fb_width;
	int fb_height;
	int fb_format;
	int bpp;
};

#endif // ADBFB_H
