/*
 * mainwindow.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QSize>
#include <QPoint>
#include <QtGui/QApplication>

#include "mainwindow.h"

#define WINDOW_BORDER 3

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QStringList argv = qApp->arguments();
    QString file;
    const char *bg = getenv("SQ_BG");

    if (argv.length() > 1) {
        file = argv[1];
    } else if (bg != NULL) {
        file = bg;
    } else {
        file = BACKGROUND_FILE;
    }

    bg_file = file;

    setupScene();
}

void MainWindow::setupScene(void)
{
    view = new QGraphicsView(this);
    scene = new CubeScene(this);

    scene->loadImage(bg_file);

    QSize size = scene->getSize();
    size += QSize(WINDOW_BORDER, WINDOW_BORDER);
    qDebug() << "Mini windows size" << size;

    resize(size);
    setMinimumSize(size);
    //setMaximumSize(size);

    view->setScene(scene);
    view->show();
    setCentralWidget (view);

    QObject::connect(scene, SIGNAL(sceneSizeChanged(QSize)),
			this, SLOT(cubeSizeChanged(QSize)));
}

void MainWindow::cubeSizeChanged(QSize size)
{
#if 0
    setupScene();
#else

    QSize wins(size);

    wins += QSize(WINDOW_BORDER, WINDOW_BORDER);

    qDebug() << "Resize main window" << size;
    setMinimumSize(wins);
    view->resize(size);
    resize(wins);

    //setCentralWidget (view);
#endif
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

