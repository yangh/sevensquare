#include <QSize>
#include <QPoint>

#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    b_view = new QGraphicsView(this);
    b_scene = new CubeScene(this);

    QSize b_size = b_scene->getSize();
    QSize size(b_size.width() + 4, b_size.height() + 4);

    resize (size);
    setMinimumSize(size);
    //setMaximumSize(size);

    setCentralWidget (b_view);

    b_view->setScene(b_scene);
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

