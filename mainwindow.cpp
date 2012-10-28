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
    view = new QGraphicsView(this);
    scene = new CubeScene(this);

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

    scene->loadImage(file);

    QSize size = scene->getSize();
    size += QSize(WINDOW_BORDER, WINDOW_BORDER);
    qDebug() << "Mini windows size" << size;

    resize(size);
    setMinimumSize(size);
    //setMaximumSize(size);

    view->setScene(scene);
    setCentralWidget (view);
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

