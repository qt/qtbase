/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcoreevent.h>

//TESTED_CLASS=
//TESTED_FILES=

class tst_QEvent : public QObject
{
    Q_OBJECT
public:
    tst_QEvent();
    ~tst_QEvent();

private slots:
    void registerEventType_data();
    void registerEventType();
};

tst_QEvent::tst_QEvent()
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
    // hint valid, but already taken
    QTest::newRow("MaxUser-1") << int(QEvent::MaxUser - 1) << int(QEvent::MaxUser - 2);
    // hint valid, but not taken
    QTest::newRow("User + 1000") << int(QEvent::User + 1000) << int(QEvent::User + 1000);
}

void tst_QEvent::registerEventType()
{
    QFETCH(int, hint);
    QFETCH(int, expected);
    QCOMPARE(QEvent::registerEventType(hint), expected);
}

QTEST_MAIN(tst_QEvent)
#include "tst_qevent.moc"
