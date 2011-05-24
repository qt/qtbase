#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

#include "ui_landscape.h"
#include "ui_portrait.h"

class QAbstractButton;

//! [0]
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

protected:
    void resizeEvent(QResizeEvent *event);

private slots:
    void onRadioButtonClicked(QAbstractButton *button);

private:
    Ui::LandscapeUI landscape;
    Ui::PortraitUI portrait;
    QWidget *landscapeWidget;
    QWidget *portraitWidget;
};
//! [0]

#endif // MAINWINDOW_H
