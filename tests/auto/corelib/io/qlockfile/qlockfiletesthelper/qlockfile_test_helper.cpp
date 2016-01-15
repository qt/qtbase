/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
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

#include <QDebug>
#include <QCoreApplication>
#include <QLockFile>
#include <QThread>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (argc <= 1)
        return -1;

    const QString lockName = QString::fromLocal8Bit(argv[1]);

    QString option;
    if (argc > 2)
        option = QString::fromLocal8Bit(argv[2]);

    if (option == "-crash") {
        QLockFile lockFile(lockName);
        lockFile.lock();
        // exit on purpose, so that the lock remains!
        exit(0);
    } else if (option == "-busy") {
        QLockFile lockFile(lockName);
        lockFile.lock();
        QThread::msleep(500);
        return 0;
    } else {
        QLockFile lockFile(lockName);
        if (lockFile.isLocked()) // cannot happen, before calling lock or tryLock
            return QLockFile::UnknownError;

        lockFile.tryLock();
        return lockFile.error();
    }
}
