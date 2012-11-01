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
#include "keymap.h"

#define ANDROID_KEY_HOME	3
#define ANDROID_KEY_BACK	4
#define ANDROID_KEY_MENU	82
#define ANDROID_KEY_ENTER	66
#define ANDROID_KEY_DPAD_UP	19
#define ANDROID_KEY_DPAD_DOWN	20
#define ANDROID_KEY_DPAD_CENTER	23

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
    //qDebug() << "Resize main window" << size;
    setMinimumSize(size);
    resize(size);
}

void CubeView::resizeEvent(QResizeEvent * event)
{
    QSize size = event->size();
    QSize oldSize = event->oldSize();

    //qDebug() << "New view size" << size << oldSize;
    QGraphicsView::resizeEvent(event);
    emit viewSizeChanged(size);
}

CubeScene::CubeScene(QObject * parent) :
    QGraphicsScene(parent)
{
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    pixel_format = 1;
    os_type =  ANDROID_JB;

    pixmap = QPixmap(":/images/sandbox.jpg");
    setItemIndexMethod(QGraphicsScene::NoIndex);
    setBackgroundBrush(QBrush(Qt::gray));

    QObject::connect(&reader, SIGNAL(newFrame(QByteArray *)),
                          this, SLOT(updateFBCell(QByteArray *)));
    QObject::connect(&reader, SIGNAL(deviceDisconnected(void)),
                          this, SLOT(deviceDisconnected(void)));
    QObject::connect(&reader, SIGNAL(deviceWaitTimeout(void)),
                          this, SLOT(deviceDisconnected(void)));

    QObject::connect(&reader, SIGNAL(newFBFound(int, int, int, int)),
                          this, SLOT(newFBFound(int, int, int, int)));

    reader.moveToThread(&fbThread);
    reader.connect(this, SIGNAL(readFrame(void)),
                   SLOT(readFrame(void)));
    reader.connect(this, SIGNAL(waitForDevice(void)),
                   SLOT(waitForDevice(void)));
    reader.connect(&reader, SIGNAL(deviceFound(void)),
                   SLOT(probeFBInfo(void)));

    fbThread.start();
    fbThread.setPriority(QThread::HighPriority);

    adbex.moveToThread(&adbThread);
    adbex.connect(this, SIGNAL(execAdbCmd(const QStringList)),
                  SLOT(execCmd(const QStringList)));
    adbThread.start();

    //TODO: Check and Enable compress here
    reader.setCompress(true);

    initialize();

    emit waitForDevice();
}

CubeScene::~CubeScene()
{
    adbThread.quit();
    fbThread.quit();
    adbThread.wait();
    fbThread.wait();
}

void CubeScene::deviceDisconnected(void)
{
    grayMask.setVisible(true);
    promptItem.setVisible(true);

    emit waitForDevice();
}

void CubeScene::cubeResize(QSize size)
{
    cube_height = size.height() - home->boundingRect().height();
    cube_width = fb_width * ((float) cube_height / fb_height);
    DT_TRACE("New scene size:" << cube_width << cube_height);

    fb.setCellSize(QSize(cube_width, cube_height));
    setMenuIconsPos();

    int height = cube_height + home->boundingRect().height();
    grayMask.setRect(QRect(0, 0, cube_width, height));
    setSceneRect(QRect(0, 0, cube_width, height));
}

void CubeScene::newFBFound(int w, int h, int f, int os)
{
    DT_TRACE("New Remote screen FB:" << w << h << f);

    if (w == fb_width && h == fb_height) {
        //qDebug() << "Remove screen size unchanged.";
        // Start read frame
        emit readFrame();
        return;
    }

    fb_width = w;
    fb_height = h;
    pixel_format = f;
    fb.setFBSize(QSize(fb_width, fb_height));

    cube_height = fb_height * ((float) cube_width / fb_width);

    setMenuIconsPos();

    int height = cube_height + home->boundingRect().height();
    grayMask.setRect(QRect(0, 0, cube_width, height));
    setSceneRect(QRect(0, 0, cube_width, height));

    emit sceneSizeChanged(QSize(cube_width, height));

    os_type = os;

    // Start read frame
    emit readFrame();
}

void CubeScene::updateFBCell(QByteArray *bytes)
{
    int ret;

    //DT_TRACE("New FB frame received");
    grayMask.setVisible(false);
    promptItem.setVisible(false);

    ret = fb.setFBRaw(bytes);

    if (ret == FBCellItem::UPDATE_DONE) {
        reader.setMiniDelay();
    } else {
        reader.increaseDelay();
    }

    emit readFrame();
}

void CubeScene::setMenuIconsPos(void)
{
    int width;
    int padding;
    int margin = 8;
    int num = 3; // TODO: more icons support?

    // Assume that icons has same width
    width = home->boundingRect().width();
    padding = (cube_width - width * num - margin * 2) / (num - 1);
    menu ->setPos(fb.boundingRect().bottomLeft() + QPoint(margin, 0));
    home ->setPos(menu->pos() + QPoint(width + padding, 0));
    back->setPos(home->pos() + QPoint(width + padding, 0));
}

CubeCellItem *CubeScene::createCellItem(const char* name, int size, int key)
{
    CubeCellItem *item;
    QPixmap p;

    p =  QPixmap(name).scaled(
                QSize (size, size),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
    item = new CubeCellItem(p);
    item->setKey(key);

    return item;
}

void CubeScene::initialize (void)
{
    if (! pixmap.width()) {
        pixmap = QPixmap(DEFAULT_FB_WIDTH, DEFAULT_FB_HEIGHT);
        pixmap.fill(Qt::black);
    }

    cube_width = pixmap.width();
    cube_height = fb_height * ((float) cube_width / fb_width);
    //qDebug() << "Cube size:" << cube_width << cube_height;

    fb.setPixmap(pixmap.scaled(QSize(cube_width, cube_height)));
    fb.setPos(QPoint(0, 0));
    fb.setZValue(0); /* lay in the bottom*/
    fb.setFBSize(QSize(fb_width, fb_height));
    fb.setBPP(3); // We converted data in the reader.
    fb.setVisible(true);
    addItem(&fb);

    promptItem.setText("ADB wait...");
    promptItem.setBrush(QBrush(QColor(250, 250, 10, 255)));
    promptItem.setPen(QPen(QColor(20, 20, 20)));
    promptItem.setFont(QFont("Arail", 16, QFont::Bold));
    promptItem.setPos(20, 20);
    promptItem.setZValue(100); /* lay in the top*/
    promptItem.setVisible(true);
    addItem(&promptItem);

    home = createCellItem(":/images/ic_menu_home.png", KEY_BTN_SIZE, ANDROID_KEY_HOME);
    back = createCellItem(":/images/ic_menu_revert.png", KEY_BTN_SIZE, ANDROID_KEY_BACK);
    menu = createCellItem(":/images/ic_menu_copy.png", KEY_BTN_SIZE, ANDROID_KEY_MENU);
    addItem(home);
    addItem(back);
    addItem(menu);
    setMenuIconsPos();

    grayMask.setRect(QRectF(0, 0, cube_width,
                            cube_height + home->boundingRect().height()));
    grayMask.setBrush(QBrush(QColor(128, 128, 128, 135)));
    grayMask.setPen(Qt::NoPen);
    grayMask.setZValue(99);
    grayMask.setVisible(true);
    addItem(&grayMask);

    pointer = createCellItem(":/images/pointer_spot_anchor.png",
                             POINTER_ANCHOR_SIZE);
    grayMask.setZValue(101);
    pointer->setVisible(false);
    addItem(pointer);

    unsigned int i;
    for (i = 0; i < KEY_NUM; i++) {
	    keys[keymaps[i].q] = keymaps[i].a;
    }
}

void CubeScene::setPointerPos(QPointF pos, bool visible)
{
    QRectF s = pointer->boundingRect();

    pointer->setPos(pos - QPoint(s.width() / 2, s.height() / 2));
    pointer->setVisible(visible);
    pointer->update(s);
}

void CubeScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    setPointerPos(pos, true);

    if (poinInFB(pos)) {
        sendVirtualClick(pos, true, false);
        return;
    }
}

void CubeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    setPointerPos(pos, true);

#if 0
    // Disable send mouse move event because is too slow
    // to send event in time to device, even we filter some
    // out.
    if (poinInFB(pos)) {
        sendVirtualClick(pos, false, false);
        return;
    }
#endif
}

void CubeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    CubeCellItem *cell = 0;
    QPointF pos = event->scenePos();

    DT_TRACE("SCREEN" << pos.x() << pos.y());
    setPointerPos(pos, false);

    if (! reader.isConnected()) {
        return;
    }

    if (poinInFB(pos)) {
        sendVirtualClick(pos, false, true);
        return;
    }

    // Virtual key on the scene bottom
    cell = dynamic_cast<CubeCellItem *>(itemAt(pos));

    if (cell) {
        //qDebug() << "Virtual key on screen" << cell->key();
        sendVirtualKey(cell->key());
        return;
    }
}

QPoint CubeScene::scenePosToVirtual(QPointF pos)
{
    return QPoint((pos.x() * fb_width / cube_width),
                  (pos.y() * fb_height / cube_height));
}

bool CubeScene::poinInFB(QPointF pos)
{
    // TODO: don't use cube_width as fb_width
    return QRectF(0, 0, cube_width, cube_height).contains(pos);
}

void CubeScene::sendVirtualClick(QPointF scene_pos,
                                 bool press, bool release)
{
    QPoint pos;

    reader.setDelay(0);

    pos = scenePosToVirtual(scene_pos);
    DT_TRACE("CLICK" << pos.x() << pos.y() << press << release);

    switch(os_type) {
    case ANDROID_ICS:
        sendEvent(pos, press, release);
        break;
    case ANDROID_JB:
        // Mouse move, ignored.
        // Both true is impossible
        if (press || release) {
            sendTap(pos, press, release);
        }
        break;
    default:
        qDebug() << "Unknown OS type, click dropped.";
    }
}

void CubeScene::sendTap(QPoint pos, bool press, bool release)
{
    bool isTap = false;

    if (press) {
        posPress = pos;
        return;
    }

    isTap = QRect(-1, -1, 2, 2).contains(pos - posPress);
    //qDebug() << "Tap as swipe" << isTap;

    cmds.clear();
    cmds << "shell";

    if (isTap) {
        cmds << "input tap";
    } else {
        cmds << "input swipe";
        cmds << QString::number(posPress.x());
        cmds << QString::number(posPress.y());
    }

    cmds << QString::number(pos.x());
    cmds << QString::number(pos.y());
    //qDebug() << cmds;

    emit execAdbCmd(cmds);
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

void CubeScene::sendEvent(QPoint pos, bool press, bool release)
{
#if 0
    //Disabled mouse event filter.
#define MMSIZE 10
    bool ignored = false;
    QRect rect;

    if (press) {
        posPrevious = pos;
    };

    rect = QRect(-(MMSIZE / 2), -(MMSIZE / 2), MMSIZE, MMSIZE);
    ignored = rect.contains(pos - posPrevious);

    if (! press && ! release && ignored) {
        qDebug() << "Ignore pos" << pos;
        return;
    } else {
        posPrevious = pos;
    }
#endif
    cmds.clear();
    cmds << "shell";

    cmds << newEventCmd(3, 0x35, pos.x());
    cmds << newEventCmd(3, 0x36, pos.y());
    if (press) {
        cmds << newEventCmd(1, 0x14a, 1);
    }

    cmds << newEventCmd(3, 0, pos.x());
    cmds << newEventCmd(3, 1, pos.y());
    cmds << newEventCmd(0, 0, 0);

    if (release) {
        cmds << newEventCmd(1, 0x14a, 0);
        cmds << newEventCmd(0, 0, 0);
    }
    //DT_TRACE("Send ICS mouse event" << pos << press << release);

    emit execAdbCmd(cmds);
}

void CubeScene::sendVirtualKey(int key)
{
    cmds.clear();
    cmds << "shell" << "input keyevent";
    cmds << QString::number(key);

    DT_TRACE("KEY" << key);
    reader.setDelay(0);

    emit execAdbCmd(cmds);
}

void CubeScene::keyReleaseEvent(QKeyEvent * event)
{
    int key, vkey = 0;

    if (! reader.isConnected()) {
        return;
    }

    key = event->key();
    vkey = keys[key];

    if (vkey == 0) {
        DT_ERROR("Unknown key pressed:" << key);
    }

    sendVirtualKey(vkey);
}
