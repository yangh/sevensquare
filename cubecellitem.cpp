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

void CubeCellItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    //emit Clicked(curr_pos);

    qDebug() << "Item clicked: " << curr_pos;
}
