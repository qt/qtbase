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


#include <QtTest/QtTest>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>

class tst_QEvent : public QObject
{
    Q_OBJECT
public:
    tst_QEvent();
    ~tst_QEvent();

private slots:
    void registerEventType_data();
    void registerEventType();
    void exhaustEventTypeRegistration(); // keep behind registerEventType() test

private:
    bool registerEventTypeSucceeded; // track success of registerEventType for use by exhaustEventTypeRegistration()
};

tst_QEvent::tst_QEvent()
    : registerEventTypeSucceeded(true)
{ }

tst_QEvent::~tst_QEvent()
{ }

void tst_QEvent::registerEventType_data()
{
    QTest::addColumn<int>("hint");
    QTest::addColumn<int>("expected");

    // default argument
    QTest::newRow("default") << -1 << int(QEvent::MaxUser);
    // hint not valid
    QTest::newRow("User-1") << int(QEvent::User - 1) << int(QEvent::MaxUser - 1);
    // hint not valid II
    QTest::newRow("MaxUser+1") << int(QEvent::MaxUser + 1) << int(QEvent::MaxUser - 2);
    // hint valid, but already taken
    QTest::newRow("MaxUser-1") << int(QEvent::MaxUser - 1) << int(QEvent::MaxUser - 3);
    // hint valid, but not taken
    QTest::newRow("User + 1000") << int(QEvent::User + 1000) << int(QEvent::User + 1000);
}

void tst_QEvent::registerEventType()
{
    const bool oldRegisterEventTypeSucceeded = registerEventTypeSucceeded;
    registerEventTypeSucceeded = false;
    QFETCH(int, hint);
    QFETCH(int, expected);
    QCOMPARE(QEvent::registerEventType(hint), expected);
    registerEventTypeSucceeded = oldRegisterEventTypeSucceeded;
}

void tst_QEvent::exhaustEventTypeRegistration()
{
    if (!registerEventTypeSucceeded)
        QSKIP("requires the previous test (registerEventType) to have finished successfully");

    int i = QEvent::User;
    int result;
    while ((result = QEvent::registerEventType(i)) == i)
        ++i;
    QCOMPARE(i, int(QEvent::User + 1000));
    QCOMPARE(result, int(QEvent::MaxUser - 4));
    i = QEvent::User + 1001;
    while ((result = QEvent::registerEventType(i)) == i)
        ++i;
    QCOMPARE(result, -1);
    QCOMPARE(i, int(QEvent::MaxUser - 4));
}

QTEST_MAIN(tst_QEvent)
#include "tst_qevent.moc"
