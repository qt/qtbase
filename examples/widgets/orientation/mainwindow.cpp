#include "mainwindow.h"
#include "ui_landscape.h"
#include "ui_portrait.h"

#include <QDesktopWidget>
#include <QResizeEvent>

//! [0]
MainWindow::MainWindow(QWidget *parent) :
        QWidget(parent),
        landscapeWidget(0),
        portraitWidget(0)
{
    landscapeWidget = new QWidget(this);
    landscape.setupUi(landscapeWidget);

    portraitWidget = new QWidget(this);
    portrait.setupUi(portraitWidget);
//! [0]

//! [1]
    connect(portrait.exitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(landscape.exitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(landscape.buttonGroup, SIGNAL(buttonClicked(QAbstractButton*)),
            this, SLOT(onRadioButtonClicked(QAbstractButton*)));

    landscape.radioAButton->setChecked(true);
    onRadioButtonClicked(landscape.radioAButton);
//! [1]

//! [2]
#ifdef Q_WS_MAEMO_5
    setAttribute(Qt::WA_Maemo5AutoOrientation, true);
#endif
}
//! [2]

//! [3]
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QSize size = event->size();
    bool isLandscape = size.width() > size.height();

    if (!isLandscape)
        size.transpose();

    landscapeWidget->setFixedSize(size);
    size.transpose();
    portraitWidget->setFixedSize(size);

    landscapeWidget->setVisible(isLandscape);
    portraitWidget->setVisible(!isLandscape);
}
//! [3]

//! [4]
void MainWindow::onRadioButtonClicked(QAbstractButton *button)
{
    QString styleTemplate = "background-image: url(:/image_%1.png);"
                            "background-repeat: no-repeat;"
                            "background-position: center center";

    QString imageStyle("");
    if (button == landscape.radioAButton)
        imageStyle = styleTemplate.arg("a");
    else if (button == landscape.radioBButton)
        imageStyle = styleTemplate.arg("b");
    else if (button == landscape.radioCButton)
        imageStyle = styleTemplate.arg("c");

    portrait.choiceWidget->setStyleSheet(imageStyle);
    landscape.choiceWidget->setStyleSheet(imageStyle);
}
//! [4]

