/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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


#include <QtTest/QtTest>
#include <QtCore/private/qhooks_p.h>

class tst_QHooks: public QObject
{
    Q_OBJECT

private slots:
    void testVersion();
    void testAddRemoveObject();
};

void tst_QHooks::testVersion()
{
    QVERIFY(qtHookData[QHooks::HookDataVersion] >= 1);
    QCOMPARE(qtHookData[QHooks::HookDataSize], (quintptr)QHooks::LastHookIndex);
    QCOMPARE(qtHookData[QHooks::QtVersion], (quintptr)QT_VERSION);
}

static int objectCount = 0;

static void objectAddHook(QObject*)
{
    ++objectCount;
}

static void objectRemoveHook(QObject*)
{
    --objectCount;
}

void tst_QHooks::testAddRemoveObject()
{
    QCOMPARE(qtHookData[QHooks::AddQObject], (quintptr)0);
    QCOMPARE(qtHookData[QHooks::RemoveQObject], (quintptr)0);

    qtHookData[QHooks::AddQObject] = (quintptr)&objectAddHook;
    qtHookData[QHooks::RemoveQObject] = (quintptr)&objectRemoveHook;

    QCOMPARE(objectCount, 0);
    QObject *obj = new QObject;
    QVERIFY(objectCount > 0);
    delete obj;
    QCOMPARE(objectCount, 0);
}

QTEST_APPLESS_MAIN(tst_QHooks)
#include "tst_qhooks.moc"
