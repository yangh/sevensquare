/*
 * cubescene.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QThread>

#include <stdlib.h>
#include <time.h>
#include <strings.h>
#include <stdint.h>
#include <zlib.h>

#include "cubescene.h"

CubeView::CubeView(QWidget * parent) :
        QGraphicsView(parent)
{
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setCacheMode(QGraphicsView::CacheBackground);
	setRenderHints(QPainter::Antialiasing
			| QPainter::SmoothPixmapTransform
			| QPainter::TextAntialiasing);
}

void CubeView::cubeSizeChanged(QSize size)
{
	size += QSize(WINDOW_BORDER, WINDOW_BORDER);
	qDebug() << "Resize main window" << size;
	setMinimumSize(size);
	resize(size);
}

void CubeView::resizeEvent(QResizeEvent * event)
{
	QGraphicsView::resizeEvent(event);

	//TODO: Resize scene as required.
}

CubeScene::CubeScene(QObject * parent) :
        QGraphicsScene(parent)
{
	cell_width = DEFAULT_CELL_WIDTH;
	fb_width = DEFAULT_FB_WIDTH;
	fb_height = DEFAULT_FB_HEIGHT;
	pixel_format = 1;
	os_type =  ANDROID_JB;

	pixmap = QPixmap(":/images/sandbox.jpg");
	setItemIndexMethod(QGraphicsScene::NoIndex);
	setBackgroundBrush(pixmap);

	QObject::connect(&reader, SIGNAL(newFbReceived(QByteArray*)),
			this, SLOT(updateScene(QByteArray*)));
	QObject::connect(&reader, SIGNAL(deviceDisconnected(void)),
			this, SLOT(fbDisconnected(void)));
	QObject::connect(&reader, SIGNAL(newFBFound(int, int, int, int)),
			this, SLOT(deviceConnected(int, int, int, int)));

	//TODO: Check and Enable compress here
	reader.setCompress(true);

	initialize();
	startFBReader();
}

CubeScene::~CubeScene()
{
	stopFBReader();
}

void CubeScene::startFBReader()
{
	reader.startRead();
	reader.setPriority(QThread::HighPriority);
}

void CubeScene::stopFBReader()
{
	reader.stopRead();
}

void CubeScene::fbDisconnected(void)
{
	fb.setFBConnected(false);
	grayMask.setVisible(true);
	promptItem.setVisible(true);
}

void CubeScene::deviceConnected(int w, int h, int f, int os)
{
	if (w == fb_width && h == fb_height) {
		qDebug() << "Remove screen size unchanged.";
		return;
	}

	fb_width = w;
	fb_height = h;
	pixel_format = f;

	qDebug() << "Remote new screen FB:" << fb_width << fb_height << pixel_format;
	fb.setFBSize(QSize(fb_width, fb_height));

	cube_height = fb_height * ((float) cube_width / fb_width);
	qDebug() << "New screne size:" << cube_width << cube_height;

	grayMask.setRect(QRect(0, 0, cube_width, cube_height));
	setSceneRect(QRect(0, 0, cube_width, cube_height));
	emit sceneSizeChanged(QSize(cube_width, cube_height));

	os_type = os;
}

void CubeScene::updateScene(QByteArray *bytes)
{
	int ret;

	fb.setFBConnected(true);
	grayMask.setVisible(false);
	promptItem.setVisible(false);

	ret = fb.setFBRaw(bytes);

	if (ret == FBCellItem::UPDATE_DONE) {
		reader.setMiniDelay();
	} else {
		reader.increaseDelay();
	}
}

void CubeScene::initialize (void)
{
    int row, col;

    if (! pixmap.width()) {
	    pixmap = QPixmap(DEFAULT_FB_WIDTH, DEFAULT_FB_HEIGHT);
	    pixmap.fill(Qt::black);
    }

    cube_width = pixmap.width();
    cube_height = fb_height * ((float) cube_width / fb_width);
    qDebug() << "Cube size:" << cube_width << cube_height;

    row_size = ROW_NUM;
    col_size = COL_NUM;

    cell_width = (cube_width - GRID_WIDTH * col_size * 2) / col_size;
    cell_height = (cube_height - GRID_WIDTH * row_size * 2) / row_size;

    x_pad = (cube_width - cell_width * col_size) / 2;
    y_pad = (cube_height - cell_height * row_size) / 2;

    fb.setPixmap(pixmap.scaled(QSize(cube_width, cube_height)));
    fb.setPos(QPoint(0, 0));
    fb.setZValue(0); /* lay in the bottom*/
    fb.setFBSize(QSize(fb_width, fb_height));
    fb.setBPP(3); // We converted data in the reader.
    grayMask.setVisible(true);
    addItem(&fb);

    grayMask.setRect(QRectF(0, 0, cube_width, cube_height));
    grayMask.setBrush(QBrush(QColor(128, 128, 128, 135)));
    grayMask.setPen(Qt::NoPen);
    grayMask.setZValue(99);
    grayMask.setVisible(true);
    addItem(&grayMask);

    promptItem.setText("ADB wait...");
    promptItem.setBrush(QBrush(QColor(240, 240, 70)));
    promptItem.setPen(QPen(QColor(20, 20, 20)));
    promptItem.setFont(QFont("Arail", 16, QFont::Bold));
    promptItem.setPos(20, 20);
    promptItem.setZValue(100); /* lay in the top*/
    promptItem.setVisible(true);
    addItem(&promptItem);

#if 0
    QPixmap cell_bg;
    QPoint cell_pos;
    CubeCellItem *item;

    /* TODO: add virtual key */
    for (col = 0; col < col_size; col++) {
        for (row = 0; row < row_size; row++) {

            qDebug() << "Init cell: " << row << ", " << col;
            cell_pos = getCellPos(row, col);
            cell_bg = pixmap.copy(cell_pos.x(), cell_pos.y(),
                                  cell_width - GRID_WIDTH,
                                  cell_height - GRID_WIDTH);

            item = new CubeCellItem(cell_bg);
            item->setPos(cell_pos);
            item->setZValue(5); /* lay in the top*/
            addItem(item);

            item->setOriginalCubePos(row, col);
            b_items[row][col] = item;
        }
    }
#endif
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
	return QPoint((pos.x() * fb_width / cube_width),
		      (pos.y() * fb_height / cube_height));
}

void CubeScene::sendVirtualClick(QPoint pos)
{
	if (pos.x() < 0 || pos.y() < 0
		|| pos.x() > fb_width
		|| pos.y() > fb_height)
	{
		qDebug() << "Out of range click" << pos;
		return;
	}

	DT_TRACE("CLICK" << pos);
	reader.setDelay(0);

	switch(os_type) {
		case ANDROID_ICS:
			sendEvent(pos);
			break;
		case ANDROID_JB:
			sendTap(pos);
			break;
		default:
			qDebug() << "Unknown OS type, click dropped.";
	}
}

void CubeScene::sendTap(QPoint pos)
{
	AdbExecutor adb;
	QStringList args;

	args << "shell" << "input tap";
        args << QString::number(pos.x());
	args << QString::number(pos.y());

	adb.clear();
	adb.addArg(args);
	adb.run();
}

QStringList CubeScene::newEventCmd (int type, int code, int value)
{
	QStringList event;

	//TODO: Use correct dev to send event
	event << "sendevent" << "/dev/input/event0";
	event << QString::number(type);
	event << QString::number(code);
	event << QString::number(value);
	event << ";";

	return event;
}

void CubeScene::sendEvent(QPoint pos)
{
	AdbExecutor adb;
	QStringList events;

	events << newEventCmd(3, 0x35, pos.x());
	events << newEventCmd(3, 0x36, pos.y());
	events << newEventCmd(1, 0x14a, 1);

	events << newEventCmd(3, 0, pos.x());
	events << newEventCmd(3, 1, pos.y());
	events << newEventCmd(0, 0, 0);

	events << newEventCmd(1, 0x14a, 0);
	events << newEventCmd(0, 0, 0);

	adb.clear();
	adb.addArg("shell");
	adb.addArg(events);
	adb.run();
}

void CubeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
     QPoint vpos;

     DT_TRACE("SCREEN" << event->scenePos());

     vpos = scenePosToVirtual(event->scenePos());
     sendVirtualClick(vpos);

#if 0
     QGraphicsItem *item = 0;
     CubeCellItem *cell = 0;

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

void CubeScene::sendVirtualKey(int key)
{
	AdbExecutor adb;
	QStringList args;

	args << "shell" << "input keyevent";
        args << QString::number(key);

	DT_TRACE("KEY" << key);
	reader.setDelay(0);

	adb.clear();
	adb.addArg(args);
	adb.run();
}

#define ANDROID_KEY_HOME	3
#define ANDROID_KEY_BACK	4
#define ANDROID_KEY_MENU	82
#define ANDROID_KEY_ENTER	66
#define ANDROID_KEY_DPAD_UP	19
#define ANDROID_KEY_DPAD_DOWN	20
#define ANDROID_KEY_DPAD_CENTER	23

void CubeScene::keyReleaseEvent(QKeyEvent * event)
{
	int key;

	key = event->key();

	switch(key) {
	case Qt::Key_B:
		sendVirtualKey(ANDROID_KEY_BACK);
		break;
	case Qt::Key_H:
		sendVirtualKey(ANDROID_KEY_HOME);
		break;
	case Qt::Key_M:
		sendVirtualKey(ANDROID_KEY_MENU);
		break;

	case Qt::Key_J:
	case Qt::Key_Up:
		sendVirtualKey(ANDROID_KEY_DPAD_UP);
		break;
	case Qt::Key_K:
	case Qt::Key_Down:
		sendVirtualKey(ANDROID_KEY_DPAD_DOWN);
		break;
	case Qt::Key_G:
	case Qt::Key_Enter: // Why no action?
	case Qt::Key_Space:
		sendVirtualKey(ANDROID_KEY_DPAD_CENTER);
		break;
	}
}

