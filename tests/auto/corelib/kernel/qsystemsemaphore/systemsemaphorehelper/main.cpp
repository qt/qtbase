/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
