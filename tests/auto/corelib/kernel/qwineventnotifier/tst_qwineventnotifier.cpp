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

#include <QtTest/QtTest>
#include <qwineventnotifier.h>
#include <qtimer.h>
#include <qt_windows.h>

class tst_QWinEventNotifier : public QObject
{
    Q_OBJECT

protected slots:
    void simple_activated();
    void simple_timerSet();
private slots:
    void simple();

private:
    HANDLE simpleHEvent;
    bool simpleActivated;
};

void tst_QWinEventNotifier::simple_activated()
{
    simpleActivated = true;
    ResetEvent((HANDLE)simpleHEvent);
    QTestEventLoop::instance().exitLoop();
}

void tst_QWinEventNotifier::simple_timerSet()
{
    SetEvent((HANDLE)simpleHEvent);
}

void tst_QWinEventNotifier::simple()
{
    simpleHEvent = CreateEvent(0, true, false, 0);
    QVERIFY(simpleHEvent);

    QWinEventNotifier n(simpleHEvent);
    QObject::connect(&n, SIGNAL(activated(HANDLE)), this, SLOT(simple_activated()));
    simpleActivated = false;

    SetEvent((HANDLE)simpleHEvent);

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Timed out");

    QVERIFY(simpleActivated);


    simpleActivated = false;

    QTimer::singleShot(3000, this, SLOT(simple_timerSet()));

    QTestEventLoop::instance().enterLoop(30);
    if (QTestEventLoop::instance().timeout())
        QFAIL("Timed out");

    QVERIFY(simpleActivated);
}

QTEST_MAIN(tst_QWinEventNotifier)

#include "tst_qwineventnotifier.moc"
