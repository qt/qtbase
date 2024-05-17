// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtCore/qmetatype.h>
#include <QScopeGuard>

class tst_QGuiMetaType : public QObject
{
    Q_OBJECT

private slots:
    void constructInPlace_data();
    void constructInPlace();
    void constructInPlaceCopy_data();
    void constructInPlaceCopy();
private:
    void constructableGuiTypes();
};


void tst_QGuiMetaType::constructableGuiTypes()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstGuiType; i <= QMetaType::LastGuiType; ++i) {
        if (QMetaType metaType(i); metaType.isValid())
            QTest::newRow(metaType.name()) << i;
    }
}


void tst_QGuiMetaType::constructInPlace_data()
{
    constructableGuiTypes();
}

void tst_QGuiMetaType::constructInPlace()
{
    QFETCH(int, typeId);
    QMetaType type(typeId);
    int size = type.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    auto cleanUp = qScopeGuard([&]() {
        qFreeAligned(storage);
    });
    QCOMPARE(type.construct(storage, /*copy=*/0), storage);
    type.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            type.construct(storage, /*copy=*/0);
            type.destruct(storage);
        }
    }
}

void tst_QGuiMetaType::constructInPlaceCopy_data()
{
    constructableGuiTypes();
}

void tst_QGuiMetaType::constructInPlaceCopy()
{
    QFETCH(int, typeId);
    QMetaType type(typeId);
    int size = type.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    void *other = type.create();
    auto cleanUp = qScopeGuard([&]() {
        type.destroy(other);
        qFreeAligned(storage);
    });
    QCOMPARE(type.construct(storage, other), storage);
    type.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            type.construct(storage, other);
            type.destruct(storage);
        }
    }
}

QTEST_MAIN(tst_QGuiMetaType)
#include "tst_qguimetatype.moc"
