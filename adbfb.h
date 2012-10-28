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

class AdbExecutor
{
public:
	AdbExecutor() {};
	AdbExecutor(const char *a)	{ args << a; };

	void addArg(const char *a)	{ args << a; };
	void addArg(const QString &a)	{ args << a; };
	void addArg(const QStringList &a) { args << a; };

	bool exitSuccess(void)		{ return ret == 0; };

	void clear(void) {
		args.clear();
		error.clear();
		output.clear();
		ret = 0;
	}

	int run(const QStringList &cmd) {
		args << cmd;
		return run();
	}

	int run(bool waitForFinished = true) {
		cmd = "adb";

		//qDebug() << "Exec: " << cmd << " " << args;
		p.start(cmd, args);

		if (waitForFinished) {
			return wait();
		}

		return 0;
	}

	int wait() {
		p.waitForFinished();

		output = p.readAllStandardOutput();
		error = p.readAllStandardError();
		ret = p.exitCode();

		// FIXME: adb bug, converted '\n' (0x0A) to '\r\n' (0x0D0A)
		// while dump binary file from shell
		output.replace("\r\n", "\n");

		return ret;
	}

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

	enum {
		DELAY_STEP	= 100,
		DELAY_FAST	= 200,
		DELAY_NORMAL	= 400,
		DELAY_SLOW	= 800,
		DELAY_MAX	= 2000,
		DELAY_INFINITE	= ULONG_MAX,
	};

	void loopDelay() {
		mutex.lock();
		if (delay) {
			DT_TRACE(delay);
			delayCond.wait(&mutex, delay);
		}
		mutex.unlock();
	}

	void setDelay(int d) {
		mutex.lock();
		delay = d;
		delayCond.wakeAll();
		mutex.unlock();
	};

	void setMiniDelay() { delay = DELAY_FAST; };
	void setMaxiDelay() { delay = DELAY_INFINITE; };

	void increaseDelay() {
		if (delay < DELAY_MAX)
			delay += DELAY_STEP;
	};

	void setStopped(bool s) { stopped = s; };
	bool isStopped(void)	{ return stopped; };

	void stop() {
		stopped = true;
		setDelay(0);
		while (! isFinished()) msleep(100);
		quit();
	};

	bool isConnected(void)	{ return connected; };

	void disconnect(void) {
		connected = false;
		emit deviceDisconnected();
	}

protected:
	bool connected;
	virtual void run();

signals:
	void deviceFound(void);
	void deviceDisconnected(void);

private:
	bool stopped;
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
		start();
		adbInstance.start();
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

public slots:
	void deviceConnected(void);

private:
	QFile gz;
	uchar *mmbuf;
	bool mmaped;
	bool do_compress;
	bool stopped;
	int fb_width;
	int fb_height;
	int fb_format;
	int bpp;
};

#endif // ADBFB_H
