// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtCore/qmetatype.h>

class tst_QMetaType : public QObject
{
    Q_OBJECT

public:
    tst_QMetaType();
    virtual ~tst_QMetaType();

private slots:
    void typeBuiltin_data();
    void typeBuiltin();
    void typeBuiltin_QByteArray_data();
    void typeBuiltin_QByteArray();
    void typeBuiltinNotNormalized_data();
    void typeBuiltinNotNormalized();
    void typeCustom();
    void typeCustomNotNormalized();
    void typeNotRegistered();
    void typeNotRegisteredNotNormalized();

    void typeNameBuiltin_data();
    void typeNameBuiltin();
    void typeNameCustom();
    void typeNameNotRegistered();

    void isRegisteredBuiltin_data();
    void isRegisteredBuiltin();
    void isRegisteredCustom();
    void isRegisteredNotRegistered();

    void constructInPlace_data();
    void constructInPlace();
    void constructInPlaceCopy_data();
    void constructInPlaceCopy();
    void constructInPlaceCopyStaticLess_data();
    void constructInPlaceCopyStaticLess();
};

tst_QMetaType::tst_QMetaType()
{
}

tst_QMetaType::~tst_QMetaType()
{
}

struct BigClass
{
    double n,i,e,r,o,b;
};
Q_DECLARE_METATYPE(BigClass);

void tst_QMetaType::typeBuiltin_data()
{
    QTest::addColumn<QByteArray>("typeName");
    for (int i = 0; i < QMetaType::User; ++i) {
        if (QMetaType metaType(i); metaType.isValid())
            QTest::newRow(metaType.name()) << QByteArray(metaType.name());
    }
}

// QMetaType::type(const char *)
void tst_QMetaType::typeBuiltin()
{
    QFETCH(QByteArray, typeName);
    const char *nm = typeName.constData();
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::fromName(nm);
    }
}

void tst_QMetaType::typeBuiltin_QByteArray_data()
{
    typeBuiltin_data();
}

// QMetaType::type(QByteArray)
void tst_QMetaType::typeBuiltin_QByteArray()
{
    QFETCH(QByteArray, typeName);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::fromName(typeName);
    }
}

void tst_QMetaType::typeBuiltinNotNormalized_data()
{
    QTest::addColumn<QByteArray>("typeName");
    for (int i = 0; i < QMetaType::User; ++i) {
        if (QMetaType metaType(i); metaType.isValid())
            QTest::newRow(metaType.name()) << QByteArray(metaType.name()).append(" ");
    }
}

void tst_QMetaType::typeBuiltinNotNormalized()
{
    QFETCH(QByteArray, typeName);
    const char *nm = typeName.constData();
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::fromName(nm);
    }
}

struct Foo { int i; };

void tst_QMetaType::typeCustom()
{
    qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::fromName("Foo");
    }
}

void tst_QMetaType::typeCustomNotNormalized()
{
    qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::fromName("Foo ");
    }
}

void tst_QMetaType::typeNotRegistered()
{
    Q_ASSERT(!QMetaType::fromName("Bar").isValid());
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::fromName("Bar");
    }
}

void tst_QMetaType::typeNotRegisteredNotNormalized()
{
    Q_ASSERT(!QMetaType::fromName("Bar").isValid());
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::fromName("Bar ");
    }
}

void tst_QMetaType::typeNameBuiltin_data()
{
    QTest::addColumn<int>("type");
    for (int i = 0; i < QMetaType::User; ++i) {
        if (QMetaType metaType(i); metaType.isValid())
            QTest::newRow(metaType.name()) << i;
    }
}

void tst_QMetaType::typeNameBuiltin()
{
    QFETCH(int, type);
    QBENCHMARK {
        for (int i = 0; i < 500000; ++i)
            QMetaType(type).name();
    }
}

void tst_QMetaType::typeNameCustom()
{
    int type = qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType(type).name();
    }
}

void tst_QMetaType::typeNameNotRegistered()
{
    // We don't care much about this case, but test it anyway.
    Q_ASSERT(QMetaType(-1).name() == nullptr);
    QBENCHMARK {
        for (int i = 0; i < 500000; ++i)
            QMetaType(-1).name();
    }
}

void tst_QMetaType::isRegisteredBuiltin_data()
{
    typeNameBuiltin_data();
}

void tst_QMetaType::isRegisteredBuiltin()
{
    QFETCH(int, type);
    QBENCHMARK {
        for (int i = 0; i < 500000; ++i)
            QMetaType::isRegistered(type);
    }
}

void tst_QMetaType::isRegisteredCustom()
{
    int type = qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::isRegistered(type);
    }
}

void tst_QMetaType::isRegisteredNotRegistered()
{
    Q_ASSERT(QMetaType(-1).name() == nullptr);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::isRegistered(-1);
    }
}

void tst_QMetaType::constructInPlace_data()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstCoreType; i <= QMetaType::LastCoreType; ++i) {
        auto name = QMetaType(i).name();
        if (name && i != QMetaType::Void)
            QTest::newRow(name) << i;
    }

    QTest::newRow("custom") << qMetaTypeId<BigClass>();
    // GUI types are tested in tst_QGuiMetaType.
}

void tst_QMetaType::constructInPlace()
{
    QFETCH(int, typeId);
    const QMetaType metaType(typeId);
    size_t size = metaType.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    QCOMPARE(metaType.construct(storage, /*copy=*/0), storage);
    metaType.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            metaType.construct(storage, /*copy=*/0);
            metaType.destruct(storage);
        }
    }
    qFreeAligned(storage);
}

void tst_QMetaType::constructInPlaceCopy_data()
{
    constructInPlace_data();
}

void tst_QMetaType::constructInPlaceCopy()
{
    QFETCH(int, typeId);
    const QMetaType metaType(typeId);
    size_t size = metaType.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    void *other = metaType.create();
    QCOMPARE(metaType.construct(storage, other), storage);
    metaType.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            metaType.construct(storage, other);
            metaType.destruct(storage);
        }
    }
    metaType.destroy(other);
    qFreeAligned(storage);
}

void tst_QMetaType::constructInPlaceCopyStaticLess_data()
{
    constructInPlaceCopy_data();
}

void tst_QMetaType::constructInPlaceCopyStaticLess()
{
    QFETCH(int, typeId);
    const QMetaType metaType(typeId);
    size_t size = metaType.sizeOf();
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    void *other = metaType.create();
    QCOMPARE(metaType.construct(storage, other), storage);
    metaType.destruct(storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            metaType.construct(storage, other);
            metaType.destruct(storage);
        }
    }
    metaType.destroy(other);
    qFreeAligned(storage);
}

QTEST_MAIN(tst_QMetaType)
#include "tst_bench_qmetatype.moc"
