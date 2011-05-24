/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTEVENTLOOP_H
#define QTESTEVENTLOOP_H

#include <QtTest/qtest_global.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qeventloop.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Test)

class Q_TESTLIB_EXPORT QTestEventLoop : public QObject
{
    Q_OBJECT

public:
    inline QTestEventLoop(QObject *aParent = 0)
        : QObject(aParent), inLoop(false), _timeout(false), timerId(-1), loop(0) {}
    inline void enterLoop(int secs);


    inline void changeInterval(int secs)
    { killTimer(timerId); timerId = startTimer(secs * 1000); }

    inline bool timeout() const
    { return _timeout; }

    inline static QTestEventLoop &instance()
    {
        static QPointer<QTestEventLoop> testLoop;
        if (testLoop.isNull())
            testLoop = new QTestEventLoop(QCoreApplication::instance());
        return *static_cast<QTestEventLoop *>(testLoop);
    }

public Q_SLOTS:
    inline void exitLoop();

protected:
    inline void timerEvent(QTimerEvent *e);

private:
    bool inLoop;
    bool _timeout;
    int timerId;

    QEventLoop *loop;
};

inline void QTestEventLoop::enterLoop(int secs)
{
    Q_ASSERT(!loop);

    QEventLoop l;

    inLoop = true;
    _timeout = false;

    timerId = startTimer(secs * 1000);

    loop = &l;
    l.exec();
    loop = 0;
}

inline void QTestEventLoop::exitLoop()
{
    if (timerId != -1)
        killTimer(timerId);
    timerId = -1;

    if (loop)
        loop->exit();

    inLoop = false;
}

inline void QTestEventLoop::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != timerId)
        return;
    _timeout = true;
    exitLoop();
}

QT_END_NAMESPACE

QT_END_HEADER

#endif
