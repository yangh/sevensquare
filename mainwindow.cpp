#include <QSize>
#include <QPoint>
#include <QtGui/QApplication>

#include "mainwindow.h"

#define WINDOW_BORDER 4

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    b_view = new QGraphicsView(this);
    b_scene = new CubeScene(this);

    QStringList argv = qApp->arguments();
    QString file;

    if (argv.length() > 1) {
        file = argv[1];
    } else {
        const char * bg = getenv("SQ_BG");

        file = (bg == NULL) ? BACKGROUND_FILE : bg;
    }

    b_scene->loadImage(file);

    QSize b_size = b_scene->getSize();
    QSize size(b_size.width() + 2,
		    b_size.height() + 2);
    qDebug() << "Mini windows size" << size;

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

