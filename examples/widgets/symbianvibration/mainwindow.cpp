#include <QtGui/QMenuBar>
#include "mainwindow.h"
#include "vibrationsurface.h"
#include "XQVibra.h"

//! [0]
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    vibra = new XQVibra(this);
    setCentralWidget(new VibrationSurface(vibra, this));
    menuBar()->addAction(tr("Vibrate"), this, SLOT(vibrate()));
}
//! [0]

//! [1]
void MainWindow::vibrate()
{
    vibra->setIntensity(75);
    vibra->start(2500);
}
//! [1]

