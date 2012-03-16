/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTSYSTEM_H
#define QTESTSYSTEM_H

#include <QtTest/qtestcase.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qelapsedtimer.h>
#include <QtGui/QWindow>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QWidget;

namespace QTest
{
    inline static void qWait(int ms)
    {
        Q_ASSERT(QCoreApplication::instance());

        QElapsedTimer timer;
        timer.start();
        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents, ms);
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QTest::qSleep(10);
        } while (timer.elapsed() < ms);
    }

    inline static bool qWaitForWindowShown(QWidget *window)
    {
        Q_UNUSED(window);
        qWait(200);
        return true;
    }

    inline static bool qWaitForWindowActive(QWindow *window, int timeout = 1000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!window->isActive()) {
            int remaining = timeout - int(timer.elapsed());
            if (remaining <= 0)
                break;
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QTest::qSleep(10);
        }
        return window->isActive();
    }

    inline static bool qWaitForWindowExposed(QWindow *window, int timeout = 1000)
    {
        QElapsedTimer timer;
        timer.start();
        while (!window->isExposed()) {
            int remaining = timeout - int(timer.elapsed());
            if (remaining <= 0)
                break;
            QCoreApplication::processEvents(QEventLoop::AllEvents, remaining);
            QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
            QTest::qSleep(10);
        }
        return window->isExposed();
    }

    inline static bool qWaitForWindowShown(QWindow *window, int timeout = 1000)
    {
        return qWaitForWindowActive(window, timeout);
    }
}

QT_END_NAMESPACE

QT_END_HEADER

#endif

