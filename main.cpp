/*
 * main.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QtGui/QApplication>
#include <QSize>
#include <QPoint>
#include <QIcon>
#include <QGLWidget>
#include <QGraphicsView>

#include "cubescene.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CubeView  view;
    CubeScene scene;
    QSizeF sizef;
    QSize size;

    QObject::connect(&scene, SIGNAL(sceneSizeChanged(QSize)),
			&view, SLOT(cubeSizeChanged(QSize)));

    view.setScene(&scene);

    sizef = scene.itemsBoundingRect().size();
    size = QSize(sizef.width(), sizef.height())
	    + QSize(WINDOW_BORDER, WINDOW_BORDER);

    view.setMinimumSize(size);
    view.resize(size);
    view.show();

    QPixmap icon(":/images/panda-ribbon.jpg");
    view.setWindowIcon(QIcon(icon.scaled(
		    QSize(24, 24),
		    Qt::KeepAspectRatio,
		    Qt::SmoothTransformation)));

    return a.exec();
}

