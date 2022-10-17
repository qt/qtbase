// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

#define COMPARE_WITH_TYPE(Type, arg1, arg2) \
switch (Type) { \
    case QTest::ComparisonOperation::CustomCompare:      QCOMPARE(arg1, arg2); break; \
    case QTest::ComparisonOperation::Equal:              QCOMPARE_EQ(arg1, arg2); break; \
    case QTest::ComparisonOperation::NotEqual:           QCOMPARE_NE(arg1, arg2); break; \
    case QTest::ComparisonOperation::LessThan:           QCOMPARE_LT(arg1, arg2); break; \
    case QTest::ComparisonOperation::LessThanOrEqual:    QCOMPARE_LE(arg1, arg2); break; \
    case QTest::ComparisonOperation::GreaterThan:        QCOMPARE_GT(arg1, arg2); break; \
    case QTest::ComparisonOperation::GreaterThanOrEqual: QCOMPARE_GE(arg1, arg2); break; \
}

class MyClass
{
public:
    MyClass(int v) : m_value(v) {}
    int value() const { return m_value; }
    void setValue(int v) { m_value = v; }

    friend bool operator==(const MyClass &lhs, const MyClass &rhs)
    {
        return lhs.m_value == rhs.m_value;
    }
    friend bool operator!=(const MyClass &lhs, const MyClass &rhs)
    {
        return lhs.m_value != rhs.m_value;
    }
    friend bool operator<(const MyClass &lhs, const MyClass &rhs)
    {
        return lhs.m_value < rhs.m_value;
    }
    friend bool operator<=(const MyClass &lhs, const MyClass &rhs)
    {
        return lhs.m_value <= rhs.m_value;
    }
    friend bool operator>(const MyClass &lhs, const MyClass &rhs)
    {
        return lhs.m_value > rhs.m_value;
    }
    friend bool operator>=(const MyClass &lhs, const MyClass &rhs)
    {
        return lhs.m_value >= rhs.m_value;
    }

private:
    int m_value;
};

// ClassWithPointerGetter returns a pointer, so pointers will be used during
// comparison. To get consistent results, we need to make sure that the pointer
// returned by first object is always smaller than the one returned by the
// second object.
// We will make sure that the objects are not destroyed until the comparison
// is finished by checking that the output does not contain 'MyClass(-1)'.
static MyClass valuesForClassWithPointerGetter[] = { MyClass(-1), MyClass(-1) };

class ClassWithPointerGetter
{
    Q_DISABLE_COPY_MOVE(ClassWithPointerGetter)
public:
    explicit ClassWithPointerGetter(int index) : m_index(index)
    {
        Q_ASSERT(m_index >= 0 && m_index < 2);
        valuesForClassWithPointerGetter[m_index].setValue(indexToValue(m_index));
    }
    ~ClassWithPointerGetter()
    {
        valuesForClassWithPointerGetter[m_index].setValue(-1);
    }

    const MyClass *getValuePointer() const { return &valuesForClassWithPointerGetter[m_index]; }

    static int indexToValue(int index) { return 2 - index; }
    static int valueToIndex(int value) { return 2 - value; }

private:
    int m_index;
};

// An auxiliary function to get a temporary object
static ClassWithPointerGetter getClassForValue(int val)
{
    return ClassWithPointerGetter(val);
}

// various toString() overloads
namespace QTest {

char *toString(const int *val)
{
    return val ? toString(*val) : toString(nullptr);
}

char *toString(const MyClass &val)
{
    char *msg = new char[128];
    qsnprintf(msg, 128, "MyClass(%d)", val.value());
    return msg;
}

char *toString(const MyClass *val)
{
    if (val) {
        char *msg = new char[128];
        const auto value = val->value();
        qsnprintf(msg, 128, "MyClass(%d) on memory address with index %d", value,
                  ClassWithPointerGetter::valueToIndex(value));
        return msg;
    }
    return toString(nullptr);
}

} // namespace QTest

enum MyUnregisteredEnum { MyUnregisteredEnumValue1, MyUnregisteredEnumValue2 };

class tst_ExtendedCompare : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase_data();
    void compareInts_data();
    void compareInts();
    void compareFloats_data();
    void compareFloats();
    void compareDoubles_data();
    void compareDoubles();
    void comparePointers_data();
    void comparePointers();
    void compareToNullptr_data();
    void compareToNullptr();
    void compareUnregistereEnum_data();
    void compareUnregistereEnum();
    void compareRegistereEnum_data();
    void compareRegistereEnum();
    void compareCustomTypes_data();
    void compareCustomTypes();
    void checkComparisonForTemporaryObjects();
    void checkComparisonWithTimeout();
};

void tst_ExtendedCompare::initTestCase_data()
{
    qRegisterMetaType<QTest::ComparisonOperation>();
    QTest::addColumn<QTest::ComparisonOperation>("operation");
    // Do not test plain old QCOMPARE() intentionally, as it's tested in other
    // places.
    QTest::newRow("EQ") << QTest::ComparisonOperation::Equal;
    QTest::newRow("NE") << QTest::ComparisonOperation::NotEqual;
    QTest::newRow("LT") << QTest::ComparisonOperation::LessThan;
    QTest::newRow("LE") << QTest::ComparisonOperation::LessThanOrEqual;
    QTest::newRow("GT") << QTest::ComparisonOperation::GreaterThan;
    QTest::newRow("GE") << QTest::ComparisonOperation::GreaterThanOrEqual;
}

template <typename T> static void generateData(T val1, T val2)
{
    Q_ASSERT(val1 < val2);
    QTest::addColumn<T>("lhs");
    QTest::addColumn<T>("rhs");
    QTest::newRow("left == right") << val1 << val1;
    QTest::newRow("left < right") << val1 << val2;
    QTest::newRow("left > right") << val2 << val1;
}

template <typename T> static void executeComparison()
{
    QFETCH_GLOBAL(QTest::ComparisonOperation, operation);
    QFETCH(T, lhs);
    QFETCH(T, rhs);
    switch (operation) {
    case QTest::ComparisonOperation::CustomCompare:      QCOMPARE(lhs, rhs); break;
    case QTest::ComparisonOperation::Equal:              QCOMPARE_EQ(lhs, rhs); break;
    case QTest::ComparisonOperation::NotEqual:           QCOMPARE_NE(lhs, rhs); break;
    case QTest::ComparisonOperation::LessThan:           QCOMPARE_LT(lhs, rhs); break;
    case QTest::ComparisonOperation::LessThanOrEqual:    QCOMPARE_LE(lhs, rhs); break;
    case QTest::ComparisonOperation::GreaterThan:        QCOMPARE_GT(lhs, rhs); break;
    case QTest::ComparisonOperation::GreaterThanOrEqual: QCOMPARE_GE(lhs, rhs); break;
    }
}

void tst_ExtendedCompare::compareInts_data()
{
    generateData(1, 2);
}

void tst_ExtendedCompare::compareInts()
{
    executeComparison<int>();
}

void tst_ExtendedCompare::compareFloats_data()
{
    generateData(1.0f, 1.1f);
}

void tst_ExtendedCompare::compareFloats()
{
    executeComparison<float>();
}

void tst_ExtendedCompare::compareDoubles_data()
{
    generateData(0.0, 0.1);
}

void tst_ExtendedCompare::compareDoubles()
{
    executeComparison<double>();
}

void tst_ExtendedCompare::comparePointers_data()
{
    static constexpr int values[] = { 1, 2 };
    generateData(&values[0], &values[1]);
}

void tst_ExtendedCompare::comparePointers()
{
    executeComparison<const int *>();
}

void tst_ExtendedCompare::compareToNullptr_data()
{
    static const int *ptr = nullptr;
    static const int value = 1;
    generateData(ptr, &value);
}

void tst_ExtendedCompare::compareToNullptr()
{
    executeComparison<const int *>();
}

void tst_ExtendedCompare::compareUnregistereEnum_data()
{
    generateData(MyUnregisteredEnumValue1, MyUnregisteredEnumValue2);
}

void tst_ExtendedCompare::compareUnregistereEnum()
{
    executeComparison<MyUnregisteredEnum>();
}

void tst_ExtendedCompare::compareRegistereEnum_data()
{
    generateData(Qt::Monday, Qt::Sunday);
}

void tst_ExtendedCompare::compareRegistereEnum()
{
    executeComparison<Qt::DayOfWeek>();
}

void tst_ExtendedCompare::compareCustomTypes_data()
{
    static const MyClass val1(1);
    static const MyClass val2(2);
    generateData(val1, val2);
}

void tst_ExtendedCompare::compareCustomTypes()
{
    executeComparison<MyClass>();
}

void tst_ExtendedCompare::checkComparisonForTemporaryObjects()
{
    // This test checks that temporary objects live until the end of
    // comparison.

    QFETCH_GLOBAL(QTest::ComparisonOperation, operation);
    COMPARE_WITH_TYPE(operation, getClassForValue(0).getValuePointer(),
                      getClassForValue(1).getValuePointer());
}

class ClassWithDeferredSetter : public MyClass
{
public:
    ClassWithDeferredSetter(int value) : MyClass(value) {}

    void setValueDeferred(int value)
    {
        QTimer::singleShot(100, [this, value] { setValue(value); });
    }
};

namespace QTest {

char *toString(const ClassWithDeferredSetter &val)
{
    char *msg = new char[128];
    qsnprintf(msg, 128, "ClassWithDeferredSetter(%d)", val.value());
    return msg;
}

} // namespace QTest

void tst_ExtendedCompare::checkComparisonWithTimeout()
{
    QFETCH_GLOBAL(QTest::ComparisonOperation, operation);
    ClassWithDeferredSetter c(0);
    c.setValueDeferred(1);
    switch (operation) {
    case QTest::ComparisonOperation::Equal:
        QTRY_COMPARE_EQ_WITH_TIMEOUT(c, ClassWithDeferredSetter(1), 300);
        break;
    case QTest::ComparisonOperation::NotEqual:
        QTRY_COMPARE_NE_WITH_TIMEOUT(c, ClassWithDeferredSetter(0), 300);
        break;
    case QTest::ComparisonOperation::LessThan:
        QTRY_COMPARE_LT_WITH_TIMEOUT(c, ClassWithDeferredSetter(0), 300);
        break;
    case QTest::ComparisonOperation::LessThanOrEqual:
        QTRY_COMPARE_LE_WITH_TIMEOUT(c, ClassWithDeferredSetter(-1), 300);
        break;
    case QTest::ComparisonOperation::GreaterThan:
        QTRY_COMPARE_GT_WITH_TIMEOUT(c, ClassWithDeferredSetter(1), 300);
        break;
    case QTest::ComparisonOperation::GreaterThanOrEqual:
        QTRY_COMPARE_GE_WITH_TIMEOUT(c, ClassWithDeferredSetter(1), 300);
        break;
    case QTest::ComparisonOperation::CustomCompare:
        QFAIL("Unexpected comparison operation");
        break;
    }
}

QT_END_NAMESPACE

QTEST_MAIN(tst_ExtendedCompare)
#include "tst_extendedcompare.moc"
