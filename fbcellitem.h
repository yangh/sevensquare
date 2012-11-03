/*
 * fbcellitem.h
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#ifndef FBCELLITEM_H
#define FBCELLITEM_H

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QMutex>

#include "adbfb.h"

class FBCellItem : public QGraphicsItem
{

public:
    FBCellItem();
    FBCellItem(const QPixmap &p);

    enum {
        UPDATE_INVALID = -1,
        UPDATE_DONE,
        UPDATE_IGNORED
    };

    void setCellSize(QSize size);
    void setPixmap(const QPixmap &p);
    void setFBSize(QSize size);
    int  setFBRaw(QByteArray *raw);
    void setFBDataFormat(int format);
    void paintFB(QByteArray *);

    QPoint cellPosToVirtual(QPointF pos);
    QRectF boundingRect() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);

private:
    QImage::Format rawFBDataFormat;
    QPixmap pixmap;
    QSize cellSize;

    QPixmap fb;
    QSize fbSize;
    QSizeF ratio;

    QMutex mutex;
    quint16 lastSum;
};

#endif // FBCELLITEM_H
