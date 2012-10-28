#include <QDebug>
#include <QPainter>
#include <QMutexLocker>
#include <QDateTime>
#include <stdint.h>

#include "fbcellitem.h"
#include "debug.h"

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
	fb->fill(QColor(Qt::black));

	bytes = NULL;
	lastSum = -1;
}

QRectF FBCellItem::boundingRect() const
{
    return QRectF(0, 0, cellSize.width(), cellSize.height());
}

void FBCellItem::setFBSize(QSize size)
{
	QMutexLocker locker(&mutex);

	if (fbSize == size)
		return;

	qDebug() << "New FB size:" << size << fbSize;
	fbSize = size;

	if (fb != NULL)
		free(fb);

	fb = new QPixmap(fbSize);
	fb->fill(QColor(Qt::black));
}

int FBCellItem::setFBRaw(QByteArray *raw)
{
	QMutexLocker locker(&mutex);
	quint16 sum;
       
	// TODO: Check and update partially, block by block
	sum = qChecksum(raw->data(), raw->length());
	if (sum == lastSum)
		return UPDATE_IGNORED;

	lastSum = sum;
	bytes = raw;
	update(boundingRect());

	return UPDATE_DONE;
}

void FBCellItem::paintFB(QPainter *painter)
{
	QMutexLocker locker(&mutex);
	QPainter fbPainter;
	uint8_t *buf;
	int x, y;

	if (bytes == NULL ||
		bytes->length() < fbSize.width() * fbSize.height() * 4)
	{
		qDebug() << "Invalid data:" << bytes->length();
		return;
	}

	//qDebug() << "Painting FB...";
	DT_TRACE("PAIT RAW S");

	buf = (uint8_t *) bytes->data();
	buf += 12; // Skip header

	fbPainter.begin(fb);
	for (y = 0; y < fbSize.height(); y++) {
		for (x = 0; x < fbSize.width(); x++) {
			fbPainter.setPen(QColor(buf[0], buf[1], buf[2]));
			buf += 4;
			fbPainter.drawPoint(x, y);
		}
	}
	fbPainter.end();

	DT_TRACE("PAIT RAW E");
}

void FBCellItem::paint(QPainter *painter,
                         const QStyleOptionGraphicsItem *option,
                         QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    DT_TRACE("FB PAINT S");
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
    DT_TRACE("FB PAINT E");
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
