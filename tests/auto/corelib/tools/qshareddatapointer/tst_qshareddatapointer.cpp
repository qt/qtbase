// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtTest/private/qcomparisontesthelper_p.h>

#include <QtCore/QSharedData>

class tst_QSharedDataPointer : public QObject
{
    Q_OBJECT

private slots:
    void compareCompiles();
    void compare();
};

class MyClass : public QSharedData
{
public:
    MyClass(int v) : m_value(v)
    {
        ref.ref();
    };
    int m_value;
};

void tst_QSharedDataPointer::compareCompiles()
{
    QTestPrivate::testAllComparisonOperatorsCompile<QSharedDataPointer<MyClass>>();
    QTestPrivate::testAllComparisonOperatorsCompile<QSharedDataPointer<MyClass>, MyClass*>();
    QTestPrivate::testAllComparisonOperatorsCompile<QSharedDataPointer<MyClass>, std::nullptr_t>();
}

void tst_QSharedDataPointer::compare()
{
    const QSharedDataPointer<MyClass> ptr;
    const QSharedDataPointer<MyClass> ptr2;
    QT_TEST_ALL_COMPARISON_OPS(ptr, nullptr, Qt::strong_ordering::equal);
    QT_TEST_ALL_COMPARISON_OPS(ptr2, nullptr, Qt::strong_ordering::equal);
    QT_TEST_ALL_COMPARISON_OPS(ptr, ptr2, Qt::strong_ordering::equal);

    const QSharedDataPointer<MyClass> copy(ptr);
    QT_TEST_ALL_COMPARISON_OPS(ptr, copy, Qt::strong_ordering::equal);

    const MyClass* pointer = nullptr;
    QT_TEST_ALL_COMPARISON_OPS(pointer, ptr, Qt::strong_ordering::equal);

    const QSharedDataPointer<MyClass> pointer2(new MyClass(0), QAdoptSharedDataTag{});
    QT_TEST_ALL_COMPARISON_OPS(pointer, pointer2, Qt::strong_ordering::less);

    std::array<MyClass, 2> myArray {MyClass(1), MyClass(0)};
    const QSharedDataPointer<const MyClass> val0(&myArray[0]);
    const QSharedDataPointer<const MyClass> val1(&myArray[1]);
    QT_TEST_ALL_COMPARISON_OPS(val0, val1, Qt::strong_ordering::less);
    QT_TEST_ALL_COMPARISON_OPS(val0, &myArray[1], Qt::strong_ordering::less);
    QT_TEST_ALL_COMPARISON_OPS(val1, val0, Qt::strong_ordering::greater);
    QT_TEST_ALL_COMPARISON_OPS(&myArray[1], val0, Qt::strong_ordering::greater);
}

QTEST_MAIN(tst_QSharedDataPointer)

#include "tst_qshareddatapointer.moc"
