#ifndef FBCELLITEM_H
#define FBCELLITEM_H

#include <QPoint>
#include <QPixmap>
#include <QGraphicsItem>
#include <QMutex>
#include <QMutexLocker>

class FBCellItem : public QGraphicsItem
{
public:
    enum {
	    UPDATE_DONE,
	    UPDATE_IGNORED,
    };

    explicit FBCellItem(QGraphicsItem *parent = 0);
    FBCellItem(const QPixmap &pixmap);

    QRectF boundingRect() const;

    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);

    void paintFB(QPainter *painter);

    void setFBConnected(bool state) { fbConnected = state; };
    void setFBSize(QSize size);
    int setFBRaw(QByteArray *raw);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QPixmap pixmap;
    QSize cellSize;

    QPixmap *fb;
    QByteArray bytes;
    QSize fbSize;
    bool fbConnected;
    QMutex mutex;
    quint16 lastSum;
};

#endif // FBCELLITEM_H
