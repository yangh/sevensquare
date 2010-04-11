#include <QDebug>
#include <QPainter>

#include "cubecellitem.h"

CubeCellItem::CubeCellItem(QGraphicsItem *parent)
{
}

CubeCellItem::CubeCellItem(const QPixmap &pixmap) :
        pixmap(pixmap)
{
    qDebug() << "New item: " << &pixmap;
}

QRectF CubeCellItem::boundingRect() const
{
    return QRectF(pixmap.rect());
}

void CubeCellItem::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (pixmap.isNull()) {
        return;
    }

    painter->drawPixmap(pixmap.rect(), pixmap);
}

void CubeCellItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item pressed: " << curr_pos;
}

void CubeCellItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item moveded: " << curr_pos;
}

void CubeCellItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    qDebug() << "Item clicked, curr pos: " << curr_pos
            << ", orig pos: " << orig_pos;
}
