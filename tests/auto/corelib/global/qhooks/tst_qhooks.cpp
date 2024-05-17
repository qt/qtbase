// Copyright (C) 2014 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Volker Krause <volker.krause@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QTest>
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

static QList<QString> hookOrder;

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
