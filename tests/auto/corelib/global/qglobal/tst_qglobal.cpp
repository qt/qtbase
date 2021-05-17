/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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


#include <QTest>

#include <QPair>
#include <QSysInfo>
#include <QLatin1String>
#include <QString>

#include <cmath>

class tst_QGlobal: public QObject
{
    Q_OBJECT

private slots:
    void cMode();
    void qIsNull();
    void for_each();
    void qassert();
    void qtry();
    void checkptr();
    void qstaticassert();
    void qConstructorFunction();
    void qCoreAppStartupFunction();
    void qCoreAppStartupFunctionRestart();
    void integerForSize();
    void buildAbiEndianness();
    void testqOverload();
    void testqMinMax();
    void qRoundFloats_data();
    void qRoundFloats();
    void qRoundDoubles_data();
    void qRoundDoubles();
    void PRImacros();
    void testqToUnderlying();
};

extern "C" {        // functions in qglobal.c
void tst_GlobalTypes();
int tst_QtVersion();
const char *tst_qVersion();
}

void tst_QGlobal::cMode()
{
    tst_GlobalTypes();
    QCOMPARE(tst_QtVersion(), QT_VERSION);

#ifndef QT_NAMESPACE
    QCOMPARE(tst_qVersion(), qVersion());
#endif
}

void tst_QGlobal::qIsNull()
{
    double d = 0.0;
    float f = 0.0f;

    QVERIFY(::qIsNull(d));
    QVERIFY(::qIsNull(f));

    d += 0.000000001;
    f += 0.0000001f;

    QVERIFY(!::qIsNull(d));
    QVERIFY(!::qIsNull(f));

    d = -0.0;
    f = -0.0f;

    QVERIFY(::qIsNull(d));
    QVERIFY(::qIsNull(f));
}

void tst_QGlobal::for_each()
{
    QList<int> list;
    list << 0 << 1 << 2 << 3 << 4 << 5;

    int counter = 0;
    foreach(int i, list) {
        QCOMPARE(i, counter++);
    }
    QCOMPARE(counter, list.count());

    // do it again, to make sure we don't have any for-scoping
    // problems with older compilers
    counter = 0;
    foreach(int i, list) {
        QCOMPARE(i, counter++);
    }
    QCOMPARE(counter, list.count());

    // check whether we can pass a constructor as container argument
    counter = 0;
    foreach (int i, QList<int>(list)) {
        QCOMPARE(i, counter++);
    }
    QCOMPARE(counter, list.count());

    // check whether we can use a lambda
    counter = 0;
    foreach (int i, [&](){ return list; }()) {
        QCOMPARE(i, counter++);
    }
    QCOMPARE(counter, list.count());

    // Should also work with an existing variable
    int local;
    counter = 0;
    foreach (local, list) {
        QCOMPARE(local, counter++);
    }
    QCOMPARE(counter, list.count());
    QCOMPARE(local, counter - 1);

    // Test the macro does not mess if/else conditions
    counter = 0;
    if (true)
        foreach (int i, list)
            QCOMPARE(i, counter++);
    else
        QFAIL("If/Else mismatch");
    QCOMPARE(counter, list.count());

    counter = 0;
    if (false)
        foreach (int i, list)
            if (i) QFAIL("If/Else mismatch");
            else QFAIL("If/Else mismatch");
    else
        foreach (int i, list)
            if (false) { }
            else QCOMPARE(i, counter++);
    QCOMPARE(counter, list.count());

    // break and continue
    counter = 0;
    foreach (int i, list) {
        if (i == 0)
            continue;
        QCOMPARE(i, (counter++) + 1);
        if (i == 3)
            break;
    }
    QCOMPARE(counter, 3);
}

void tst_QGlobal::qassert()
{
    bool passed = false;
    if (false) {
        Q_ASSERT(false);
    } else {
        passed = true;
    }
    QVERIFY(passed);

    passed = false;
    if (false) {
        Q_ASSERT_X(false, "tst_QGlobal", "qassert");
    } else {
        passed = true;
    }
    QVERIFY(passed);

    passed = false;
    if (false)
        Q_ASSERT(false);
    else
        passed = true;
    QVERIFY(passed);

    passed = false;
    if (false)
        Q_ASSERT_X(false, "tst_QGlobal", "qassert");
    else
        passed = true;
    QVERIFY(passed);
}

void tst_QGlobal::qtry()
{
    int i = 0;
    QT_TRY {
        i = 1;
        QT_THROW(42);
        i = 2;
    } QT_CATCH(int) {
        QCOMPARE(i, 1);
        i = 7;
    }
#ifdef QT_NO_EXCEPTIONS
    QCOMPARE(i, 2);
#else
    QCOMPARE(i, 7);
#endif

    // check propper if/else scoping
    i = 0;
    if (true) {
        QT_TRY {
            i = 2;
            QT_THROW(42);
            i = 4;
        } QT_CATCH(int) {
            QCOMPARE(i, 2);
            i = 4;
        }
    } else {
        QCOMPARE(i, 0);
    }
    QCOMPARE(i, 4);

    i = 0;
    if (false) {
        QT_TRY {
            i = 2;
            QT_THROW(42);
            i = 4;
        } QT_CATCH(int) {
            QCOMPARE(i, 2);
            i = 2;
        }
    } else {
        i = 8;
    }
    QCOMPARE(i, 8);

    i = 0;
    if (false) {
        i = 42;
    } else {
        QT_TRY {
            i = 2;
            QT_THROW(42);
            i = 4;
        } QT_CATCH(int) {
            QCOMPARE(i, 2);
            i = 4;
        }
    }
    QCOMPARE(i, 4);
}

void tst_QGlobal::checkptr()
{
    int i;
    QCOMPARE(q_check_ptr(&i), &i);

    const char *c = "hello";
    QCOMPARE(q_check_ptr(c), c);
}

// Check Q_STATIC_ASSERT, It should compile
// note that, we are not able to test Q_STATIC_ASSERT(false), to do it manually someone has
// to replace expressions (in the asserts) one by one to false, and check if it breaks build.
class MyTrue
{
public:
    MyTrue()
    {
        Q_STATIC_ASSERT(true);
        Q_STATIC_ASSERT(!false);
        Q_STATIC_ASSERT_X(true,"");
        Q_STATIC_ASSERT_X(!false,"");
    }
    ~MyTrue()
    {
        Q_STATIC_ASSERT(true);
        Q_STATIC_ASSERT(!false);
        Q_STATIC_ASSERT_X(true,"");
        Q_STATIC_ASSERT_X(!false,"");
    }
    Q_STATIC_ASSERT(true);
    Q_STATIC_ASSERT(!false);
    Q_STATIC_ASSERT_X(true,"");
    Q_STATIC_ASSERT_X(!false,"");
};

struct MyExpresion
{
    void foo()
    {
        Q_STATIC_ASSERT(sizeof(MyTrue) > 0);
        Q_STATIC_ASSERT(sizeof(MyTrue) > 0);
        Q_STATIC_ASSERT_X(sizeof(MyTrue) > 0,"");
        Q_STATIC_ASSERT_X(sizeof(MyTrue) > 0,"");
    }
private:
    Q_STATIC_ASSERT(sizeof(MyTrue) > 0);
    Q_STATIC_ASSERT(sizeof(MyTrue) > 0);
    Q_STATIC_ASSERT_X(sizeof(MyTrue) > 0, "");
    Q_STATIC_ASSERT_X(sizeof(MyTrue) > 0, "");
};

struct TypeDef
{
    typedef int T;
    Q_STATIC_ASSERT(sizeof(T));
    Q_STATIC_ASSERT_X(sizeof(T), "");
};

template<typename T1, typename T2>
struct Template
{
    static const bool True = true;
    typedef typename T1::T DependentType;
    Q_STATIC_ASSERT(True);
    Q_STATIC_ASSERT(!!True);
    Q_STATIC_ASSERT(sizeof(DependentType));
    Q_STATIC_ASSERT(!!sizeof(DependentType));
    Q_STATIC_ASSERT_X(True, "");
    Q_STATIC_ASSERT_X(!!True, "");
    Q_STATIC_ASSERT_X(sizeof(DependentType), "");
    Q_STATIC_ASSERT_X(!!sizeof(DependentType), "");
};

struct MyTemplate
{
    static const bool Value = Template<TypeDef, int>::True;
    Q_STATIC_ASSERT(Value);
    Q_STATIC_ASSERT(!!Value);
    Q_STATIC_ASSERT_X(Value, "");
    Q_STATIC_ASSERT_X(!!Value, "");
};

void tst_QGlobal::qstaticassert()
{
    // Test multiple Q_STATIC_ASSERT on a single line
    Q_STATIC_ASSERT(true); Q_STATIC_ASSERT_X(!false, "");

    // Force compilation of these classes
    MyTrue tmp1;
    MyExpresion tmp2;
    MyTemplate tmp3;
    Q_UNUSED(tmp1);
    Q_UNUSED(tmp2);
    Q_UNUSED(tmp3);
    QVERIFY(true); // if the test compiles it has passed.
}

static int qConstructorFunctionValue;
static void qConstructorFunctionCtor()
{
    qConstructorFunctionValue = 123;
}
Q_CONSTRUCTOR_FUNCTION(qConstructorFunctionCtor);

void tst_QGlobal::qConstructorFunction()
{
    QCOMPARE(qConstructorFunctionValue, 123);
}

static int qStartupFunctionValue;
static void myStartupFunc()
{
   Q_ASSERT(QCoreApplication::instance());
   if (QCoreApplication::instance())
       qStartupFunctionValue += 124;
}

Q_COREAPP_STARTUP_FUNCTION(myStartupFunc)

void tst_QGlobal::qCoreAppStartupFunction()
{
    QCOMPARE(qStartupFunctionValue, 0);
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    QCoreApplication app(argc, argv);
    QCOMPARE(qStartupFunctionValue, 124);
}

void tst_QGlobal::qCoreAppStartupFunctionRestart()
{
    qStartupFunctionValue = 0;
    qCoreAppStartupFunction();
    qStartupFunctionValue = 0;
    qCoreAppStartupFunction();
}

struct isEnum_A {
    int n_;
};

enum isEnum_B_Byte { isEnum_B_Byte_x = 63 };
enum isEnum_B_Short { isEnum_B_Short_x = 1024 };
enum isEnum_B_Int { isEnum_B_Int_x = 1 << 20 };

union isEnum_C {};

class isEnum_D {
public:
    operator int() const;
};

class isEnum_E {
private:
    operator int() const;
};

class isEnum_F {
public:
    enum AnEnum {};
};

struct Empty {};
template <class T> struct AlignmentInStruct { T dummy; };

typedef int (*fun) ();
typedef int (Empty::*memFun) ();

void tst_QGlobal::integerForSize()
{
    // compile-only test:
    static_assert(sizeof(QIntegerForSize<1>::Signed) == 1);
    static_assert(sizeof(QIntegerForSize<2>::Signed) == 2);
    static_assert(sizeof(QIntegerForSize<4>::Signed) == 4);
    static_assert(sizeof(QIntegerForSize<8>::Signed) == 8);

    static_assert(sizeof(QIntegerForSize<1>::Unsigned) == 1);
    static_assert(sizeof(QIntegerForSize<2>::Unsigned) == 2);
    static_assert(sizeof(QIntegerForSize<4>::Unsigned) == 4);
    static_assert(sizeof(QIntegerForSize<8>::Unsigned) == 8);
}

typedef QPair<const char *, const char *> stringpair;
Q_DECLARE_METATYPE(stringpair)

void tst_QGlobal::buildAbiEndianness()
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    QLatin1String endian("little_endian");
#elif Q_BYTE_ORDER == Q_BIG_ENDIAN
    QLatin1String endian("big_endian");
#endif
    QVERIFY(QSysInfo::buildAbi().contains(endian));
}

struct Overloaded
{
    void foo() {}
    void foo(QByteArray) {}
    void foo(QByteArray, const QString &) {}

    void constFoo() const {}
    void constFoo(QByteArray) const {}
    void constFoo(QByteArray, const QString &) const {}

    void mixedFoo() {}
    void mixedFoo(QByteArray) const {}
};

void freeOverloaded() {}
void freeOverloaded(QByteArray) {}
void freeOverloaded(QByteArray, const QString &) {}

void freeOverloadedGet(QByteArray) {}
QByteArray freeOverloadedGet() { return QByteArray(); }


void tst_QGlobal::testqOverload()
{
#ifdef Q_COMPILER_VARIADIC_TEMPLATES

    // void returning free overloaded functions
    QVERIFY(QOverload<>::of(&freeOverloaded) ==
             static_cast<void (*)()>(&freeOverloaded));

    QVERIFY(QOverload<QByteArray>::of(&freeOverloaded) ==
             static_cast<void (*)(QByteArray)>(&freeOverloaded));

    QVERIFY((QOverload<QByteArray, const QString &>::of(&freeOverloaded)) ==
             static_cast<void (*)(QByteArray, const QString &)>(&freeOverloaded));

    // value returning free overloaded functions
    QVERIFY(QOverload<>::of(&freeOverloadedGet) ==
             static_cast<QByteArray (*)()>(&freeOverloadedGet));

    QVERIFY(QOverload<QByteArray>::of(&freeOverloadedGet) ==
             static_cast<void (*)(QByteArray)>(&freeOverloadedGet));

    // void returning overloaded member functions
    QVERIFY(QOverload<>::of(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::foo));

    QVERIFY(QOverload<QByteArray>::of(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)(QByteArray)>(&Overloaded::foo));

    QVERIFY((QOverload<QByteArray, const QString &>::of(&Overloaded::foo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &)>(&Overloaded::foo));

    // void returning overloaded const member functions
    QVERIFY(QOverload<>::of(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)() const>(&Overloaded::constFoo));

    QVERIFY(QOverload<QByteArray>::of(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::constFoo));

    QVERIFY((QOverload<QByteArray, const QString &>::of(&Overloaded::constFoo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &) const>(&Overloaded::constFoo));

    // void returning overloaded const AND non-const member functions
    QVERIFY(QNonConstOverload<>::of(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::mixedFoo));

    QVERIFY(QConstOverload<QByteArray>::of(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::mixedFoo));

#if defined(__cpp_variable_templates) && __cpp_variable_templates >= 201304 // C++14

    // void returning free overloaded functions
    QVERIFY(qOverload<>(&freeOverloaded) ==
             static_cast<void (*)()>(&freeOverloaded));

    QVERIFY(qOverload<QByteArray>(&freeOverloaded) ==
             static_cast<void (*)(QByteArray)>(&freeOverloaded));

    QVERIFY((qOverload<QByteArray, const QString &>(&freeOverloaded) ==
             static_cast<void (*)(QByteArray, const QString &)>(&freeOverloaded)));

    // value returning free overloaded functions
    QVERIFY(qOverload<>(&freeOverloadedGet) ==
             static_cast<QByteArray (*)()>(&freeOverloadedGet));

    QVERIFY(qOverload<QByteArray>(&freeOverloadedGet) ==
             static_cast<void (*)(QByteArray)>(&freeOverloadedGet));

    // void returning overloaded member functions
    QVERIFY(qOverload<>(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::foo));

    QVERIFY(qOverload<QByteArray>(&Overloaded::foo) ==
             static_cast<void (Overloaded::*)(QByteArray)>(&Overloaded::foo));

    QVERIFY((qOverload<QByteArray, const QString &>(&Overloaded::foo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &)>(&Overloaded::foo));

    // void returning overloaded const member functions
    QVERIFY(qOverload<>(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)() const>(&Overloaded::constFoo));

    QVERIFY(qOverload<QByteArray>(&Overloaded::constFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::constFoo));

    QVERIFY((qOverload<QByteArray, const QString &>(&Overloaded::constFoo)) ==
             static_cast<void (Overloaded::*)(QByteArray, const QString &) const>(&Overloaded::constFoo));

    // void returning overloaded const AND non-const member functions
    QVERIFY(qNonConstOverload<>(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)()>(&Overloaded::mixedFoo));

    QVERIFY(qConstOverload<QByteArray>(&Overloaded::mixedFoo) ==
             static_cast<void (Overloaded::*)(QByteArray) const>(&Overloaded::mixedFoo));
#endif

#endif
}

// enforce that types are identical when comparing
template<typename T>
void compare(T a, T b)
{ QCOMPARE(a, b); }

void tst_QGlobal::testqMinMax()
{
    // signed types
    compare(qMin(float(1), double(-1)), double(-1));
    compare(qMin(double(1), float(-1)), double(-1));
    compare(qMin(short(1), int(-1)), int(-1));
    compare(qMin(short(1), long(-1)), long(-1));
    compare(qMin(qint64(1), short(-1)), qint64(-1));

    compare(qMax(float(1), double(-1)), double(1));
    compare(qMax(short(1), long(-1)), long(1));
    compare(qMax(qint64(1), short(-1)), qint64(1));

    // unsigned types
    compare(qMin(ushort(1), ulong(2)), ulong(1));
    compare(qMin(quint64(1), ushort(2)), quint64(1));

    compare(qMax(ushort(1), ulong(2)), ulong(2));
    compare(qMax(quint64(1), ushort(2)), quint64(2));
}

void tst_QGlobal::qRoundFloats_data()
{
    QTest::addColumn<float>("actual");
    QTest::addColumn<float>("expected");

    QTest::newRow("round half") << 0.5f << 1.0f;
    QTest::newRow("round negative half") << -0.5f << -1.0f;
    QTest::newRow("round negative") << -1.4f << -1.0f;
    QTest::newRow("round largest representable float less than 0.5") << std::nextafter(0.5f, 0.0f) << 0.0f;
}

void tst_QGlobal::qRoundFloats() {
    QFETCH(float, actual);
    QFETCH(float, expected);

#if !(defined(Q_PROCESSOR_ARM_64) && (__has_builtin(__builtin_round) || defined(Q_CC_GNU)) && !defined(Q_CC_CLANG))
    QEXPECT_FAIL("round largest representable float less than 0.5",
                 "We know qRound fails in this case, but decided that we value simplicity over correctness",
                 Continue);
#endif
    QCOMPARE(qRound(actual), expected);

#if !(defined(Q_PROCESSOR_ARM_64) && (__has_builtin(__builtin_round) || defined(Q_CC_GNU)) && !defined(Q_CC_CLANG))
    QEXPECT_FAIL("round largest representable float less than 0.5",
                 "We know qRound fails in this case, but decided that we value simplicity over correctness",
                 Continue);
#endif
    QCOMPARE(qRound64(actual), expected);
}

void tst_QGlobal::qRoundDoubles_data() {
    QTest::addColumn<double>("actual");
    QTest::addColumn<double>("expected");

    QTest::newRow("round half") << 0.5 << 1.0;
    QTest::newRow("round negative half") << -0.5 << -1.0;
    QTest::newRow("round negative") << -1.4 << -1.0;
    QTest::newRow("round largest representable double less than 0.5") << std::nextafter(0.5, 0.0) << 0.0;
}

void tst_QGlobal::qRoundDoubles() {
    QFETCH(double, actual);
    QFETCH(double, expected);

#if !(defined(Q_PROCESSOR_ARM_64) && (__has_builtin(__builtin_round) || defined(Q_CC_GNU)) && !defined(Q_CC_CLANG))
    QEXPECT_FAIL("round largest representable double less than 0.5",
                 "We know qRound fails in this case, but decided that we value simplicity over correctness",
                 Continue);
#endif
    QCOMPARE(qRound(actual), expected);

#if !(defined(Q_PROCESSOR_ARM_64) && (__has_builtin(__builtin_round) || defined(Q_CC_GNU)) && !defined(Q_CC_CLANG))
    QEXPECT_FAIL("round largest representable double less than 0.5",
                 "We know qRound fails in this case, but decided that we value simplicity over correctness",
                 Continue);
#endif
    QCOMPARE(qRound64(actual), expected);
}

void tst_QGlobal::PRImacros()
{
    // none of these calls must generate a -Wformat warning
    {
        quintptr p = 123u;
        QCOMPARE(QString::asprintf("The value %" PRIuQUINTPTR " is nice", p), "The value 123 is nice");
        QCOMPARE(QString::asprintf("The value %" PRIoQUINTPTR " is nice", p), "The value 173 is nice");
        QCOMPARE(QString::asprintf("The value %" PRIxQUINTPTR " is nice", p), "The value 7b is nice");
        QCOMPARE(QString::asprintf("The value %" PRIXQUINTPTR " is nice", p), "The value 7B is nice");
    }

    {
        qintptr p = 123;
        QCOMPARE(QString::asprintf("The value %" PRIdQINTPTR " is nice", p), "The value 123 is nice");
        QCOMPARE(QString::asprintf("The value %" PRIiQINTPTR " is nice", p), "The value 123 is nice");
    }

    {
        qptrdiff d = 123;
        QCOMPARE(QString::asprintf("The value %" PRIdQPTRDIFF " is nice", d), "The value 123 is nice");
        QCOMPARE(QString::asprintf("The value %" PRIiQPTRDIFF " is nice", d), "The value 123 is nice");
    }
    {
        qsizetype s = 123;
        QCOMPARE(QString::asprintf("The value %" PRIdQSIZETYPE " is nice", s), "The value 123 is nice");
        QCOMPARE(QString::asprintf("The value %" PRIiQSIZETYPE " is nice", s), "The value 123 is nice");
    }
}

void tst_QGlobal::testqToUnderlying()
{
    enum class E {
        E1 = 123,
        E2 = 456,
    };
    static_assert(std::is_same_v<decltype(qToUnderlying(E::E1)), int>);
    QCOMPARE(qToUnderlying(E::E1), 123);
    QCOMPARE(qToUnderlying(E::E2), 456);

    enum EE : unsigned long {
        EE1 = 123,
        EE2 = 456,
    };
    static_assert(std::is_same_v<decltype(qToUnderlying(EE1)), unsigned long>);
    QCOMPARE(qToUnderlying(EE1), 123UL);
    QCOMPARE(qToUnderlying(EE2), 456UL);
}

QTEST_APPLESS_MAIN(tst_QGlobal)
#include "tst_qglobal.moc"
