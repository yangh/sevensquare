#include <QDebug>
#include <QPainter>
#include <QMutexLocker>
#include <stdint.h>

#include "fbcellitem.h"

FBCellItem::FBCellItem(QGraphicsItem *parent)
{
	fbConnected = false;
}

FBCellItem::FBCellItem(const QPixmap &pixmap) :
        pixmap(pixmap),
	fbConnected(false)
{
	cellSize.setWidth(pixmap.width());
	cellSize.setHeight(pixmap.height());

	fbSize = cellSize;
	fb = new QPixmap(fbSize);
	fb->fill(QColor(255, 255, 255));

	bytes = NULL;
}

QRectF FBCellItem::boundingRect() const
{
    return QRectF(pixmap.rect());
}

void FBCellItem::setFBSize(QSize size)
{
	QMutexLocker locker(&mutex);

	fbSize = size;
	qDebug() << "New FB size:" << size;

	if (fb != NULL)
		free(fb);

	fb = new QPixmap(fbSize);
	fb->fill(QColor(255, 255, 255));
}

void FBCellItem::setFBRaw(QByteArray *raw)
{
	QMutexLocker locker(&mutex);
	quint16 sum;
       
	sum = qChecksum(raw->data(), raw->length());
	if (sum == lastSum)
		return;

	lastSum = sum;
	bytes = raw;
	update(boundingRect());
}

void FBCellItem::paintFB(QPainter *painter)
{
	QPainter fbPainter;
	QPen pen;
	uint8_t *buf;
	int x, y;
	QMutexLocker locker(&mutex);

	if (bytes == NULL)
		return;

	qDebug() << "Painting FB...";
	buf = (uint8_t *) bytes->data();
	buf += 12; // Skip header

	pen.setWidth(1);

	fbPainter.begin(fb);
	for (x = 0; x < fbSize.height(); x++) {
		for (y = 0; y < fbSize.width(); y++) {
#if 0
			uint32_t v;
			uint8_t r, g, b;
			bcopy(buf, &v, sizeof(uint32_t));
			r = (v >> 24) & 0xFF;
			g = (v >> 16) & 0xFF;
			b = (v >> 8) & 0xFF;
			buf += 4;
#endif
			// FIXME: adb bug, converted '\n' (0x0A) to '\r\n' (0x0D0A)
			// while dump binary file from shell

			int i;
			uint8_t c[3];
			for (i = 0; i < 3; i++) {
				if (buf[0] == 0x0D && buf[1] == 0x0A) {
					c[i] = 0x0A;
					buf += 2;
				} else {
					c[i] = *buf++;
				}
			}
			buf++;
			pen.setColor(QColor(c[0], c[1], c[2]));
			fbPainter.setPen(pen);
			fbPainter.drawPoint(y, x);
		}
	}
	fbPainter.end();
}

void FBCellItem::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (fbConnected) {
	paintFB(painter);
	pixmap = fb->scaled(cellSize,
			Qt::KeepAspectRatio,
			Qt::SmoothTransformation);
    }

    if (pixmap.isNull()) {
        return;
    }

    painter->drawPixmap(pixmap.rect(), pixmap);
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
    //qDebug() << "Item clicked, curr pos: " << curr_pos
    //        << ", orig pos: " << orig_pos;
}
