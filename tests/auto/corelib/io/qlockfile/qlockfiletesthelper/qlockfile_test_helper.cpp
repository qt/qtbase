/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
        QLockFile *lockFile = new QLockFile(lockName);
        lockFile->lock();
        // leak the lockFile on purpose, so that the lock remains!
        return 0;
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
