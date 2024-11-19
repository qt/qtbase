// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/qguiapplication.h>
#include <QtGui/qevent.h>

#include <QtCore/qfile.h>

struct MyApplication : public QGuiApplication
{
    MyApplication(int& argc, char** argv)
    : QGuiApplication(argc, argv)
    {}

    bool event(QEvent * event) override
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent* ev = static_cast<QFileOpenEvent *>(event);
            QFile file(ev->file());
            bool ok = file.open(QFile::Append | QFile::Unbuffered);
            if (ok)
                file.write(QByteArray("+external"));
            return true;
        } else {
            return QGuiApplication::event(event);
        }
    }
};

int main(int argc, char *argv[])
{
    MyApplication a(argc, argv);
    a.sendPostedEvents(&a, QEvent::FileOpen);
    return 0;
}
