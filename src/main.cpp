/*
 * main.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#include <QtGui/QApplication>
#include <QSize>
#include <QIcon>

#include "cubescene.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CubeView  view;
    CubeScene scene;
    QSize size;

    QObject::connect(&scene, SIGNAL(sceneSizeChanged(QSize)),
                     &view, SLOT(cubeSizeChanged(QSize)));
    QObject::connect(&view, SIGNAL(viewSizeChanged(QSize)),
                     &scene, SLOT(cubeResize(QSize)));

    size = (scene.itemsBoundingRect().size()
            + QSize(WINDOW_BORDER, WINDOW_BORDER)).toSize();

    view.setScene(&scene);
    view.setMinimumSize(size);
    view.resize(size);
    view.show();

    QPixmap icon(":/images/panda-ribbon.jpg");
    view.setWindowIcon(QIcon(icon.scaled(
                                 QSize(24, 24),
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation)));
    view.setWindowTitle(QObject::tr("Seven Square"));

    return a.exec();
}

