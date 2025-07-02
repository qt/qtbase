// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QtCore/qcompare.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

class MyClass
{
public:
    MyClass(int v) : m_value(v) {}
    int value() const { return m_value; }
    void setValue(int v) { m_value = v; }

    friend bool comparesEqual(const MyClass &lhs, const MyClass &rhs) noexcept
    {
        return lhs.m_value == rhs.m_value;
    }
    friend Qt::strong_ordering compareThreeWay(const MyClass &lhs,
                                               const MyClass &rhs) noexcept
    {
        return Qt::compareThreeWay(lhs.m_value, rhs.m_value);
    }
    Q_DECLARE_STRONGLY_ORDERED(MyClass);
private:
    int m_value;
};
static MyClass arrayOfMyClassValues[] = { MyClass(-1), MyClass(-1) };

class ClassWithPointerGetter
{
    Q_DISABLE_COPY_MOVE(ClassWithPointerGetter)
public:
    explicit ClassWithPointerGetter(int index) : m_index(index)
    {
        Q_ASSERT(m_index >= 0 && m_index < 2);
        arrayOfMyClassValues[m_index].setValue(indexToValue(m_index));
    }
    ~ClassWithPointerGetter()
    {
        arrayOfMyClassValues[m_index].setValue(-1);
    }

    const MyClass *getValuePointer() const { return &arrayOfMyClassValues[m_index]; }

    static int indexToValue(int index) { return 2 - index; }
    static int valueToIndex(int value) { return 2 - value; }

private:
    int m_index;
};

#ifdef __cpp_lib_three_way_comparison
// An auxiliary function to get a temporary object
static ClassWithPointerGetter getClassForValue(int val)
{
    return ClassWithPointerGetter(val);
}
#endif

// various toString() overloads
namespace QTest {

template <> char *toString(const int *const &val)
{
    return val ? toString(*val) : toString(nullptr);
}

} // namespace QTest

char *toString(const MyClass &val)
{
    char *msg = new char[128];
    std::snprintf(msg, 128, "MyClass(%d)", val.value());
    return msg;
}

char *toString(const MyClass *val)
{
    if (val) {
        char *msg = new char[128];
        const auto value = val->value();
        std::snprintf(msg, 128, "MyClass(%d) on memory address with index %d", value,
                      ClassWithPointerGetter::valueToIndex(value));
        return msg;
    }
    return toString(nullptr);
}

enum MyUnregisteredEnum { MyUnregisteredEnumValue1, MyUnregisteredEnumValue2 };

namespace {
static constexpr int values[] = { 1, 2 };
static const MyClass val1(1);
static const MyClass val2(2);
static const int *ptr = nullptr;
static const int value = 1;
}

class tst_ThreeWayCompare : public QObject
{
    Q_OBJECT

private:
    template <typename T, typename OrderingType>
    void generateData(T val1, T val2, OrderingType order);
    template <typename T, typename OrderingType> void executeComparison();

private slots:
    void compareInts_data() { generateData(1, 2, Qt::strong_ordering::equal); }
    void compareInts() { executeComparison<int, Qt::strong_ordering>(); }
    void compareFloats_data() { generateData(1.0f, 1.1f, Qt::partial_ordering::less); }
    void compareFloats() { executeComparison<float, Qt::partial_ordering>(); }
    void compareDoubles_data() { generateData(0.0, 0.1, Qt::partial_ordering::greater); }
    void compareDoubles() { executeComparison<double, Qt::partial_ordering>(); }
    void comparePointers_data()
    { generateData(&values[0], &values[1], Qt::strong_ordering::equal); }
    void comparePointers() { executeComparison<const int *, Qt::strong_ordering>(); }
    void compareToNullptr_data() { generateData(ptr, &value, Qt::strong_ordering::equivalent); }
    void compareToNullptr() { executeComparison<const int *, Qt::strong_ordering>(); }
    void compareUnregisteredEnum_data()
    { generateData(MyUnregisteredEnumValue1,
                   MyUnregisteredEnumValue2, Qt::strong_ordering::less); }
    void compareUnregisteredEnum()
    { executeComparison<MyUnregisteredEnum, Qt::strong_ordering>(); }
    void compareRegisteredEnum_data()
    { generateData(Qt::Monday, Qt::Sunday, Qt::strong_ordering::greater); }
    void compareRegisteredEnum() { executeComparison<Qt::DayOfWeek, Qt::strong_ordering>(); }
    void compareCustomTypes_data() { generateData(val1, val2, Qt::strong_ordering::less); }
    void compareCustomTypes() { executeComparison<MyClass, Qt::strong_ordering>(); }
    void stdCompareInts_data() { generateData(1, -2, std::strong_ordering::equal); }
    void stdCompareInts() { (executeComparison<int, std::strong_ordering>()); }
    void stdCompareFloats_data()
    { generateData(2.0f, 1.1f, std::partial_ordering::less); }
    void stdCompareFloats() { (executeComparison<float, std::partial_ordering>()); }
    void stdCompareDoubles_data() { generateData(0.15, 0.25, std::partial_ordering::greater); }
    void stdCompareDoubles() { (executeComparison<double, std::partial_ordering>()); }
    void stdComparePointers_data()
    { generateData(&values[0], &values[1], std::strong_ordering::equal); }
    void stdComparePointers() { executeComparison<const int *, std::strong_ordering>(); }
    void stdCompareToNullptr_data()
    { generateData(ptr, &value, std::strong_ordering::equivalent); }
    void stdCompareToNullptr() { executeComparison<const int *, std::strong_ordering>(); }
    void stdCompareUnregisteredEnum_data()
    { generateData(MyUnregisteredEnumValue2,
                     MyUnregisteredEnumValue1, std::strong_ordering::less); }
    void stdCompareUnregisteredEnum()
    { executeComparison<MyUnregisteredEnum, std::strong_ordering>(); }
    void stdCompareRegisteredEnum_data()
    { generateData(Qt::Monday, Qt::Friday, std::strong_ordering::greater); }
    void stdCompareRegisteredEnum() { executeComparison<Qt::DayOfWeek, std::strong_ordering>(); }
    void stdCompareCustomTypes_data() { generateData(val1, val2, std::strong_ordering::less); }
    void stdCompareCustomTypes() { executeComparison<MyClass, std::strong_ordering>(); }

    void checkComparisonForTemporaryObjects();
    void checkWeakComparison();
};

template <typename OrderingType> inline QByteArray typeName()
{
#ifdef __cpp_lib_three_way_comparison
    if constexpr (std::is_same_v<OrderingType, std::strong_ordering>)
        return QByteArray("std::strong_ordering");
    if constexpr (std::is_same_v<OrderingType, std::weak_ordering>)
        return QByteArray("std::weak_ordering");
    if constexpr (std::is_same_v<OrderingType, std::partial_ordering>)
        return QByteArray("std::partial_ordering");
#endif

    if constexpr (std::is_same_v<OrderingType, Qt::strong_ordering>)
        return QByteArray("Qt::strong_ordering");
    if constexpr (std::is_same_v<OrderingType, Qt::partial_ordering>)
        return QByteArray("Qt::partial_ordering");
    if constexpr (std::is_same_v<OrderingType, Qt::weak_ordering>)
        return QByteArray("Qt::weak_ordering");
}

template <typename T, typename OrderingType>
void tst_ThreeWayCompare::generateData(T val1, T val2, OrderingType order)
{
#ifdef __cpp_lib_three_way_comparison
    QTest::addColumn<T>("lhs");
    QTest::addColumn<T>("rhs");
    QTest::addColumn<OrderingType>("expectedOrder");

    QTest::newRow(typeName<OrderingType>().append("::equivalent").constData())
            << val1 << val1 << order;
    QTest::newRow(typeName<OrderingType>().append("::less").constData())
            << val1 << val2 << order;
    QTest::newRow(typeName<OrderingType>().append("::greater").constData())
            << val2 << val1 << order;
#else
    Q_UNUSED(val1)
    Q_UNUSED(val2)
    Q_UNUSED(order)
#endif
}

template <typename T, typename OrderingType>
void tst_ThreeWayCompare::executeComparison()
{
#ifdef __cpp_lib_three_way_comparison
    QFETCH(T, lhs);
    QFETCH(T, rhs);
    QFETCH(OrderingType, expectedOrder);
    QCOMPARE_3WAY(lhs, rhs, expectedOrder);
#endif
}

void tst_ThreeWayCompare::checkComparisonForTemporaryObjects()
{
#ifdef __cpp_lib_three_way_comparison
    QCOMPARE_3WAY(getClassForValue(0).getValuePointer(),
                  getClassForValue(1).getValuePointer(),
                  Qt::strong_ordering::less);

    QCOMPARE_3WAY(getClassForValue(0).getValuePointer(),
                  getClassForValue(1).getValuePointer(),
                  std::strong_ordering::equal);
#endif
}

void tst_ThreeWayCompare::checkWeakComparison()
{
#ifdef __cpp_lib_three_way_comparison
    QUrl example_left("http://example.com");
    QUrl example_right("http://example.com:0");

    QCOMPARE_3WAY(example_left, example_right, std::weak_ordering::less);

    QDateTime june(QDate(2012, 6, 20), QTime(14, 33, 2, 500), QTimeZone::UTC);
    QDateTime juneLater = june.addMSecs(1);
    QCOMPARE_3WAY(june, juneLater, Qt::weak_ordering::greater);
#endif
}

QT_END_NAMESPACE

QTEST_MAIN(tst_ThreeWayCompare)
#include "tst_threewaycompare.moc"
