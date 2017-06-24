/*
 * main.cpp
 *
 * Copyright 2012-2013 Yang Hong
 *
 */

#include <QApplication>
#include <QSize>
#include <QIcon>

#include "cubescene.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CubeView  view;

    QPixmap icon(":/images/panda-ribbon.jpg");
    view.setWindowIcon(QIcon(icon.scaled(
                                 QSize(24, 24),
                                 Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation)));
    view.setWindowTitle(QObject::tr("Seven Square"));
    view.show();

    return a.exec();
}

