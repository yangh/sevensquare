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
    enum {
        UPDATE_INVALID = -1,
        UPDATE_DONE,
        UPDATE_IGNORED
    };

    FBCellItem();
    FBCellItem(const QPixmap &p);

    QRectF boundingRect() const;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);

    void paintFB(QByteArray *);

    void setCellSize(QSize size);
    void setPixmap(const QPixmap &p);
    void setFBSize(QSize size);
    int  setFBRaw(QByteArray *raw);
    void setFBDataFormat(int format);

    QPoint cellPosToVirtual(QPointF pos);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

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
