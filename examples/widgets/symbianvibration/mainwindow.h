#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui/QMainWindow>
class XQVibra;

//! [0]
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

private slots:
    void vibrate();

private:
    XQVibra *vibra;
};
//! [0]

#endif // MAINWINDOW_H
