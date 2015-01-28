/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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
        const char *name = QMetaType::typeName(i);
        if (name)
            QTest::newRow(name) << QByteArray(name);
    }
}

// QMetaType::type(const char *)
void tst_QMetaType::typeBuiltin()
{
    QFETCH(QByteArray, typeName);
    const char *nm = typeName.constData();
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::type(nm);
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
            QMetaType::type(typeName);
    }
}

void tst_QMetaType::typeBuiltinNotNormalized_data()
{
    QTest::addColumn<QByteArray>("typeName");
    for (int i = 0; i < QMetaType::User; ++i) {
        const char *name = QMetaType::typeName(i);
        if (name)
            QTest::newRow(name) << QByteArray(name).append(" ");
    }
}

void tst_QMetaType::typeBuiltinNotNormalized()
{
    QFETCH(QByteArray, typeName);
    const char *nm = typeName.constData();
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::type(nm);
    }
}

struct Foo { int i; };

void tst_QMetaType::typeCustom()
{
    qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::type("Foo");
    }
}

void tst_QMetaType::typeCustomNotNormalized()
{
    qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::type("Foo ");
    }
}

void tst_QMetaType::typeNotRegistered()
{
    Q_ASSERT(QMetaType::type("Bar") == 0);
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::type("Bar");
    }
}

void tst_QMetaType::typeNotRegisteredNotNormalized()
{
    Q_ASSERT(QMetaType::type("Bar") == 0);
    QBENCHMARK {
        for (int i = 0; i < 10000; ++i)
            QMetaType::type("Bar ");
    }
}

void tst_QMetaType::typeNameBuiltin_data()
{
    QTest::addColumn<int>("type");
    for (int i = 0; i < QMetaType::User; ++i) {
        const char *name = QMetaType::typeName(i);
        if (name)
            QTest::newRow(name) << i;
    }
}

void tst_QMetaType::typeNameBuiltin()
{
    QFETCH(int, type);
    QBENCHMARK {
        for (int i = 0; i < 500000; ++i)
            QMetaType::typeName(type);
    }
}

void tst_QMetaType::typeNameCustom()
{
    int type = qRegisterMetaType<Foo>("Foo");
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::typeName(type);
    }
}

void tst_QMetaType::typeNameNotRegistered()
{
    // We don't care much about this case, but test it anyway.
    Q_ASSERT(QMetaType::typeName(-1) == 0);
    QBENCHMARK {
        for (int i = 0; i < 500000; ++i)
            QMetaType::typeName(-1);
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
    Q_ASSERT(QMetaType::typeName(-1) == 0);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i)
            QMetaType::isRegistered(-1);
    }
}

void tst_QMetaType::constructInPlace_data()
{
    QTest::addColumn<int>("typeId");
    for (int i = QMetaType::FirstCoreType; i <= QMetaType::LastCoreType; ++i)
        if (i != QMetaType::Void)
            QTest::newRow(QMetaType::typeName(i)) << i;

    QTest::newRow("custom") << qMetaTypeId<BigClass>();
    // GUI types are tested in tst_QGuiMetaType.
}

void tst_QMetaType::constructInPlace()
{
    QFETCH(int, typeId);
    int size = QMetaType::sizeOf(typeId);
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    QCOMPARE(QMetaType::construct(typeId, storage, /*copy=*/0), storage);
    QMetaType::destruct(typeId, storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            QMetaType::construct(typeId, storage, /*copy=*/0);
            QMetaType::destruct(typeId, storage);
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
    int size = QMetaType::sizeOf(typeId);
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    void *other = QMetaType::create(typeId);
    QCOMPARE(QMetaType::construct(typeId, storage, other), storage);
    QMetaType::destruct(typeId, storage);
    QBENCHMARK {
        for (int i = 0; i < 100000; ++i) {
            QMetaType::construct(typeId, storage, other);
            QMetaType::destruct(typeId, storage);
        }
    }
    QMetaType::destroy(typeId, other);
    qFreeAligned(storage);
}

void tst_QMetaType::constructInPlaceCopyStaticLess_data()
{
    constructInPlaceCopy_data();
}

void tst_QMetaType::constructInPlaceCopyStaticLess()
{
    QFETCH(int, typeId);
    int size = QMetaType::sizeOf(typeId);
    void *storage = qMallocAligned(size, 2 * sizeof(qlonglong));
    void *other = QMetaType::create(typeId);
    QCOMPARE(QMetaType::construct(typeId, storage, other), storage);
    QMetaType::destruct(typeId, storage);
    QBENCHMARK {
        QMetaType type(typeId);
        for (int i = 0; i < 100000; ++i) {
            type.construct(storage, other);
            type.destruct(storage);
        }
    }
    QMetaType::destroy(typeId, other);
    qFreeAligned(storage);
}

QTEST_MAIN(tst_QMetaType)
#include "tst_qmetatype.moc"
