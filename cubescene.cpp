#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QDebug>

#include <stdlib.h>
#include <time.h>

#include "cubescene.h"

#define BACKGROUND_FILE "C:\\cygwin\\home\\gucuie\\tetris\\20090728-google_aol_080808_mn.jpg"

static const short int MASK_MAP[ROW_SIZE][COL_SIZE] = {
    { 1, 1, 1, 1, 0 },
    { 1, 1, 1, 1, 0 },
    { 1, 1, 1, 1, 1 },
};

static short int MASK_CURRENT_MAP[ROW_SIZE][COL_SIZE] = {
    { 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },
};

static void mapMaskRest (short int mask[ROW_SIZE][COL_SIZE])
{
    int row, col;

    for (col = 0; col < COL_SIZE; col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            mask[row][col] = 0;
        }
    }
}

CubeScene::CubeScene(QObject * parent) :
        QGraphicsScene(parent)
{
    int row, col;

    for (col = 0; col < COL_SIZE; col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            b_items[row][col] = 0;
            b_curr_items[row][col] = 0;
        }
    }

    initialize(BACKGROUND_FILE);
}

void CubeScene::initialize (const char *image_file)
{
    int row, col;
    QPixmap pixmap;

    qDebug() << "Scene initialize.";

    /* TODO: Load default background file if @image_file is null. */

    pixmap.load (image_file, 0);
    setBackgroundBrush(QBrush(pixmap));

    /* Thumnail of image */
    int tw, th;
    QPixmap nail_bg;
    CubeCellItem *nail_cell;

    tw = CUBE_WIDTH - THUMNAIL_X_PAD * 2;
    th = pixmap.height() * tw / pixmap.width();
    nail_bg = pixmap.scaled(QSize(tw, th),
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
    nail_cell = new CubeCellItem(nail_bg);

    int tx, ty;
    tx = CUBE_WIDTH * (COL_SIZE - 1) + THUMNAIL_X_PAD + X_PAD;
    ty = CUBE_WIDTH * (ROW_SIZE - 2) + (CUBE_WIDTH - th) / 2 + Y_PAD;
    //qDebug() << "Draw thumnail at: " << tx << ", " << ty;
    nail_cell->setPos(tx, ty);
    nail_cell->setOriginalCubePos(THUMNAIL_CELL_POS, THUMNAIL_CELL_POS);

    addItem(nail_cell);

    /* Draw grid */

    QPen grid_pen;
    grid_pen.setColor(QColor(GRID_COLOR));
    grid_pen.setWidth(GRID_WIDTH);

    for (col = 0; col < COL_SIZE; col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            if (! MASK_MAP[row][col]) {
                continue;
            }

            QGraphicsRectItem *item;

            item = new QGraphicsRectItem(QRectF(
                    CUBE_WIDTH * col + X_PAD,
                    CUBE_WIDTH * row + Y_PAD,
                    CUBE_WIDTH, CUBE_WIDTH));
            item->setBrush(QBrush(QColor(255, 255, 255, 235)));
            item->setPen(grid_pen);

            addItem(item);
        }
    }

    /* Initialize cell with background */

    for (col = 0; col < COL_SIZE - 1; col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            QPixmap cell_bg;
            int x, y;

            x = CUBE_WIDTH * col + X_PAD + GRID_WIDTH;
            y = CUBE_WIDTH * row + Y_PAD + GRID_WIDTH;
            cell_bg = pixmap.copy(x, y,
                                  CUBE_WIDTH - GRID_WIDTH,
                                  CUBE_WIDTH - GRID_WIDTH);

            CubeCellItem *item;

            item = new CubeCellItem(cell_bg);
            item->setPos(x, y);
            item->setOriginalCubePos(row, col);

            addItem(item);
            b_items[row][col] = item;
        }
    }

#if 0
    /* White item */
    CubeCellItem *white_cell;
    QPixmap bg(CUBE_WIDTH - GRID_WIDTH,
               CUBE_WIDTH - GRID_WIDTH);

    row = ROW_SIZE - 1;
    col = COL_SIZE - 1;

    bg.fill(Qt::white);
    white_cell = new CubeCellItem(bg);
    white_cell->setPos(CUBE_WIDTH * col + X_PAD + GRID_WIDTH,
                       CUBE_WIDTH * row + Y_PAD + GRID_WIDTH);
    white_cell->setOriginalCubePos(WHITE_CELL_POS, WHITE_CELL_POS);
    white_cell->setCubePos(row, col);

    addItem(white_cell);
    b_items[row][col] = white_cell;
#endif
}

void CubeScene::startPlay(void)
{
    int row, col;

    qDebug() << "Start play.";

    /* Radomize cubes */
    time_t t = time(NULL);
    srand (t);

    for (col = 0; col < (COL_SIZE - 1); col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            int nx, ny;
            int nrand;
            int r, c, n;
            bool found = TRUE;

            nrand = rand() % ((COL_SIZE - 1) * ROW_SIZE);
            r = nrand % ROW_SIZE;
            c = nrand % (COL_SIZE - 1);
            n = 0;

            //qDebug() << "Rand r/c: " <<  r << ", " << c;

            while (MASK_CURRENT_MAP[r][c] != 0) {
                c ++;
                if (c > (COL_SIZE - 2)) {
                    c = 0;
                    r++;
                    if (r > (ROW_SIZE - 1)) {
                        r = 0;
                    }
                }
                n ++;
                if (n > (ROW_SIZE * (COL_SIZE - 1)) * 2) {
                    qDebug() << "Found too much times, stop now.";
                    found = FALSE;
                    break;
                }
            }

            //qDebug() << "Rand r/c after find: " <<  r << ", " << c;

            MASK_CURRENT_MAP[r][c] = 1;
            nx = CUBE_WIDTH * c + X_PAD + GRID_WIDTH;
            ny = CUBE_WIDTH * r + Y_PAD + GRID_WIDTH;
            b_items[row][col]->setCubePos(r, c);
            b_items[row][col]->setPos(nx, ny);
            b_curr_items[r][c] = b_items[row][col];

            //qDebug() << "Move item[" << row << "][" << col << "] to: "
            //        << nx << ", " << ny;
        }
    }

    mapMaskRest(MASK_CURRENT_MAP);

    m_white_col = COL_SIZE - 1;
    m_white_row = ROW_SIZE - 1;

    /* Make cube resolve able */
    moveCell (b_items[ROW_SIZE - 1][COL_SIZE-2]->cubePos(),
              m_white_row, m_white_col);
    moveCell (b_curr_items[ROW_SIZE - 1][COL_SIZE-2]->cubePos(),
              m_white_row, m_white_col);
}

void CubeScene::moveAllCell(const QPoint &pos, int off_row, int off_col)
{
    qDebug() << "Move cellse which clicked.";

    int i;
    int nx, ny;
    CubeCellItem *cell;
    int off;

    /* Row move */

    off = off_row > 0 ? -1 : 1;
    for (i = 0; i < abs(off_row); i++ ) {
        qDebug() << "White cell move left 1 step." <<
                m_white_row << ", " << m_white_col;
        nx = CUBE_WIDTH * m_white_col + X_PAD + GRID_WIDTH;
        ny = CUBE_WIDTH * m_white_row + Y_PAD + GRID_WIDTH;

        cell = b_curr_items[m_white_row + off][m_white_col];
        cell->setPos(nx, ny);
        cell->setCubePos(m_white_row, m_white_col);

        b_curr_items[m_white_row][m_white_col] = cell;
        b_curr_items[m_white_row + off][m_white_col] = 0;
        m_white_row += off;
    }

    /* Col move */
    off = off_col > 0 ? -1 : 1;
    for (i = 0; i < abs(off_col); i++ ) {
        qDebug() << "White cell move left 1 step." <<
                m_white_row << ", " << m_white_col;
        nx = CUBE_WIDTH * m_white_col + X_PAD + GRID_WIDTH;
        ny = CUBE_WIDTH * m_white_row + Y_PAD + GRID_WIDTH;

        cell = b_curr_items[m_white_row][m_white_col + off];
        cell->setPos(nx, ny);
        cell->setCubePos(m_white_row, m_white_col);

        b_curr_items[m_white_row][m_white_col] = cell;
        b_curr_items[m_white_row][m_white_col + off] = 0;
        m_white_col += off;
    }
    //b_curr_items[row][col]->setPos(nx, ny);
}

void CubeScene::moveCell(const QPoint &pos, int row, int col)
{
    qDebug() << "Move cell " << pos << " to: " << row << ", " << col;

    int nx, ny;
    CubeCellItem *cell;

    /* Cell move */
    nx = CUBE_WIDTH * col + X_PAD + GRID_WIDTH;
    ny = CUBE_WIDTH * row + Y_PAD + GRID_WIDTH;

    cell = b_curr_items[pos.x()][pos.y()];
    cell->setPos(nx, ny);
    cell->setCubePos(row, col);

    b_curr_items[row][col] = cell;
    b_curr_items[pos.x()][pos.y()] = 0;

    m_white_row = pos.x();
    m_white_col = pos.y();
}

void CubeScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item pressed: " << curr_pos;
}

void CubeScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    //qDebug() << "Item moveded: " << curr_pos;
}

void CubeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
     QGraphicsItem *item = 0;
     CubeCellItem *cell = 0;

     //qDebug() << "Scene clicked: " << event->scenePos();

     item = itemAt(event->scenePos());
     cell =  dynamic_cast<CubeCellItem *>(item);
     if (!cell) {
         return;
     }

     qDebug() << "Item clicked, curr pos: " << cell->cubePos()
             << ", orig pos: " << cell->originalCubePos();

     int off_row, off_col;
     QPoint pos;

     pos = cell->originalCubePos();

     /* Check command cell */
     if (THUMNAIL_CELL_IDX == (pos.x() + pos.y())) {
         startPlay();
         return;
     }

     /* Check cell move */
     pos = cell->cubePos();
     off_row = m_white_row - pos.x();
     off_col = m_white_col - pos.y();

     if (off_row == 0 || off_col == 0) {
         qDebug() << "Clicked move able cell: " << cell->cubePos()
                 << ", Offset: " << off_row << ", " << off_col;

         moveAllCell (pos, off_row, off_col);
     }
 }

