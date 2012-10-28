/*
 * cubecellitem.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#ifndef CUBECELLITEM_H
#define CUBECELLITEM_H

#include <QPoint>
#include <QPixmap>
#include <QGraphicsItem>

class CubeCellItem : public QGraphicsItem
{
public:
    explicit CubeCellItem(QGraphicsItem *parent = 0);
    CubeCellItem(const QPixmap &pixmap);

    QRectF boundingRect() const;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);


    const QPoint cubePos(void) { return curr_pos; };
    const QPoint originalCubePos(void) { return orig_pos; };

    void setCubePos(int row, int col) {
        curr_pos.setX(row);
        curr_pos.setY(col);
    }

    void setOriginalCubePos(int row, int col) {
        orig_pos.setX(row);
        orig_pos.setY(col);
    }

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QPixmap pixmap;
    QPoint orig_pos;
    QPoint curr_pos;
};

#endif // CUBECELLITEM_H
