/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <QSystemSemaphore>

int acquire(int count = 1)
{
    QSystemSemaphore sem("store");

    for (int i = 0; i < count; ++i) {
        if (!sem.acquire()) {
            qWarning() << "Could not acquire" << sem.key();
            return EXIT_FAILURE;
        }
    }
    qDebug("done aquiring");
    return EXIT_SUCCESS;
}

int release()
{
    QSystemSemaphore sem("store");
    if (!sem.release()) {
        qWarning() << "Could not release" << sem.key();
        return EXIT_FAILURE;
    }
    qDebug("done releasing");
    return EXIT_SUCCESS;
}

int acquirerelease()
{
    QSystemSemaphore sem("store");
    if (!sem.acquire()) {
        qWarning() << "Could not acquire" << sem.key();
        return EXIT_FAILURE;
    }
    if (!sem.release()) {
        qWarning() << "Could not release" << sem.key();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QStringList arguments = app.arguments();
    // binary name is not used here
    arguments.takeFirst();
    if (arguments.count() < 1) {
        qWarning("Please call the helper with the function to call as argument");
        return EXIT_FAILURE;
    }
    QString function = arguments.takeFirst();
    if (function == QLatin1String("acquire")) {
        int count = 1;
        bool ok = true;
        if (arguments.count())
            count = arguments.takeFirst().toInt(&ok);
        if (!ok)
            count = 1;
        return acquire(count);
    } else if (function == QLatin1String("release")) {
        return release();
    } else if (function == QLatin1String("acquirerelease")) {
        return acquirerelease();
    } else {
        qWarning() << "Unknown function" << function;
    }
    return EXIT_SUCCESS;
}
