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
            item->setPen(grid_pen);

            addItem(item);
        }
    }

    /* Initialize cell with background */

    for (col = 0; col < COL_SIZE; col++) {
        for (row = 0; row < ROW_SIZE; row++) {
            if (! MASK_MAP[row][col]) {
                continue;
            }

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

            //qDebug() << "Move item[" << row << "][" << col << "] to: "
            //        << nx << ", " << ny;
        }
    }

    mapMaskRest(MASK_CURRENT_MAP);
}

void CubeScene::cellClicked(CubeCellItem *item)
{
    qDebug() << "Cell clicked.";
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
     QGraphicsItem *item = itemAt(event->scenePos());
     CubeCellItem *cell = dynamic_cast<CubeCellItem *>(item);

     qDebug() << "Scene clicked: " << event->scenePos();

     if (cell) {
         qDebug() << "Item clicked, curr pos: " << cell->cubePos()
                 << ", orig pos: " << cell->originalCubePos();
     }
}

