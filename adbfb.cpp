/*
 * adbfb.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QThread>

#include <stdlib.h>
#include <time.h>
#include <strings.h>
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

FBReader::FBReader()
{
	do_compress = false;
	fb_width = DEFAULT_FB_WIDTH;
	fb_height = DEFAULT_FB_HEIGHT;
	fb_format = 1; //TODO:...
	bpp = FB_BPP_MAX;
	mmbuf = NULL;
	mmaped = false;

	QObject::connect(&adbInstance, SIGNAL(deviceFound(void)),
		    this, SLOT(deviceConnected(void)));
}

bool FBReader::supportCompress()
{
	return false;
}

void FBReader::setCompress(bool value)
{
	if (do_compress != value) {
		do_compress = value;
		// Notify compress status changed.
	}

	if (do_compress) {
		gz.setFileName(GZ_FILE);
		gz.open(QIODevice::WriteOnly|QIODevice::Unbuffered);
		gz.resize(fb_width * fb_height * FB_BPP_MAX);
		//TODO: the map alway fail, find a new way.
		mmbuf = gz.map(0, gz.size());
		mmaped = (mmbuf != 0);
	} else {
		if (mmaped) {
			mmaped = false;
			mmbuf = NULL;
			gz.unmap(mmbuf);
		}
		gz.close();
	}
}

int bigEndianToInt32(const QByteArray &bytes)
{
	uint32_t v;
	const char *buf = bytes.data();

	bcopy(buf, &v, sizeof(uint32_t));

	return v;
}

int FBReader::AndrodDecompress(QByteArray &bytes)
{
	int ret;
	QProcess p;

	if (mmaped) {
		//TODO:
		qDebug() << "Maped write file";
		memcpy(mmbuf, bytes.data(), bytes.length());
	} else {
		gz.seek(0);
		gz.write(bytes.data(), bytes.length());
		gz.flush();
	}
	DT_TRACE("DECOMP GZ TO FILE");

	p.start("minigzip", QStringList() << "-d" << "-c" << GZ_FILE);
	p.waitForFinished();
	ret = p.exitCode();

	bytes = p.readAllStandardOutput();
	DT_TRACE("DECOMP FINISHED");
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

int FBReader::screenCap(QByteArray &bytes,
			bool compress = false,
			bool removeHeader = false)
{
	int ret;
	AdbExecutor adb;
	QStringList args;

	args << "shell" << "screencap";

	if (compress) {
		args << "|" << "gzip";
	}

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

int FBReader::getScreenInfo(int &width, int &height, int &format)
{
	QByteArray bytes;
	int ret;

	ret = screenCap(bytes);

	if (ret != 0)
		return -1;

	width = bigEndianToInt32(bytes.mid(0, 4));
	height = bigEndianToInt32(bytes.mid(4, 4));
	format = bigEndianToInt32(bytes.mid(8, 4));

	fb_width = width;
	fb_height = height;
	fb_format = format;

	//TODO: emit screenInfoChanged

	return 0;
}

void FBReader::run()
{
	QByteArray bytes;
	QByteArray out;
	int ret;

	DT_TRACE("FBR START");
	setMaxiDelay();
	loopDelay();

	bytes.fill(0x00, length());

	while (! stopped) {
		DT_TRACE("CAP");
		ret = screenCap(bytes, do_compress, true);

		if (ret == 0) {
			out = bytes;
			emit newFbReceived(&out);
		} else {
			disconnect();
			setMaxiDelay();
			adbInstance.setDelay(0);
		}

		loopDelay();
	}
	qDebug() << "FBReader stopped";

	return;
}

void FBReader::deviceConnected(void)
{
	int w, h, f, os;
	int ret;

	ret = getScreenInfo(w, h, f);

	if (ret == -1) {
		qDebug() << "Failed to get screen info.";
		return;
	}

	os = getDeviceOSType();
	emit newFBFound(w, h, f, os);
	setDelay(0);
}

ADB::ADB()
{
	stopped = false;
	delay = DELAY_INFINITE;
	connected = false;
}

ADB::~ADB()
{
}

void ADB::run()
{
	AdbExecutor adb;
	QByteArray bytes;

	while (! stopped) {
		if (! adb.isRunning()) {
			DT_TRACE("ADB wait");
			adb.clear();
			adb.addArg("wait-for-device");
			adb.run(false);
		}

		adb.wait(500);

		if (adb.ret == 0) {
			DT_TRACE("ADB found");
			connected = true;
			delay = DELAY_INFINITE;
			emit deviceFound();

			// Wait for next request
			loopDelay();
		}
	}
	qDebug() << "ADB stopped";

	return;
}

void ADB::stop()
{
	stopped = true;
	setDelay(0);
	while (! isFinished())
		msleep(100);
	quit();
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

int ADB::getDeviceOSType(void)
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

