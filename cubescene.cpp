#include <QGraphicsRectItem>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QStringList>
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QKeyEvent>

#include <stdlib.h>
#include <time.h>
#include <strings.h>
#include <stdint.h>

#include "cubescene.h"

FbReader::FbReader(QObject * parent) :
	QThread(parent)
{
	do_compress = false;
	delay = 300;
	stopped = false;
}

bool FbReader::supportCompress()
{
	return false;
}

void FbReader::setCompress(bool value)
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

	return v;
}

QByteArray AndrodDecompress(QByteArray &compressed)
{
	QByteArray bytes;

	//TODO: Decompress from android minizip
	bytes = compressed;

	return bytes;
}

int FbReader::screenCap(QByteArray &bytes, bool compress = false)
{
	int ret;
	AdbExecutor adb;

	adb.clear();
	adb.addArg("shell");
	adb.addArg("screencap");

	if (compress)
		adb.addArg("| gzip");

	ret = adb.run();

	if (adb.exitSuccess()) {
		if (compress) {
			bytes = AndrodDecompress(adb.output);
		} else {
			bytes = adb.output;
		}
	} else {
		adb.printErrorInfo();
	}

	//<< QDateTime::currentMSecsSinceEpoch() / 1000;

	return ret;
}

int FbReader::getScreenInfo(int &width, int &height, int &format)
{
	QByteArray bytes;
	int ret;

	ret = screenCap(bytes);

	if (ret != 0)
		return -1;

	width = bigEndianToInt32(bytes.mid(0, 4));
	height = bigEndianToInt32(bytes.mid(4, 4));
	format = bigEndianToInt32(bytes.mid(8, 4));

	//TODO: emit screenInfoChanged

	return 0;
}

void FbReader::run()
{
	QByteArray bytes;
	int ret;

	while (1) {
		ret = screenCap(bytes, do_compress);
		int ms = delay;

		if (ret == 0) {
			emit newFbReceived(&bytes);
		} else {
			ms = DELAY_MAX;
		}

		//qDebug() << "Delay:" << ms<< "ms";
		mutex.lock();
		readDelay.wait(&mutex, ms);
		mutex.unlock();

		if (stopped) {
			qDebug() << "FbReader stopped";
			break;
		}
	}

	return;
}

CubeScene::CubeScene(QObject * parent) :
        QGraphicsScene(parent)
{
    cell_width = DEFAULT_CELL_WIDTH;

    fb_width = 480;
    fb_height = 800;

    reader = new FbReader(parent);

    reader->getScreenInfo(fb_width, fb_height, pixel_format);
    qDebug() << "Screen:" << fb_width << fb_height << pixel_format;

    QObject::connect(reader, SIGNAL(newFbReceived(QByteArray*)),
		    this, SLOT(updateSceen(QByteArray*)));

    //TODO: Check and Enable compress here
    reader->setCompress(false);
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
	reader->stop();
	reader->quit();
}

void CubeScene::updateSceen(QByteArray *bytes) {
	int ret;

	ret = bg_mask->setFBRaw(bytes);
	if (ret == FBCellItem::UPDATE_DONE) {
		reader->setMiniDelay();
	} else {
		reader->IncreaseDelay();
	}
}

void CubeScene::loadImage (const QString &file)
{
    qDebug() << "Load image from: " << file;

    pixmap.load (file, 0);
    initialize();
}

void CubeScene::initialize (void)
{
    int row, col;
    QPixmap pixmap_scaled;

    qDebug() << "Scene initialize.";

    cube_width = pixmap.width();
    cube_height = fb_height * ((float) cube_width / fb_width);
    qDebug() << "Cube : " << cube_width << cube_height;

    row_size = ROW_NUM;
    col_size = COL_NUM;

    cell_width = (cube_width - GRID_WIDTH * col_size * 2) / col_size;
    cell_height = (cube_height - GRID_WIDTH * row_size * 2) / row_size;

    x_pad = (cube_width - cell_width * col_size) / 2;
    y_pad = (cube_height - cell_height * row_size) / 2;

    pixmap_scaled = pixmap.copy(0, 0, cube_width, cube_height);

    /* Background */
    setBackgroundBrush(QBrush(pixmap_scaled));

    FBCellItem *fb;

    fb = new FBCellItem(pixmap_scaled);
    fb->setPos(QPoint(0, 0));
    fb->setZValue(0); /* lay in the bottom*/
    fb->setFBConnected(true);
    fb->setFBSize(QSize(fb_width, fb_height));
    addItem(fb);

    bg_mask = fb;

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
	QPoint v;
	float x_ratio = (float) fb_width / cube_width;
	float y_ratio = (float) fb_height / cube_height;

	v.setX(pos.x() * x_ratio);
	v.setY(pos.y() * y_ratio);

	return v;
}

void CubeScene::sendVirtualClick(QPoint pos)
{
	bool isIcs = true;

	if (pos.x() < 0 || pos.y() < 0
		|| pos.x() > fb_width
		|| pos.y() > fb_height)
	{
		qDebug() << "Out of range click" << pos;
		return;
	}

	// TODO: Check device version to send event in diff way
	if (isIcs) {
		sendEvent(pos);
	} else {
		sendTap(pos);
	}

	reader->setDelay(0);
}

void CubeScene::sendTap(QPoint pos)
{
	AdbExecutor adb;
	QStringList cmds;

	cmds << "shell" << "input tap";
        cmds << QString::number(pos.x());
	cmds << QString::number(pos.y());

	adb.clear();
	adb.addArg(cmds);
	adb.run();
}

QStringList CubeScene::newEventCmd (int type, int code, int value)
{
	QStringList event;

	event.clear();

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

void CubeScene::sendVirtualKey(int key)
{
	AdbExecutor adb;
	QStringList cmds;

	cmds << "shell" << "input keyevent";
        cmds << QString::number(key);

	adb.clear();
	adb.addArg(cmds);
	adb.run();

	reader->setDelay(0);
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

