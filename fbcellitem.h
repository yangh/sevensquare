#ifndef FBCELLITEM_H
#define FBCELLITEM_H

#include <QPoint>
#include <QPixmap>
#include <QGraphicsItem>
#include <QMutex>
#include <QMutexLocker>

#define DEFAULT_FB_WIDTH	480
#define DEFAULT_FB_HEIGHT	800

class FBCellItem : public QGraphicsItem
{
public:
    enum {
	    UPDATE_DONE,
	    UPDATE_IGNORED,
    };

    FBCellItem(QGraphicsItem *parent);
    FBCellItem(const QPixmap &p);
    FBCellItem();

    void setPixmap(const QPixmap &p);
    QRectF boundingRect() const;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = 0);

    void paintFB(QPainter *painter);

    void setFBConnected(bool state);
    void setFBSize(QSize size);
    int  setFBRaw(QByteArray *raw);
    void setBPP(int n) { bpp = n; };
    int  getFBDataSize(void) {
	    return fbSize.width() * fbSize.height() * bpp;
    }

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    QPixmap pixmap;
    QSize cellSize;

    QPixmap fb;
    QByteArray bytes;
    int bpp;
    QSize fbSize;
    bool fbConnected;
    QMutex mutex;
    quint16 lastSum;
};

#endif // FBCELLITEM_H
