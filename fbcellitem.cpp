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
    bpp = 4;

    cellSize = QSize(DEFAULT_FB_WIDTH, DEFAULT_FB_HEIGHT);
    pixmap = QPixmap(cellSize);
    pixmap.fill(QColor(Qt::black));

    fbSize = cellSize;
    fb = QPixmap(fbSize);
    fb.fill(QColor(Qt::black));

    // Clickable
    setFlags(QGraphicsItem::ItemIsSelectable);
}

FBCellItem::FBCellItem(const QPixmap &p)
{
    setPixmap(pixmap);
}

void FBCellItem::setPixmap(const QPixmap &p)
{
    pixmap = p;
    cellSize = pixmap.size();
}

QRectF FBCellItem::boundingRect() const
{
    return QRectF(QPoint(0, 0), cellSize);
}

void FBCellItem::setFBSize(QSize size)
{
    QMutexLocker locker(&mutex);
    int w, h;

    if (fbSize == size)
        return;

    qDebug() << "New FB size:" << size << fbSize;
    fbSize = size;

    fb = QPixmap(fbSize);
    fb.fill(QColor(Qt::black));

    w = cellSize.width();
    h = fbSize.height() * ((float) w / fbSize.width());
    cellSize = QSize(w, h);

    update(boundingRect());
}

int FBCellItem::setFBRaw(QByteArray *raw)
{
    QMutexLocker locker(&mutex);
    quint16 sum;

    if (! raw || raw->length() < getFBDataSize()) {
        qDebug() << "Invalid data, ignored";
        return UPDATE_INVALID;
    }

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
                   QImage::Format_RGB888);
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

    DT_TRACE("FB PAINT S");

    painter->drawPixmap(pixmap.rect(), pixmap);

    DT_TRACE("FB PAINT E");
}

void FBCellItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item pressed: " << curr_pos;
}

void FBCellItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item moveded: " << curr_pos;
}

void FBCellItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "Item clicked";
}
