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
	new_device_found = false;
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
	uint32_t v = 0;
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

int FBReader::convertRGBAtoRGB888(QByteArray &bytes, int offset)
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

void FBReader::run()
{
	QByteArray bytes;
	QByteArray out;
	int ret;
	int len;

	DT_TRACE("FBR START");

	while (! stopped) {
		if (! adbInstance.isConnected()) {
			setMaxiDelay();
			loopDelay();
		}

		ret = screenCap(bytes, do_compress, false);

		if (ret == 0 && new_device_found) {
			ret = probeFBInfo(bytes);
			connected = true;
		}

		if (ret == 0) {
			len = convertRGBAtoRGB888(bytes, FB_DATA_OFFSET);
			out = bytes.mid(FB_DATA_OFFSET, len);
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

int FBReader::getScreenInfo(const QByteArray &bytes)
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

int FBReader::probeFBInfo(const QByteArray &bytes)
{
	int os;
	int ret;

	new_device_found = false;

	ret =  getScreenInfo(bytes);
	if (ret != 0) {
		qDebug() << "Failed to probe screen info.";
		return ret;
	}

	os = getDeviceOSType();
	emit newFBFound(fb_width, fb_height, fb_format, os);

	return 0;
}

void FBReader::deviceConnected(void)
{
	new_device_found = true;
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
			emit deviceFound();

			// Wait for next request
			delay = DELAY_INFINITE;
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

