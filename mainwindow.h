/*
 * mainwindow.h
 *
 * Copyright 2012-2012 Yang Hong
 *
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>

#include "cubescene.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    QGraphicsView   *view;
    CubeScene       *scene;
};

#endif // MAINWINDOW_H
