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
#include <QSize>

#include "cubescene.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void cubeSizeChanged(QSize);

protected:
    void changeEvent(QEvent *e);
    void setupScene(void);

private:
    QGraphicsView   *view;
    CubeScene       *scene;
    QString          bg_file;
};

#endif // MAINWINDOW_H
