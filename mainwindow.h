#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include "cubecellitem.h"

#define ROW_SIZE 3
#define COL_SIZE 5
#define CUBE_WIDTH 60
#define X_PAD ((320 - 60 * COL_SIZE)/2)
#define Y_PAD ((240 - 60 * ROW_SIZE)/2)

#define GRID_COLOR 120, 120, 120
#define GRID_WIDTH 1
#define WHITE_CELL_COLOR "White"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;

    void setupBoardView(void);

    QGraphicsView   *b_view;
    QGraphicsScene  *b_scene;
    CubeCellItem    *b_items[ROW_SIZE][COL_SIZE];
    QGraphicsItem   *b_bg;
};

#endif // MAINWINDOW_H
