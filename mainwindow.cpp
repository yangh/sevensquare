#include <QGraphicsRectItem>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QDebug>

#include <stdlib.h>
#include <time.h>

#include "cubecellitem.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupBoardView();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::setupBoardView(void)
{
    b_view = ui->graphicsView;
    b_scene = new QGraphicsScene(this);

    int row, col;
    QPixmap pixmap;

#define BACKGROUND_FILE "C:\\cygwin\\home\\gucuie\\tetris\\20090728-google_aol_080808_mn.jpg"

    /* TODO: Load background file from othere place. */
    pixmap.load (BACKGROUND_FILE, 0);
    b_bg = b_scene->addPixmap(pixmap);

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
            //item->setBrush(QBrush(QColor(0, 0, 240)));
            item->setPen(grid_pen);

            b_scene->addItem(item);
            //b_items[j][i] = item;
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
                                  CUBE_WIDTH - GRID_WIDTH * 2,
                                  CUBE_WIDTH - GRID_WIDTH * 2);

            CubeCellItem *item;

            item = new CubeCellItem(cell_bg);
            item->setPos(x, y);
            item->setOriginalCubePos(row, col);

            b_scene->addItem(item);
            b_items[row][col] = item;
        }
    }

    /* White item */
    CubeCellItem *white_cell;
    QPixmap bg(CUBE_WIDTH - GRID_WIDTH * 2,
               CUBE_WIDTH - GRID_WIDTH * 2);

    row = ROW_SIZE - 1;
    col = COL_SIZE - 1;

    bg.fill(Qt::white);
    white_cell = new CubeCellItem(bg);
    white_cell->setPos(CUBE_WIDTH * col + X_PAD + GRID_WIDTH,
                       CUBE_WIDTH * row + Y_PAD + GRID_WIDTH);
    white_cell->setOriginalCubePos(row, col);

    b_scene->addItem(white_cell);
    b_items[row][col] = white_cell;

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
            b_items[row][col]->setOriginalCubePos(r, c);
            b_items[row][col]->setPos(nx, ny);

            //qDebug() << "Move item[" << row << "][" << col << "] to: "
            //        << nx << ", " << ny;
        }
    }

    mapMaskRest(MASK_CURRENT_MAP);

    b_view->setScene(b_scene);
}
