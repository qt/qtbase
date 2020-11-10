// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QPushButton>
#include <QFileInfo>
#include <QDir>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

//! [0]
    QFileInfo fileInfo1("~/examples/191697/.");
    QFileInfo fileInfo2("~/examples/191697/..");
    QFileInfo fileInfo3("~/examples/191697/main.cpp");
//! [0]
//! [1]
    QFileInfo fileInfo4(".");
    QFileInfo fileInfo5("..");
    QFileInfo fileInfo6("main.cpp");
//! [1]

    qDebug() << fileInfo1.fileName();
    qDebug() << fileInfo2.fileName();
    qDebug() << fileInfo3.fileName();
    qDebug() << fileInfo4.fileName();
    qDebug() << fileInfo5.fileName();
    qDebug() << fileInfo6.fileName();

    QGroupBox *groupBox = new QGroupBox(QStringLiteral("QFileInfo::dir() test"));

    QVBoxLayout *vbox = new QVBoxLayout(groupBox);

    QPushButton* button1 = new QPushButton(fileInfo1.dir().path());
    QPushButton* button2 = new QPushButton(fileInfo2.dir().path());
    QPushButton* button3 = new QPushButton(fileInfo3.dir().path());
    QPushButton* button4 = new QPushButton(fileInfo4.dir().path());
    QPushButton* button5 = new QPushButton(fileInfo5.dir().path());
    QPushButton* button6 = new QPushButton(fileInfo6.dir().path());
    vbox->addWidget(button1);
    vbox->addWidget(button2);
    vbox->addWidget(button3);
    vbox->addWidget(button4);
    vbox->addWidget(button5);
    vbox->addWidget(button6);
    vbox->addStretch(1);

    groupBox->show();

    return app.exec();
}
