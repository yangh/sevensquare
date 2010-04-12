#include <QPoint>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    b_view = new QGraphicsView(this);
    b_scene = new CubeScene(this);

    resize (CUBE_WIDTH + 2, CUBE_HEIGHT + 2);
    setCentralWidget (b_view);

    b_view->setScene(b_scene);

    /* TODO: start some else where */
    b_scene->startPlay();
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        //ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

