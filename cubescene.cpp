#include <QGraphicsRectItem>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QPixmap>
#include <QStringList>
#include <QDebug>

#include <stdlib.h>
#include <time.h>

#include "cubescene.h"

CubeScene::CubeScene(QObject * parent) :
        QGraphicsScene(parent)
{
    cell_width = DEFAULT_CELL_WIDTH;
}

void CubeScene::loadImage (const QString &file)
{
    qDebug() << "Load image from: " << file;

    if (pixmap.load (file, 0)) {
        initialize();
    }
}

void CubeScene::initialize (void)
{
    int row, col;

    qDebug() << "Scene initialize.";

    cube_width = pixmap.width();
    cube_height = pixmap.height();
    row_size = (cube_height - MIN_PAD) / cell_width;
    col_size = (cube_width - MIN_PAD) / cell_width;

    row_size = (row_size > MAX_ROW_COL_SIZE) ? MAX_ROW_COL_SIZE : row_size;
    col_size = (col_size > MAX_ROW_COL_SIZE) ? MAX_ROW_COL_SIZE : col_size;

    x_pad = (cube_width - cell_width * col_size) / 2;
    y_pad = (cube_height - cell_width * row_size) / 2;

    setBackgroundBrush(QBrush(pixmap));

    bg_mask = new QGraphicsRectItem(QRectF(0, 0, cube_width, cube_height));
    bg_mask->setBrush(QBrush(QColor(0, 0, 0, 135)));
    bg_mask->setPen(Qt::NoPen);
    bg_mask->setZValue(0); /* lay in the bottom */

    addItem(bg_mask);

    /* Thumnail of image */
    int tw, th;
    QPixmap nail_bg;
    CubeCellItem *nail_cell;

    tw = cell_width - THUMNAIL_X_PAD * 2;
    th = pixmap.height() * tw / pixmap.width();
    nail_bg = pixmap.scaled(QSize(tw, th),
                            Qt::IgnoreAspectRatio,
                            Qt::SmoothTransformation);
    nail_cell = new CubeCellItem(nail_bg);

    int tx, ty;
    tx = cell_width * (col_size - 1) + THUMNAIL_X_PAD + x_pad;
    ty = cell_width * (row_size - 2) + (cell_width - th) / 2 + y_pad;
    //qDebug() << "Draw thumnail at: " << tx << ", " << ty;
    nail_cell->setPos(tx, ty);
    nail_cell->setOriginalCubePos(THUMNAIL_CELL_POS, THUMNAIL_CELL_POS);
    nail_cell->setZValue(5);

    addItem(nail_cell);

    /* Start button */
    CubeCellItem *start_cell;
    QPixmap start_icon(16, 16);
    static const QPointF points[3] = {
        QPointF(4.0, 0.0),
        QPointF(16.0, 8.0),
        QPointF(4.0, 16.0)
    };

    start_icon.fill(Qt::transparent);

    QPainter painter(&start_icon);
    QPen pen;

    pen.setColor(QColor(Qt::gray));
    pen.setWidth(1);

    painter.setPen(pen);
    painter.setBrush(QBrush(QColor(Qt::black)));
    painter.drawPolygon(points, 3, Qt::WindingFill);
    start_cell = new CubeCellItem(start_icon);
    tx = cell_width * (col_size - 1) + (cell_width - 16) / 2 + x_pad;
    ty = cell_width * (row_size - 3) + (cell_width - 16) / 2 + y_pad;
    qDebug() << "Draw start buttonat: " << tx << ", " << ty;
    start_cell->setPos(tx, ty);
    start_cell->setOriginalCubePos(STARTBUTTON_CELL_POS, STARTBUTTON_CELL_POS);
    start_cell->setZValue(5);

    addItem(start_cell);

    /* Draw grid */

    QPen grid_pen;
    grid_pen.setColor(QColor(GRID_COLOR));
    grid_pen.setWidth(GRID_WIDTH);

    for (col = 0; col < col_size - 1; col++) {
        for (row = 0; row < row_size; row++) {
            QGraphicsRectItem *item;

            item = new QGraphicsRectItem(QRectF(
                    cell_width * col + x_pad,
                    cell_width * row + y_pad,
                    cell_width, cell_width));
            item->setBrush(QBrush(QColor(255, 255, 255, 235), Qt::Dense6Pattern));
            item->setPen(grid_pen);
            item->setZValue(3); /* lay in the bottom */

            addItem(item);
        }
    }

    QGraphicsRectItem *item;

    /* Grid in right-bottom */
    row = row_size - 1;
    col = col_size - 1;
    item = new QGraphicsRectItem(QRectF(
            cell_width * col + x_pad,
            cell_width * row + y_pad,
            cell_width, cell_width));
    item->setBrush(QBrush(QColor(255, 255, 255, 235), Qt::Dense6Pattern));
    item->setPen(grid_pen);
    item->setZValue(3); /* lay in the bottom */

    addItem(item);

    /* Initialize cell with background */

    for (col = 0; col < col_size - 1; col++) {
        for (row = 0; row < row_size; row++) {
            QPixmap cell_bg;
            int x, y;

            qDebug() << "Init cell: " << row << ", " << col;
            x = cell_width * col + x_pad + GRID_WIDTH;
            y = cell_width * row + y_pad + GRID_WIDTH;
            cell_bg = pixmap.copy(x, y,
                                  cell_width - GRID_WIDTH,
                                  cell_width - GRID_WIDTH);

            CubeCellItem *item;

            item = new CubeCellItem(cell_bg);
            item->setPos(x, y);
            item->setZValue(5); /* lay in the top*/
            addItem(item);

            item->setOriginalCubePos(row, col);
            b_items[row][col] = item;
        }
    }

    m_white_col = col_size - 1;
    m_white_row = row_size - 1;
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
            int nx, ny;
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
            nx = cell_width * c + x_pad + GRID_WIDTH;
            ny = cell_width * r + y_pad + GRID_WIDTH;
            b_items[row][col]->setCubePos(r, c);
            b_items[row][col]->setPos(nx, ny);
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
    int nx, ny;
    CubeCellItem *cell = 0;

    qDebug() << "Move cells beside the white cell.";

    /* Row move */
    off = off_row > 0 ? -1 : 1;
    for (i = 0; i < abs(off_row); i++ ) {
        nx = cell_width * m_white_col + x_pad + GRID_WIDTH;
        ny = cell_width * m_white_row + y_pad + GRID_WIDTH;

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
        nx = cell_width * m_white_col + x_pad + GRID_WIDTH;
        ny = cell_width * m_white_row + y_pad + GRID_WIDTH;

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
    int r, c;
    int nx, ny;
    CubeCellItem *cell = 0;

    qDebug() << "Move cell " << pos << " to: " << row << ", " << col;

    r = pos.x();
    c = pos.y();
    cell = b_curr_items[r][c];

    if (! cell) {
        return;
    }

    /* Cell move */
    nx = cell_width * col + x_pad + GRID_WIDTH;
    ny = cell_width * row + y_pad + GRID_WIDTH;

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
    for (col = 0; col < (col_size - 1); col++) {
        for (row = 0; row < row_size; row++) {
            /* Only count first 11 cells */
            if (col == (col_size - 2) && row == (row_size - 1) ) {
                break;
            }

            CubeCellItem *cell = b_curr_items[row][col];
            if (cell && cell->cubePos() == cell->originalCubePos()) {
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
}

