#include <QGraphicsRectItem>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QStringList>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QDateTime>

#include <stdlib.h>
#include <time.h>
#include <strings.h>
#include <stdint.h>

#include "cubescene.h"

FbReader::FbReader(QObject * parent) :
	QThread(parent)
{
	do_compress = false;
}

bool FbReader::supportCompress()
{

}

bool FbReader::setCompress(bool value)
{
	if (do_compress != value) {
		do_compress = value;
		// Notify compress status changed.
	}
}

int bigEndianToInt32(const QByteArray &bytes)
{
	uint32_t v;
	const char *buf = bytes.data();

	bcopy(buf, &v, sizeof(uint32_t));

	qDebug() << "V: " << v;

	return v;
}

void FbReader::parseFbData(const QByteArray &bytes)
{
	int i, v[3];
	QByteArray c;
	const char *buf;

	for (i = 0; i < 3; i++) {
		c = bytes.mid(i * 4, 8);
		v[i] = bigEndianToInt32(c);
		qDebug() << "Values: " << v[i];
	}
}

void FbReader::run()
{
	QProcess p;
	QString cmd = "adb";
	QStringList args;
	QByteArray bytes;
	int ret;

	args << "shell" << "screencap";
	if (do_compress)
		args << "| gzip";

	qDebug() << "Exec: " << cmd << " " << args;

	while (1) {
		p.start(cmd, args);
		p.waitForFinished();
		ret = p.exitCode();

		if (ret == 0) {
			p.setReadChannel(QProcess::StandardOutput);
			bytes = p.readAllStandardOutput();
			qDebug() << "Read data..." << bytes.length()
				<< QDateTime::currentMSecsSinceEpoch() / 1000;
			parseFbData(bytes);
		} else {
			bytes = p.readAllStandardError();
			bytes.chop(1); // Remove trailly new line char
			qDebug() << "Process return:" << ret <<  bytes;
			msleep(1000);
		}
	}
}

CubeScene::CubeScene(QObject * parent) :
        QGraphicsScene(parent)
{
    cell_width = DEFAULT_CELL_WIDTH;

    v_width = 720;
    v_height = 1280;

    reader = new FbReader(parent);
    startFbReader();
}

CubeScene::~CubeScene()
{
	stopFbReader();
}

void CubeScene::startFbReader() {
	reader->start();
	reader->setPriority(QThread::HighPriority);
}

void CubeScene::stopFbReader() {
	reader->quit();
}

void CubeScene::loadImage (const QString &file)
{
    qDebug() << "Load image from: " << file;

    if (pixmap.load (file, 0)) {
        initialize();
    }
}

void CubeScene::initialize (void)
{
    int row, col;

    qDebug() << "Scene initialize.";

    cube_width = pixmap.width();
    cube_height = pixmap.height();

    row_size = ROW_NUM;
    col_size = COL_NUM;

    cell_width = (cube_width - GRID_WIDTH * col_size * 2) / col_size;
    cell_height = (cube_height - GRID_WIDTH * row_size * 2) / row_size;

    x_pad = (cube_width - cell_width * col_size) / 2;
    y_pad = (cube_height - cell_height * row_size) / 2;

    /* Background */
    setBackgroundBrush(QBrush(pixmap));

    bg_mask = new QGraphicsRectItem(QRectF(0, 0, cube_width, cube_height));
    bg_mask->setBrush(QBrush(QColor(0, 0, 0, 135)));
    bg_mask->setPen(Qt::NoPen);
    bg_mask->setZValue(0); /* lay in the bottom */

    addItem(bg_mask);

#if 0
    /* Thumnail of image */
    int tw, th;
    QPixmap nail_bg;
    CubeCellItem *nail_cell;

    tw = cell_width - THUMNAIL_X_PAD * 2;
    th = pixmap.height() * tw / pixmap.width();
    nail_bg = pixmap.scaled(QSize(tw, th),
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
    nail_cell = new CubeCellItem(nail_bg);

    int tx, ty;
    tx = cell_width * (col_size - 1) + THUMNAIL_X_PAD + x_pad;
    ty = cell_width * (row_size - 2) + (cell_width - th) / 2 + y_pad;
    //qDebug() << "Draw thumnail at: " << tx << ", " << ty;
    nail_cell->setPos(tx, ty);
    nail_cell->setOriginalCubePos(THUMNAIL_CELL_POS, THUMNAIL_CELL_POS);
    nail_cell->setZValue(5);

    addItem(nail_cell);

    /* Start button */
    CubeCellItem *start_cell;
    QPixmap start_icon(16, 16);
    static const QPointF points[3] = {
        QPointF(4.0, 0.0),
        QPointF(16.0, 8.0),
        QPointF(4.0, 16.0)
    };

    start_icon.fill(Qt::transparent);

    QPainter painter(&start_icon);
    QPen pen;

    pen.setColor(QColor(Qt::gray));
    pen.setWidth(1);
    painter.setPen(pen);
    painter.setBrush(QBrush(QColor(Qt::black)));
    painter.drawPolygon(points, 3, Qt::WindingFill);

    start_cell = new CubeCellItem(start_icon);
    tx = cell_width * (col_size - 1) + (cell_width - 16) / 2 + x_pad;
    ty = cell_width * (row_size - 3) + (cell_width - 16) / 2 + y_pad;
    qDebug() << "Draw start buttonat: " << tx << ", " << ty;
    start_cell->setPos(tx, ty);
    start_cell->setOriginalCubePos(STARTBUTTON_CELL_POS, STARTBUTTON_CELL_POS);
    start_cell->setZValue(5);

    addItem(start_cell);

    /* Grid in right-bottom */
    drawGrid (row_size - 1, col_size - 1);

#endif

    /* Draw grid */
    for (col = 0; col < col_size; col++) {
        for (row = 0; row < row_size; row++) {
            drawGrid (row, col);
        }
    }

    /* Initialize cell with background */
    for (col = 0; col < col_size; col++) {
        for (row = 0; row < row_size; row++) {
            QPixmap cell_bg;
            QPoint cell_pos;

            qDebug() << "Init cell: " << row << ", " << col;
            cell_pos = getCellPos(row, col);
            cell_bg = pixmap.copy(cell_pos.x(), cell_pos.y(),
                                  cell_width - GRID_WIDTH,
                                  cell_height - GRID_WIDTH);

            CubeCellItem *item;

            item = new CubeCellItem(cell_bg);
            item->setPos(cell_pos);
            item->setZValue(5); /* lay in the top*/
            addItem(item);

            item->setOriginalCubePos(row, col);
            b_items[row][col] = item;
        }
    }

    m_white_col = col_size - 1;
    m_white_row = row_size - 1;
}

void CubeScene::drawGrid (int row, int col)
{
    QGraphicsRectItem *item;
    QBrush brush;
    QPen pen;

    brush.setColor(QColor(255, 255, 255, 235));
    brush.setStyle(Qt::Dense6Pattern);
    pen.setColor(QColor(GRID_COLOR));
    pen.setWidth(GRID_WIDTH);

    item = new QGraphicsRectItem(QRectF(
            cell_width * col + x_pad,
            cell_height * row + y_pad,
            cell_width, cell_height));
    item->setBrush(brush);
    item->setPen(pen);
    item->setZValue(3); /* lay in the bottom */

    addItem(item);
}

void CubeScene::startPlay(void)
{
    int row, col;
    short int MASK_CURRENT_MAP[row_size][col_size];

    qDebug() << "Start play.";

    setBgVisible(TRUE);

    for (col = 0; col < col_size; col++) {
        for (row = 0; row < row_size; row++) {
            MASK_CURRENT_MAP[row][col] = 0;
        }
    }

    /* Reorder all items */
    for (col = 0; col < (col_size - 1); col++) {
        for (row = 0; row < row_size; row++) {
            b_curr_items[row][col] = b_items[row][col];
            b_curr_items[row][col]->setCubePos(row, col);
        }
    }

    /* Move cell in the right-bottom into empty cell */
    int lr = row_size - 1;
    int lc = col_size - 2;

    moveCell (b_items[lr][lc]->cubePos(), lr, lc + 1); 
    MASK_CURRENT_MAP[lr][lc] = 1;

    m_white_row = lr;
    m_white_col = lc;

    /* Radomize cubes */
    time_t t = time(NULL);
    srand (t);

    for (col = 0; col < (col_size - 1); col++) {
        for (row = 0; row < row_size; row++) {
            int nrand;
            int r, c;

            /* Only randmoize first 11 cells */
            if (col == (col_size - 2) && row == (row_size - 1) ) {
                break;
            }

            nrand = rand() % ((col_size - 1) * row_size);
            r = nrand % row_size;
            c = nrand % (col_size - 2);

            /* Find unused cell by mask */
            while (MASK_CURRENT_MAP[r][c] != 0) {
                c ++;
                if (c > (col_size - 2)) {
                    c = 0;
                    r++;
                    if (r > (row_size - 1)) {
                        r = 0;
                    }
                }
            }

            MASK_CURRENT_MAP[r][c] = 1;
            b_items[row][col]->setCubePos(r, c);
            b_items[row][col]->setPos(getCellPos(r, c));
            b_curr_items[r][c] = b_items[row][col];

            //qDebug() << "Move item[" << row << "][" << col << "] to: "
            //        << nx << ", " << ny << ". " << n++;
        }
    }

    m_white_row = row_size - 1;
    m_white_col = col_size - 2;
}

void CubeScene::moveAllCell(const QPoint &pos, int off_row, int off_col)
{
    int i, off;

    qDebug() << "Move cells beside the white cell.";

    /* Row move */
    off = off_row > 0 ? -1 : 1;
    for (i = 0; i < abs(off_row); i++ ) {
        moveCell (QPoint(m_white_row + off, m_white_col),
                  m_white_row, m_white_col);
    }

    /* Col move */
    off = off_col > 0 ? -1 : 1;
    for (i = 0; i < abs(off_col); i++ ) {
        moveCell (QPoint(m_white_row, m_white_col + off),
                  m_white_row, m_white_col);
    }
}

void CubeScene::moveCell(const QPoint &pos, int row, int col)
{
    int r, c;
    CubeCellItem *cell = 0;

    qDebug() << "Move cell " << pos << " to: " << row << ", " << col;

    r = pos.x();
    c = pos.y();
    cell = b_curr_items[r][c];

    if (! cell) {
        return;
    }

    /* Cell move */
    cell->setPos(getCellPos(row, col));
    cell->setCubePos(row, col);

    b_curr_items[row][col] = cell;
    b_curr_items[r][c] = 0;

    m_white_row = r;
    m_white_col = c;
}

QPoint CubeScene::getCellPos(int row, int col)
{
    return QPoint (cell_width * col + x_pad + GRID_WIDTH,
                   cell_height * row + y_pad + GRID_WIDTH);
}

void CubeScene::checkAllCell(void)
{
    int row, col;
    int n = 0;

    /* Check all items */
    for (col = 0; col < (col_size - 1); col++) {
        for (row = 0; row < row_size; row++) {
            /* Only count first 11 cells */
            if (col == (col_size - 2) && row == (row_size - 1) ) {
                break;
            }

            CubeCellItem *cell = b_curr_items[row][col];
            if (cell && (cell->cubePos() == cell->originalCubePos())) {
                n++;
            }
        }
    }

    qDebug() << "Finished " << n << " cells";

    if (n == ((col_size - 1) * row_size) - 1) {
        qDebug() << "You win!";
        setBgVisible(FALSE);
    }
}

void CubeScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item pressed: " << curr_pos;
}

void CubeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item moveded: " << curr_pos;
}

QPoint CubeScene::scenePosToVirtual(QPointF pos)
{
	QPoint v;
	float x_ratio = (float) v_width / cube_width;
	float y_ratio = (float) v_height / cube_height;

	v.setX(pos.x() * x_ratio);
	v.setY(pos.y() * y_ratio);

	return v;
}

void CubeScene::sendVirtualClick(QPoint pos)
{
	QProcess p;
	QString cmd = "/usr/bin/adb";
	QStringList args;

	args << "shell" << "input" << "tap";
        args << QString::number(pos.x()) << QString::number(pos.y());

     	qDebug() << "Exec: " << cmd << " " << args;

	p.start(cmd, args);
	p.waitForFinished();
}

void CubeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
     QGraphicsItem *item = 0;
     CubeCellItem *cell = 0;

     qDebug() << "Scene clicked: " << event->scenePos();

     QPoint vpos;

     vpos = scenePosToVirtual(event->scenePos());

     qDebug() << "Virtual pos: " << vpos;

     sendVirtualClick(vpos);

#if 0
     item = itemAt(event->scenePos());
     cell =  dynamic_cast<CubeCellItem *>(item);
     if (!cell) {
         return;
     }

     qDebug() << "Item clicked, curr pos: " << cell->cubePos()
             << ", orig pos: " << cell->originalCubePos();

     QPoint pos;

     pos = cell->originalCubePos();

     /* Check command cell */
     if (THUMNAIL_CELL_IDX == (pos.x() + pos.y())) {
         setBgVisible (!getBgVisible()); /* color egg */
         return;
     }

     /* Check command start */
     if (STARTBUTTON_CELL_IDX == (pos.x() + pos.y())) {
         startPlay();
         return;
     }

     /* Check cell move */
     int off_row, off_col;

     pos = cell->cubePos();
     off_row = m_white_row - pos.x();
     off_col = m_white_col - pos.y();

     if (off_row == 0 || off_col == 0) {
         qDebug() << "Clicked move able cell: " << cell->cubePos()
                 << ", Offset: " << off_row << ", " << off_col;

         moveAllCell (pos, off_row, off_col);

         checkAllCell();
     }
#endif
}

