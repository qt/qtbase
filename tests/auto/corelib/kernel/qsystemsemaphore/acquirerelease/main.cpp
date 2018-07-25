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
