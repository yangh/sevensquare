#include <QSize>
#include <QPoint>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    b_view = new QGraphicsView(this);
    b_scene = new CubeScene(this);

    /* FIXME: Fixed window size, maybe no suite for individual device. */
    QSize size(CUBE_WIDTH + 4, CUBE_HEIGHT + 4);
    resize (size);
    setMinimumSize(size);
    setMaximumSize(size);

    setCentralWidget (b_view);

    b_view->setScene(b_scene);

    /* TODO: start some else where */
    //b_scene->startPlay();
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

