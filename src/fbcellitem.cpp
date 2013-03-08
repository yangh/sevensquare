/*
 * fbcellitem.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#include <QPainter>
#include <QMutexLocker>
#include <QDateTime>
#include <QImage>

#include <stdint.h>

#include "cubescene.h"
#include "fbcellitem.h"
#include "debug.h"

FBCellItem::FBCellItem()
{
    lastSum = -1;

    // default converted in adbfb
    rawFBDataFormat = QImage::Format_RGB888;

    cellSize = QSize(DEFAULT_FB_WIDTH, DEFAULT_FB_HEIGHT);
    pixmap = QPixmap(cellSize);
    pixmap.fill(QColor(Qt::black));

    fbSize = cellSize;
    fb = QPixmap(fbSize);
    fb.fill(QColor(Qt::black));
    ratio = QSizeF(1.0, 1.0);

    //setFlag(QGraphicsItem::ItemIsSelectable);
    //setFlag(QGraphicsItem::ItemIsMovable);
}

FBCellItem::FBCellItem(const QPixmap &p)
{
    setPixmap(p);
}

void FBCellItem::setPixmap(const QPixmap &p)
{
    pixmap = p;
    fb = p;
    cellSize = pixmap.size();
}

void FBCellItem::setCellSize(QSize size)
{
    QMutexLocker locker(&mutex);

    cellSize = size;
    pixmap = fb.scaled(cellSize,
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation);

    ratio = QSizeF((qreal) fbSize.width() / cellSize.width(),
                   (qreal) fbSize.height() / cellSize.height());
    update(boundingRect());
}

void FBCellItem::setFBSize(QSize size)
{
    int w, h;

    if (fbSize == size)
        return;
    {
        QMutexLocker locker(&mutex);

        //qDebug() << "New FB size:" << size << fbSize;
        fbSize = size;
        fb = fb.scaled(size);

        w = cellSize.width();
        h = fbSize.height() * ((float) w / fbSize.width());
    }

    setCellSize(QSize(w, h));
    //qDebug() << "New fb cell size" << cellSize;
}

void FBCellItem::setFBDataFormat(int format)
{
    switch(format) {
    case ADBFrameBuffer::PIXEL_FORMAT_RGBA_8888:
    case ADBFrameBuffer::PIXEL_FORMAT_RGBX_8888:
        rawFBDataFormat = QImage::Format_RGB888; // converted in adbfb
        break;
    case ADBFrameBuffer::PIXEL_FORMAT_RGBX_565:
        rawFBDataFormat = QImage::Format_RGB16; // converted from device
        break;
    case ADBFrameBuffer::PIXEL_FORMAT_RGB_888:
        rawFBDataFormat = QImage::Format_RGB888; // converted from device
        break;
    default:
        DT_ERROR("Unknown fb data format " << format);
    }
}

int FBCellItem::setFBRaw(QByteArray *raw)
{
    QMutexLocker locker(&mutex);
    quint16 sum;

    // TODO: There is a hotspot here, and if make
    // it threaded here, lock is another issue.
    // FBReader should run as fast as possible to
    // grab new frame from device, so the duty is
    // not for it.

    // TODO: Check and update partially, block by block
    sum = qChecksum(raw->data(), raw->length());
    if (sum == lastSum)
        return UPDATE_IGNORED;

    lastSum = sum;
    // TODO: Do in a separate thread?
    paintFB(raw);

    return UPDATE_DONE;
}

void FBCellItem::paintFB(QByteArray *bytes)
{
    QPainter fbPainter;
    QImage image;

    //DT_TRACE("FB PAINT RAW S");
    fbPainter.begin(&fb);
    image = QImage((const uchar*)bytes->data(),
                   fbSize.width(), fbSize.height(),
                   rawFBDataFormat);
    fbPainter.drawImage(fb.rect(), image);
    fbPainter.end();

    pixmap = fb.scaled(cellSize,
                       Qt::KeepAspectRatio,
                       Qt::SmoothTransformation);
    update(boundingRect());
    //DT_TRACE("FB PAINT RAW E");
}

void FBCellItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    sendVirtualClick(event->scenePos(), true, false);
}

void FBCellItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
#if 0
    QPointF pos = event->scenePos();

    // Disable send mouse move event because is too slow
    // to send event in time to device, even we filter some
    // out.
    if (sendVirtualClick(pos, false, false)) {
        return;
    }
#endif
    QGraphicsItem::mouseMoveEvent(event);
}

void FBCellItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    //DT_TRACE("SCREEN Click" << pos.x() << pos.y());
    sendVirtualClick(pos, false, true);
}

QPoint FBCellItem::cellPosToVirtual(QPointF pos)
{
    return QPoint(pos.x() * ratio.width(),
                  pos.y() * ratio.height());
}

bool FBCellItem::sendVirtualClick(QPointF scenePos, bool press, bool release)
{
    QPoint pos;

    if (cube) {
        pos = cellPosToVirtual(scenePos);
        return cube->sendVirtualClick(pos, press, release);
    }

    return false;
}
