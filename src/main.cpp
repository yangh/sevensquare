/*
 * main.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#include <QApplication>
#include <QSize>
#include <QIcon>
#include <QString>

#include "cubescene.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
	QString fullopt = "-f";
	int fullscreen = 0;
    CubeView  view;

	if (argc > 1 && fullopt == argv[1])
		fullscreen = 1;

    QPixmap icon(":/images/panda-ribbon.jpg");
    view.setWindowIcon(QIcon(icon.scaled(
                                 QSize(24, 24),
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation)));
    view.setWindowTitle(QObject::tr("Seven Square"));

	if (fullscreen)
    	view.showFullScreen();
	else
    	view.show();

    return a.exec();
}

