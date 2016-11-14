/****************************************************************************
**
** Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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
#include <QtCore/private/qhooks_p.h>

class tst_QHooks: public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void testVersion();
    void testAddRemoveObject();
    void testChaining();
};

void tst_QHooks::cleanup()
{
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
}

void tst_QHooks::testVersion()
{
    QVERIFY(qtHookData[QHooks::HookDataVersion] >= 3);
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

static QVector<QString> hookOrder;

static QHooks::AddQObjectCallback existingAddHook = 0;
static QHooks::RemoveQObjectCallback existingRemoveHook = 0;

static void firstAddHook(QObject *)
{
    hookOrder.append(QLatin1String("firstAddHook"));
}

static void firstRemoveHook(QObject *)
{
    hookOrder.append(QLatin1String("firstRemoveHook"));
}

static void secondAddHook(QObject *object)
{
    if (existingAddHook)
        existingAddHook(object);

    hookOrder.append(QLatin1String("secondAddHook"));
}

static void secondRemoveHook(QObject *object)
{
    if (existingRemoveHook)
        existingRemoveHook(object);

    hookOrder.append(QLatin1String("secondRemoveHook"));
}

// Tests that it's possible to "chain" hooks together (i.e. have multiple hooks)
void tst_QHooks::testChaining()
{
    QCOMPARE(qtHookData[QHooks::AddQObject], (quintptr)0);
    QCOMPARE(qtHookData[QHooks::RemoveQObject], (quintptr)0);

    // Set the add and remove hooks (could just skip this and go straight to the next step,
    // but it's for illustrative purposes).
    qtHookData[QHooks::AddQObject] = (quintptr)&firstAddHook;
    qtHookData[QHooks::RemoveQObject] = (quintptr)&firstRemoveHook;

    // Store them so that we can call them later.
    existingAddHook = reinterpret_cast<QHooks::AddQObjectCallback>(qtHookData[QHooks::AddQObject]);
    existingRemoveHook = reinterpret_cast<QHooks::RemoveQObjectCallback>(qtHookData[QHooks::RemoveQObject]);

    // Overide them with hooks that call them first.
    qtHookData[QHooks::AddQObject] = (quintptr)&secondAddHook;
    qtHookData[QHooks::RemoveQObject] = (quintptr)&secondRemoveHook;

    QObject *obj = new QObject;
    QCOMPARE(hookOrder.size(), 2);
    QCOMPARE(hookOrder.at(0), QLatin1String("firstAddHook"));
    QCOMPARE(hookOrder.at(1), QLatin1String("secondAddHook"));
    delete obj;
    QCOMPARE(hookOrder.size(), 4);
    QCOMPARE(hookOrder.at(2), QLatin1String("firstRemoveHook"));
    QCOMPARE(hookOrder.at(3), QLatin1String("secondRemoveHook"));

    hookOrder.clear();
}

QTEST_APPLESS_MAIN(tst_QHooks)
#include "tst_qhooks.moc"
