/*
 * fbcellitem.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QPainter>
#include <QMutexLocker>
#include <QDateTime>
#include <QImage>

#include <stdint.h>

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

QRectF FBCellItem::boundingRect() const
{
    return QRectF(QPoint(0, 0), cellSize);
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
    QMutexLocker locker(&mutex);
    int w, h;

    if (fbSize == size)
        return;

    //qDebug() << "New FB size:" << size << fbSize;
    fbSize = size;

    fb = fb.scaled(size);

    w = cellSize.width();
    h = fbSize.height() * ((float) w / fbSize.width());
    cellSize = QSize(w, h);
    //qDebug() << "New fb cell size" << cellSize;

    ratio = QSizeF((qreal) fbSize.width() / cellSize.width(),
                   (qreal) fbSize.height() / cellSize.height());
    update(boundingRect());
}

void FBCellItem::setFBDataFormat(int format)
{
    switch(format) {
    case FBEx::PIXEL_FORMAT_RGBX_8888:
	rawFBDataFormat = QImage::Format_RGB888; // converted in adbfb
        break;
    case FBEx::PIXEL_FORMAT_RGBX_565:
	rawFBDataFormat = QImage::Format_RGB16; // converted from device
	break;
    case FBEx::PIXEL_FORMAT_RGB_888:
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

    DT_TRACE("FB PAINT RAW S");
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
    DT_TRACE("FB PAINT RAW E");
}

void FBCellItem::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget)
{
    QMutexLocker locker(&mutex);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    //DT_TRACE("FB PAINT S");
    painter->drawPixmap(pixmap.rect(), pixmap);
    //DT_TRACE("FB PAINT E");
}

void FBCellItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
}

void FBCellItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
}

void FBCellItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
}

QPoint FBCellItem::cellPosToVirtual(QPointF pos)
{
    return QPoint(pos.x() * ratio.width(),
                  pos.y() * ratio.height());
}

