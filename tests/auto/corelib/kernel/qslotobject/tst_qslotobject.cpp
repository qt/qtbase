// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>

#include <QtCore/qscopedvaluerollback.h>

#include <QtTest/qtest.h>

class tst_QSlotObject : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void uniquePtr();
    void sharedPtr();
};

void tst_QSlotObject::uniquePtr()
{
    using Prototype = void(*)();
    bool exists = false;
    QtPrivate::SlotObjUniquePtr p;
    QVERIFY(!p);
    auto rb = std::make_unique<QScopedValueRollback<bool>>(exists, true); // make movable
    p.reset(QtPrivate::makeCallableObject<Prototype>([rb = std::move(rb)] {}));
    QVERIFY(p);
    QVERIFY(exists);
    p.reset();
    QVERIFY(!p);
    QVERIFY(!exists);
}

void tst_QSlotObject::sharedPtr()
{
    using Prototype = void(*)();
    bool exists = false;
    QtPrivate::SlotObjUniquePtr p;
    auto rb = std::make_unique<QScopedValueRollback<bool>>(exists, true); // make movable
    p.reset(QtPrivate::makeCallableObject<Prototype>([rb = std::move(rb)] {}));
    QVERIFY(p);
    QVERIFY(exists);

    QtPrivate::SlotObjSharedPtr sp{std::move(p)};
    QVERIFY(!p);
    QVERIFY(exists);

    {
        const auto copy = sp;
        QVERIFY(sp);
        QVERIFY(copy);
        QVERIFY(exists);

        sp = nullptr; // SlotObjSharedPtr doesn't have a reset()...
        QVERIFY(!sp);
        QVERIFY(copy);
        QVERIFY(exists);
    } // `copy` goes out of scope

    QVERIFY(!exists);
}

QTEST_APPLESS_MAIN(tst_QSlotObject)
#include "tst_qslotobject.moc"
