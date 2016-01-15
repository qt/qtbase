/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtGui>
#include <QEvent>

struct MyApplication : public QGuiApplication
{
    MyApplication(int& argc, char** argv)
    : QGuiApplication(argc, argv)
    {}

    bool event(QEvent * event)
    {
        if (event->type() == QEvent::FileOpen) {
            QFileOpenEvent* ev = static_cast<QFileOpenEvent *>(event);
            QFile file;
            bool ok = ev->openFile(file, QFile::Append | QFile::Unbuffered);
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
