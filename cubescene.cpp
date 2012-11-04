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

void CubeView::keyReleaseEvent(QKeyEvent * event)
{
    bool ctrlPressed = event->modifiers() & Qt::ControlModifier;

    if ( ctrlPressed && event->key() == Qt::Key_W) {
        DT_TRACE("Good Luck, Be Happy!");
        QCoreApplication::quit();
    }
}

CubeScene::CubeScene(QObject * parent) :
    QGraphicsScene(parent)
{
    fb_width = DEFAULT_FB_WIDTH;
    fb_height = DEFAULT_FB_HEIGHT;
    pixel_format = 1;
    waitCount = 1;

    setItemIndexMethod(QGraphicsScene::NoIndex);
    setBackgroundBrush(QBrush(Qt::gray));

    QObject::connect(&reader, SIGNAL(newFrame(QByteArray *)),
                          this, SLOT(updateFBCell(QByteArray *)));
    QObject::connect(&reader, SIGNAL(deviceDisconnected(void)),
                          this, SLOT(deviceDisconnected(void)));
    QObject::connect(&reader, SIGNAL(deviceWaitTimeout(void)),
                          this, SLOT(deviceDisconnected(void)));
    QObject::connect(&reader, SIGNAL(newFBFound(int, int, int)),
                          this, SLOT(newFBFound(int, int, int)));
    this->connect(&reader, SIGNAL(deviceFound()),
                  SLOT(deviceConnected()));
    this->connect(&adbex, SIGNAL(screenTurnedOff()),
                  SLOT(deviceScreenTurnedOff()));
    this->connect(&adbex, SIGNAL(screenTurnedOn()),
                  SLOT(deviceScreenTurnedOn()));
    this->connect(&adbex, SIGNAL(newPropmtMessae(QString)),
                  SLOT(showPromptMessage(QString)));

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
                  SLOT(execCommand(const QStringList)));
    adbex.connect(&reader, SIGNAL(deviceFound()),
                  SLOT(probeDevicePowerKey(void)));
    adbex.connect(this, SIGNAL(wakeUpDevice()),
                  SLOT(wakeUpDevice()));
    adbex.connect(this, SIGNAL(updateDeviceBrightness()),
                  SLOT(updateDeviceBrightness()));
    adbex.connect(this, SIGNAL(newVirtualClick(QPoint,bool,bool)),
                  SLOT(sendVirtualClick(QPoint,bool,bool)));
    adbex.connect(this, SIGNAL(newVirtualKey(int)),
                  SLOT(sendVirtualKey(int)));
    adbThread.start();

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

void CubeScene::showPromptMessage(QString msg)
{
    grayMask.setVisible(true);
    promptItem.setText(msg);
    promptItem.setVisible(true);
}

void CubeScene::hidePrompt(void)
{
    promptItem.setVisible(false);
    grayMask.setVisible(false);
}

void CubeScene::deviceConnected(void)
{
    showPromptMessage("Connected...");
    ghost->setVisible(false);
}

void CubeScene::deviceDisconnected(void)
{
    QString bubble("Waiting");

    for (unsigned long i = 0; i < waitCount % 5; i++)
        bubble.append(".");

    showPromptMessage(bubble);
    waitCount++;

    if ((waitCount % 6) == 0) {
        QRectF rect = ghost->boundingRect();
        QPointF pos;

        qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));
        pos.setX(qrand() % (cube_width - (int)rect.width()));
        pos.setY(qrand() % (cube_height - (int)rect.height()));
        ghost->setPos(pos);
        ghost->setRotation(0.);
        ghost->setVisible(true);
    }

    emit waitForDevice();
}

void CubeScene::deviceScreenTurnedOff(void)
{
    showPromptMessage("Click to wakeup...");
}

void CubeScene::deviceScreenTurnedOn(void)
{
    emit readFrame();
    hidePrompt();
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
    promptItem.setPos(20, cube_height/2);
    setSceneRect(QRect(0, 0, cube_width, height));
}

void CubeScene::newFBFound(int w, int h, int f)
{
    DT_TRACE("New Remote screen FB:" << w << h << f);

    if (w == fb_width && h == fb_height) {
        //qDebug() << "Remote screen size unchanged.";
        // Start read frame
        emit readFrame();
        return;
    }

    fb_width = w;
    fb_height = h;
    pixel_format = f;
    fb.setFBSize(QSize(fb_width, fb_height));
    fb.setFBDataFormat(f);

    cube_height = fb_height * ((float) cube_width / fb_width);

    setMenuIconsPos();

    int height = cube_height + home->boundingRect().height();
    grayMask.setRect(QRect(0, 0, cube_width, height));
    setSceneRect(QRect(0, 0, cube_width, height));

    emit sceneSizeChanged(QSize(cube_width, height));

    // Start read frame
    emit readFrame();
}

void CubeScene::updateFBCell(QByteArray *bytes)
{
    int ret;
    unsigned long delay;

    if (! adbex.screenIsOn()) {
        return;
    }

    emit readFrame();

    //DT_TRACE("New FB frame received");
    hidePrompt();

    ret = fb.setFBRaw(bytes);

    if (ret == FBCellItem::UPDATE_DONE) {
        reader.setMiniDelay();
    } else {
        delay = reader.increaseDelay();

        if (delay >= FBEx::DELAY_NORMAL) {
            emit updateDeviceBrightness();
        }
    }
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
    menu ->setCubePos(fb.boundingRect().bottomLeft() + QPoint(margin, 0));
    home ->setCubePos(menu->pos() + QPoint(width + padding, 0));
    back->setCubePos(home->pos() + QPoint(width + padding, 0));
}

CubeCellItem *CubeScene::createCellItem(const char* name, int size, int key)
{
    CubeCellItem *item;
    QPixmap p;

    p =  QPixmap(name);

    if (size > 0) {
        p = p.scaled(
                    QSize (size, size),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation);
    }

    item = new CubeCellItem(p);
    item->setKey(key);
    item->setCube(this);

    return item;
}

void CubeScene::initialize (void)
{
    pixmap = QPixmap(":/images/sandbox.jpg");

    if (! pixmap.width()) {
        pixmap = QPixmap(DEFAULT_FB_WIDTH, DEFAULT_FB_HEIGHT);
        pixmap.fill(Qt::black);
    } else {
        pixmap = pixmap.scaled(QSize(DEFAULT_FB_WIDTH, DEFAULT_FB_HEIGHT),
                               Qt::IgnoreAspectRatio,
                               Qt::SmoothTransformation);
    }

    cube_width = fb_width;
    cube_height = fb_height;

    fb.setPixmap(pixmap);
    fb.setPos(QPoint(0, 0));
    fb.setZValue(0); /* lay in the bottom*/
    fb.setFBSize(QSize(fb_width, fb_height));
    fb.setVisible(true);
    fb.setCube(this);
    addItem(&fb);

    promptItem.setText("Waiting...");
    promptItem.setBrush(QBrush(QColor(QColor(0, 153,204))));
    promptItem.setPen(QPen(QColor(20, 20, 20)));
    promptItem.setFont(QFont("Arail", 16, QFont::Bold));
    promptItem.setPos(20, cube_height / 2);
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

    int total_height = cube_height + home->boundingRect().height();
    grayMask.setRect(QRectF(0, 0, cube_width, total_height));
    grayMask.setBrush(QBrush(QColor(128, 128, 128, 140)));
    grayMask.setPen(Qt::NoPen);
    grayMask.setZValue(99);
    grayMask.setVisible(true);
    addItem(&grayMask);

    pointer = createCellItem(":/images/pointer_spot_anchor.png",
                             POINTER_ANCHOR_SIZE);
    pointer->setZValue(101);
    pointer->setVisible(false);
    addItem(pointer);

    ghost = createCellItem(":/images/android-logo.png", 0);
    ghost->setZValue(98);
    ghost->setVisible(false);
    addItem(ghost);

    unsigned int i;
    for (i = 0; i < KEY_NUM; i++) {
	    keys[keymaps[i].q] = keymaps[i].a;
    }
}

void CubeScene::setPointerPos(QPointF pos, bool visible)
{
    QRectF s = pointer->boundingRect();

    pointer->setVisible(visible);
    pointer->setPos(pos.toPoint() - QPoint(s.width() / 2, s.height() / 2));
    //pointer->update(s);

    if(! adbex.screenIsOn()) {
        QRectF rect = pointer->boundingRect();
        QRectF grect = ghost->boundingRect();
#if 0
        // Follow the pointer
        ghost->setPos(pointer->pos()
                      + rect.topRight()
                      + QPointF(0, (rect.height() - grect.height()) / 2));
#else
        QPointF pos;

        // Random position
        pos.setX(qrand() % (cube_width - (int)grect.width()));
        pos.setY(qrand() % (cube_height - (int)grect.height()));
        ghost->setRotation(qrand());
        ghost->setPos(pos);
#endif
        ghost->setVisible(visible);
    }
}

void CubeScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    QGraphicsScene::mousePressEvent(event);
    setPointerPos(pos, true);
}

void CubeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    QGraphicsScene::mouseMoveEvent(event);
    setPointerPos(pos, true);
}

void CubeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF pos = event->scenePos();

    QGraphicsScene::mouseReleaseEvent(event);
    setPointerPos(pos, false);
}

bool CubeScene::poinInFB(QPointF pos)
{
    // TODO: don't use cube_width as fb_width
    return QRectF(0, 0, cube_width, cube_height).contains(pos);
}

void CubeScene::keyReleaseEvent(QKeyEvent * event)
{
    int key;

    key = event->key();
    if (sendVirtualKey(keys[key])) {
        return;
    }

    DT_ERROR("Unknown key pressed:" << key);
    QGraphicsScene::keyReleaseEvent(event);
}

bool CubeScene::isConnectedAndWakedup()
{
    if (! reader.isConnected()) {
        return false;
    }

    if (! adbex.screenIsOn()) {
        emit wakeUpDevice();
        return true;
    }

    return true;
}

bool CubeScene::sendVirtualClick(QPoint pos,
                                 bool press, bool release)
{
    if (! isConnectedAndWakedup()) {
        return true;
    }

    // Read frame asap
    reader.setDelay(0);

    emit newVirtualClick(pos, press, release);

    return true;
}

bool CubeScene::sendVirtualKey(int key)
{

    if (! isConnectedAndWakedup()) {
        return true;
    }

    if (key > 0) {
        // Read frame asap
        reader.setDelay(0);
        emit newVirtualKey(key);
        return true;
    }

    return false;
}
