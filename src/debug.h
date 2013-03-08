/*
 * debug.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include <QDebug>
#include <QDateTime>

#define DEBUG_TIME 1

#ifdef DEBUG_TIME
#define DT_TRACE(msg) \
    qDebug() << QDateTime::currentMSecsSinceEpoch() << msg;
#define DT_ERROR(msg) \
    qDebug() << QDateTime::currentMSecsSinceEpoch() << "ERROR" << msg;
#else
#define DT_TRACE(z)
#define DT_ERROR(z)
#endif

#endif /* DEBUG_H_ */
