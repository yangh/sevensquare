#ifndef DEBUG_H_
#define DEBUG_H_

#include <QDebug>
#include <QDateTime>

#define DEBUG_TIME 1

#ifdef DEBUG_TIME
  #define DT_TRACE(msg) \
    qDebug() << QDateTime::currentMSecsSinceEpoch() << msg;
#else
  #define DT_TRACE(z)
#endif

#endif /* DEBUG_H_ */
