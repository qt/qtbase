// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Rick Stockton <rickstockton@reno-computerhelp.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "buttontester.h"

#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>

int main(int argv, char **args)
{
    QApplication app(argv, args);

    ButtonTester *testArea = new ButtonTester;
    testArea->setMinimumSize(500, 350);

//  For this test, Qt::RightButton behaves like any other button.
    testArea->setContextMenuPolicy(Qt::NoContextMenu);
    testArea->setTextInteractionFlags(Qt::TextSelectableByMouse);
    testArea->setText("To test your mouse with Qt, press buttons in this area.\nYou may also scroll or tilt your mouse wheel.");
    QPushButton *quitButton = new QPushButton("Quit");

    QObject::connect(quitButton, &QPushButton::clicked, qApp, &QCoreApplication::quit);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(testArea);
    layout->addWidget(quitButton);

    QWidget window;
    window.setLayout(layout);
    window.setWindowTitle("Mouse Button Tester");
    window.show();

    return app.exec();
}
