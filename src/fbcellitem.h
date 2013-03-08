/*
 * fbcellitem.h
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#ifndef FBCELLITEM_H
#define FBCELLITEM_H

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QMutex>

#include "cubecellitem.h"
#include "adbfb.h"

class FBCellItem : public CubeCellItem
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

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    bool sendVirtualClick(QPointF posScene, bool, bool);

private:
    QImage::Format rawFBDataFormat;
    QSize   cellSize;

    QPixmap fb;
    QSize   fbSize;
    QSizeF  ratio;

    quint16 lastSum;
};

#endif // FBCELLITEM_H
