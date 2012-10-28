/*
 * cubescene.h
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#ifndef CUBESCENE_H
#define CUBESCENE_H

#include <QSize>
#include <QString>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMutex>
#include <QKeyEvent>

#include "cubecellitem.h"
#include "fbcellitem.h"
#include "adbfb.h"
#include "debug.h"

#define DEFAULT_CELL_WIDTH 120
#define MIN_PAD 15
#define MAX_ROW_COL_SIZE 16

#define COL_NUM 3
#define ROW_NUM 5

#define GRID_COLOR 120, 120, 120
#define GRID_WIDTH 1

#define WHITE_CELL_COLOR "White"
#define WHITE_CELL_POS -1
#define WHITE_CELL_IDX (WHITE_CELL_POS * 2)

#define THUMNAIL_X_PAD 6
#define THUMNAIL_CELL_POS -2
#define THUMNAIL_CELL_IDX (THUMNAIL_CELL_POS * 2)

#define STARTBUTTON_X_PAD 6
#define STARTBUTTON_CELL_POS -3
#define STARTBUTTON_CELL_IDX (STARTBUTTON_CELL_POS * 2)

#define BACKGROUND_FILE "sandbox.jpg"

class CubeScene : public QGraphicsScene
{
	Q_OBJECT
public:
    CubeScene(QObject * parent = 0);
    ~CubeScene();

    void loadImage (const QString &file);

    void initialize (void);

    void startPlay(void);

    void moveAllCell(const QPoint &pos, int off_row, int off_col);

    void moveCell(const QPoint &pos, int row, int col);

    void checkAllCell(void);

    void setBgVisible(bool visible) { fb.setVisible(visible); };

    QSize getSize(void) { return QSize(cube_width, cube_height); }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void keyReleaseEvent(QKeyEvent * event);

    void drawGrid (int row, int col);
    QPoint getCellPos(int row, int col);

    QStringList newEventCmd (int type, int code, int value);
    QPoint scenePosToVirtual(QPointF pos);
    void sendTap(QPoint pos);
    void sendEvent(QPoint pos);
    void sendVirtualClick(QPoint pos);
    void sendVirtualKey(int key);

    void startFBReader();
    void stopFBReader();

public slots:
    void updateScene(QByteArray *bytes);
    void fbDisconnected(void);

private:
    FBCellItem fb;
    QGraphicsRectItem grayMask;
    QGraphicsSimpleTextItem promptItem;
    CubeCellItem *b_items[MAX_ROW_COL_SIZE][MAX_ROW_COL_SIZE];
    CubeCellItem *b_curr_items[MAX_ROW_COL_SIZE][MAX_ROW_COL_SIZE];
    int m_white_row;
    int m_white_col;

    int fb_width;
    int fb_height;
    int pixel_format;
    int cube_width;
    int cube_height;
    int cell_width;
    int cell_height;
    int row_size;
    int col_size;
    int x_pad;
    int y_pad;

    QMutex update_mutex;
    QPixmap pixmap;
    FBReader reader;
};

#endif // CUBESCENE_H
