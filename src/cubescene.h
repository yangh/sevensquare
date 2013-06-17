/*
 * cubescene.h
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#ifndef CUBESCENE_H
#define CUBESCENE_H

#include <QSize>
#include <QString>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsItemGroup>
#include <QMutex>
#include <QKeyEvent>
#include <QWidget>
#include <QThread>
#include <QTimer>
#include <QMap>

#include <QGraphicsView>
#include <QCoreApplication>

#include "cubecellitem.h"
#include "fbcellitem.h"
#include "adbfb.h"
#include "debug.h"

#define WINDOW_BORDER 2
#define KEY_BTN_SIZE  32
#define POINTER_ANCHOR_SIZE 24

enum {
    PORTRAIT = 90,
    LANDSCAPE = 270
};

class CubeView;

class CubeScene : public QGraphicsScene
{
    Q_OBJECT

    friend class CubeView;

public:
    CubeScene(QObject * parent = 0);
    ~CubeScene();

    void initialize (void);

    bool sendVirtualClick(QPoint pos, bool, bool);
    bool sendVirtualKey(int key);
    void setIconOffset(float v) { iconOffset = v; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void keyReleaseEvent(QKeyEvent * event);

    CubeCellItem *createCellItem(const char* name, int size, int key = 0);
    void setMenuIconsPos(void);
    void switchMenuIcons(void);
    void setPointerPos(QPointF, bool);
    bool poinInFB(QPointF);
    bool isConnectedAndWakedup(bool doWakeup);

public slots:
    void newFBFound(int, int, int);
    void newFBFormat(int);
    void updateFBCell(QByteArray *);
    void deviceConnected(void);
    void deviceDisconnected(void);
    void deviceScreenTurnedOff(void);
    void deviceScreenTurnedOn(void);
    void cubeResize(QSize);
    void showPromptMessage(QString);
    void hidePrompt(void);
    void adbExecError(void);
    void switchOrientation(void);

signals:
    void sceneSizeChanged(QSize);
    void execAdbCmd(const QStringList);
    void waitForDevice(void);
    void wakeUpDevice(void);
    void updateDeviceBrightness(void);
    void readFrame(void);

    void newVirtualClick(QPoint, bool, bool);
    void newVirtualKey(int key);

private:
    FBCellItem fb;
    QGraphicsRectItem grayMask;
    QGraphicsSimpleTextItem promptItem;
    CubeCellItem *ghost;
    CubeCellItem *home;
    CubeCellItem *back;
    CubeCellItem *menu;
    CubeCellItem *pointer;
    float iconOffset;
    bool showMenuIcon;

    int fb_width;
    int fb_height;
    int cube_width;
    int cube_height;
    unsigned long waitCount;

    QMutex update_mutex;
    QPixmap pixmap;

    ADBFrameBuffer reader;
    QThread fbThread;

    ADBDevice adbex;
    QThread adbThread;

    // Qt key, Android key map
    QMap<int, int> keys;
};


class CubeView : public QGraphicsView
{
    Q_OBJECT

public:
    CubeView(QWidget * parent = 0);

protected:
    void switchOrientation(void);
    void keyReleaseEvent ( QKeyEvent * event );
    void resizeEvent ( QResizeEvent * event );

public slots:
    void cubeSizeChanged(QSize);

signals:
    void viewSizeChanged(QSize);

private:
    QTimer timer;
    QSize delayedSize;
    CubeScene scene;
    int mOrientation;
};

#endif // CUBESCENE_H
