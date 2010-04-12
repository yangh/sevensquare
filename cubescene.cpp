#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QDebug>

#include <stdlib.h>
#include <time.h>

#include "cubescene.h"

#define BACKGROUND_FILE "./everest-320x240.jpg"

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

    const char * bg = getenv("SQ_BG");

    initialize (bg == NULL ? BACKGROUND_FILE : bg);
}

void CubeScene::initialize (const char *image_file)
{
    int row, col;
    QPixmap pixmap;

    qDebug() << "Scene initialize.";

    /* TODO: Load default background file if @image_file is null. */
    pixmap.load (image_file, 0);

    setBackgroundBrush(QBrush(pixmap));

    bg_mask = new QGraphicsRectItem(QRectF(0, 0, CUBE_WIDTH, CUBE_HEIGHT));
    bg_mask->setBrush(QBrush(QColor(0, 0, 0, 135)));
    bg_mask->setZValue(1); /* lay in the bottom */

    addItem(bg_mask);

    /* Thumnail of image */
    int tw, th;
    QPixmap nail_bg;
    CubeCellItem *nail_cell;

    tw = CELL_WIDTH - THUMNAIL_X_PAD * 2;
    th = pixmap.height() * tw / pixmap.width();
    nail_bg = pixmap.scaled(QSize(tw, th),
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
    nail_cell = new CubeCellItem(nail_bg);

    int tx, ty;
    tx = CELL_WIDTH * (COL_SIZE - 1) + THUMNAIL_X_PAD + X_PAD;
    ty = CELL_WIDTH * (ROW_SIZE - 2) + (CELL_WIDTH - th) / 2 + Y_PAD;
    //qDebug() << "Draw thumnail at: " << tx << ", " << ty;
    nail_cell->setPos(tx, ty);
    nail_cell->setOriginalCubePos(THUMNAIL_CELL_POS, THUMNAIL_CELL_POS);
    nail_cell->setZValue(5);

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
                    CELL_WIDTH * col + X_PAD,
                    CELL_WIDTH * row + Y_PAD,
                    CELL_WIDTH, CELL_WIDTH));
            item->setBrush(QBrush(QColor(255, 255, 255, 235), Qt::Dense5Pattern));
            item->setPen(grid_pen);
            item->setZValue(3); /* lay in the bottom */

            addItem(item);
        }
    }

    /* Initialize cell with background */

    for (col = 0; col < COL_SIZE - 1; col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            QPixmap cell_bg;
            int x, y;

            qDebug() << "Init cell: " << row << ", " << col;
            x = CELL_WIDTH * col + X_PAD + GRID_WIDTH;
            y = CELL_WIDTH * row + Y_PAD + GRID_WIDTH;
            cell_bg = pixmap.copy(x, y,
                                  CELL_WIDTH - GRID_WIDTH,
                                  CELL_WIDTH - GRID_WIDTH);

            CubeCellItem *item;

            item = new CubeCellItem(cell_bg);
            item->setPos(x, y);
            item->setZValue(5); /* lay in the top*/
            addItem(item);

            item->setOriginalCubePos(row, col);
            b_items[row][col] = item;
        }
    }

    m_white_col = COL_SIZE - 1;
    m_white_row = ROW_SIZE - 1;

#if 0
    /* White item */
    CubeCellItem *white_cell;
    QPixmap bg(CELL_WIDTH - GRID_WIDTH,
               CELL_WIDTH - GRID_WIDTH);

    row = ROW_SIZE - 1;
    col = COL_SIZE - 1;

    bg.fill(Qt::white);
    white_cell = new CubeCellItem(bg);
    white_cell->setPos(CELL_WIDTH * col + X_PAD + GRID_WIDTH,
                       CELL_WIDTH * row + Y_PAD + GRID_WIDTH);
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

    setBgVisible(TRUE);

    /* Reorder all items */
    for (col = 0; col < (COL_SIZE - 1); col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            b_curr_items[row][col] = b_items[row][col];
            b_curr_items[row][col]->setCubePos(row, col);
        }
    }

    /* Move cell in the right-bottom into empty cell */
    int lr = ROW_SIZE - 1;
    int lc = COL_SIZE - 2;

    moveCell (b_items[lr][lc]->cubePos(), lr, lc + 1); 

    MASK_CURRENT_MAP[lr][lc] = 1;

    m_white_row = lr;
    m_white_col = lc;

    /* Radomize cubes */
    time_t t = time(NULL);
    srand (t);

    int n = 0;
    for (col = 0; col < (COL_SIZE - 1); col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            int nx, ny;
            int nrand;
            int r, c;

            /* Only randmoize first 11 cells */
            if (col == (COL_SIZE - 2) && row == (ROW_SIZE - 1) ) {
                break;
            }

            nrand = rand() % ((COL_SIZE - 1) * ROW_SIZE);
            r = nrand % ROW_SIZE;
            c = nrand % (COL_SIZE - 2);

            /* Find unused cell by mask */
            while (MASK_CURRENT_MAP[r][c] != 0) {
                c ++;
                if (c > (COL_SIZE - 2)) {
                    c = 0;
                    r++;
                    if (r > (ROW_SIZE - 1)) {
                        r = 0;
                    }
                }
            }

            MASK_CURRENT_MAP[r][c] = 1;
            nx = CELL_WIDTH * c + X_PAD + GRID_WIDTH;
            ny = CELL_WIDTH * r + Y_PAD + GRID_WIDTH;
            b_items[row][col]->setCubePos(r, c);
            b_items[row][col]->setPos(nx, ny);
            b_curr_items[r][c] = b_items[row][col];

            //qDebug() << "Move item[" << row << "][" << col << "] to: "
            //        << nx << ", " << ny << ". " << n++;
        }
    }

    mapMaskRest(MASK_CURRENT_MAP);

    m_white_row = ROW_SIZE - 1;
    m_white_col = COL_SIZE - 2;
}

void CubeScene::moveAllCell(const QPoint &pos, int off_row, int off_col)
{
    qDebug() << "Move cells beside the white cell.";

    int i, off;
    int nx, ny;
    CubeCellItem *cell = 0;

    /* Row move */

    off = off_row > 0 ? -1 : 1;
    for (i = 0; i < abs(off_row); i++ ) {
        nx = CELL_WIDTH * m_white_col + X_PAD + GRID_WIDTH;
        ny = CELL_WIDTH * m_white_row + Y_PAD + GRID_WIDTH;

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
        nx = CELL_WIDTH * m_white_col + X_PAD + GRID_WIDTH;
        ny = CELL_WIDTH * m_white_row + Y_PAD + GRID_WIDTH;

        cell = b_curr_items[m_white_row][m_white_col + off];
        cell->setPos(nx, ny);
        cell->setCubePos(m_white_row, m_white_col);

        b_curr_items[m_white_row][m_white_col] = cell;
        b_curr_items[m_white_row][m_white_col + off] = 0;
        m_white_col += off;
    }
}

void CubeScene::moveCell(const QPoint &pos, int row, int col)
{
    qDebug() << "Move cell " << pos << " to: " << row << ", " << col;

    int r, c;
    int nx, ny;
    CubeCellItem *cell = 0;

    r = pos.x();
    c = pos.y();
    cell = b_curr_items[r][c];
    if (! cell) {
        return;
    }

    /* Cell move */
    nx = CELL_WIDTH * col + X_PAD + GRID_WIDTH;
    ny = CELL_WIDTH * row + Y_PAD + GRID_WIDTH;

    cell->setPos(nx, ny);
    cell->setCubePos(row, col);

    b_curr_items[row][col] = cell;
    b_curr_items[r][c] = 0;

    m_white_row = r;
    m_white_col = c;
}

void CubeScene::checkAllCell(void)
{
    int row, col;
    int n = 0;

    /* Check all items */
    for (col = 0; col < (COL_SIZE - 1); col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            /* Only randmoize first 11 cells */
            if (col == (COL_SIZE - 2) && row == (ROW_SIZE - 1) ) {
                break;
            }

            CubeCellItem *cell = b_curr_items[row][col];
            if (cell && cell->cubePos() == cell->originalCubePos()) {
                n++;
            }
        }
    }

    qDebug() << "Finished " << n << " cells";

    if (n == 11) {
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

void CubeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
     QGraphicsItem *item = 0;
     CubeCellItem *cell = 0;

     //qDebug() << "Scene clicked: " << event->scenePos();

     item = itemAt(event->scenePos());
     cell =  dynamic_cast<CubeCellItem *>(item);
     if (!cell) {
         setBgVisible (!getBgVisible()); /* color egg */
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

         checkAllCell();
     }
 }

