/*
 * cubecellitem.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#ifndef CUBECELLITEM_H
#define CUBECELLITEM_H

#include <QMutex>
#include <QPoint>
#include <QPixmap>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>

#include "debug.h"

class CubeScene;

class CubeCellItem : public QGraphicsItem
{
public:
    CubeCellItem();
    CubeCellItem(const QPixmap &pixmap);

    QRectF boundingRect() const;

    const QPoint cubePos(void)          { return curr_pos; }
    const QPoint originalCubePos(void)  { return orig_pos; }

    void setCubePos(QPoint pos) {
        curr_pos = pos;
        //qDebug() << "new pos" << pos << curr_pos;
        setPos(curr_pos);
        update(boundingRect());
    }

    void setCubePos(QPointF pos) {
        setCubePos(pos.toPoint());
    }

    void setOriginalCubePos(int row, int col) {
        orig_pos.setX(row);
        orig_pos.setY(col);
    }

    void setCube(CubeScene *scene)      { cube = scene; }
    void setKey(int k)                  { virtual_key = k; }
    int  key(void)                      { return virtual_key; }

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);

    QMutex mutex;
    QPixmap pixmap;
    CubeScene *cube;

private:
    QPoint orig_pos;
    QPoint curr_pos;
    int virtual_key;
};

#endif // CUBECELLITEM_H
