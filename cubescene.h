#ifndef CUBESCENE_H
#define CUBESCENE_H

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

#include "cubecellitem.h"

#define ROW_SIZE 3
#define COL_SIZE 5
#define CUBE_WIDTH 60
#define X_PAD ((320 - 60 * COL_SIZE)/2)
#define Y_PAD ((240 - 60 * ROW_SIZE)/2)

#define GRID_COLOR 120, 120, 120
#define GRID_WIDTH 1

#define WHITE_CELL_COLOR "White"
#define WHITE_CELL_POS -1
#define WHITE_CELL_IDX (WHITE_CELL_POS * 2)

#define THUMNAIL_X_PAD 6
#define THUMNAIL_CELL_POS -2
#define THUMNAIL_CELL_IDX (THUMNAIL_CELL_POS * 2)

class CubeScene : public QGraphicsScene
{
public:
    CubeScene(QObject * parent = 0);

    void initialize (const char *image_file = 0);

    void startPlay(void);

    void moveAllCell(const QPoint &pos, int off_row, int off_col);

    void moveCell(const QPoint &pos, int row, int col);

    void checkAllCell(void);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    CubeCellItem    *b_items[ROW_SIZE][COL_SIZE];
    CubeCellItem    *b_curr_items[ROW_SIZE][COL_SIZE];
    int m_white_row;
    int m_white_col;
};

#endif // CUBESCENE_H
