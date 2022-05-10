// Copyright (C) 2016 Samuel Gaist <samuel.gaist@edeltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [QApplication subclass]
#include <QApplication>
#include <QDebug>
#include <QFileOpenEvent>
#include <QPushButton>

class MyApplication : public QApplication
{
public:
    MyApplication(int &argc, char **argv)
        : QApplication(argc, argv)
    {
    }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
            qDebug() << "Open file" << openEvent->file();
        }

        return QApplication::event(event);
    }
};
//! [QApplication subclass]

int main(int argc, char *argv[])
{
    MyApplication app(argc, argv);
    QPushButton closeButton("Quit");
    QObject::connect(&closeButton, &QPushButton::clicked, &app, &QApplication::quit);
    closeButton.show();
    return app.exec();
}
