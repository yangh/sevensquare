/*
 * main.cpp
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

    w.show();

    return a.exec();
}

