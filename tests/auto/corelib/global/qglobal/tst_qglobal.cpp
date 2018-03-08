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


#include <QtTest/QtTest>

#include <QPair>
#include <QTextCodec>
#include <QSysInfo>
#include <QLatin1String>

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
    void qAlignOf();
    void integerForSize();
    void qprintable();
    void qprintable_data();
    void buildAbiEndianness();
    void testqOverload();
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
    QVector<int> list;
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
    foreach (int i, QVector<int>(list)) {
        QCOMPARE(i, counter++);
    }
    QCOMPARE(counter, list.count());

    // check whether we can use a lambda
    counter = 0;
    foreach (int i, [&](){ return list; }()) {
        QCOMPARE(i, counter++);
    }
    QCOMPARE(counter, list.count());
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
    // Force compilation of these classes
    MyTrue tmp1;
    MyExpresion tmp2;
    MyTemplate tmp3;
    Q_UNUSED(tmp1);
    Q_UNUSED(tmp2);
    Q_UNUSED(tmp3);
#ifdef __COUNTER__
    // if the compiler supports __COUNTER__, multiple
    // Q_STATIC_ASSERT's on a single line should compile:
    Q_STATIC_ASSERT(true); Q_STATIC_ASSERT_X(!false, "");
#endif // __COUNTER__
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

#define TEST_AlignOf(type, alignment)                                       \
    do {                                                                    \
        TEST_AlignOf_impl(type, alignment);                                 \
                                                                            \
        TEST_AlignOf_impl(type &, alignment);                               \
        TEST_AlignOf_RValueRef(type &&, alignment);                         \
                                                                            \
        TEST_AlignOf_impl(type [5], alignment);                             \
        TEST_AlignOf_impl(type (&) [5], alignment);                         \
                                                                            \
        TEST_AlignOf_impl(AlignmentInStruct<type>, alignment);              \
                                                                            \
        /* Some internal sanity validation, just for fun */                 \
        TEST_AlignOf_impl(AlignmentInStruct<type [5]>, alignment);          \
        TEST_AlignOf_impl(AlignmentInStruct<type &>, Q_ALIGNOF(void *));    \
        TEST_AlignOf_impl(AlignmentInStruct<type (&) [5]>,                  \
                Q_ALIGNOF(void *));                                         \
        TEST_AlignOf_RValueRef(AlignmentInStruct<type &&>,                  \
                Q_ALIGNOF(void *));                                         \
    } while (false)                                                         \
    /**/

#ifdef Q_COMPILER_RVALUE_REFS
#define TEST_AlignOf_RValueRef(type, alignment) \
        TEST_AlignOf_impl(type, alignment)
#else
#define TEST_AlignOf_RValueRef(type, alignment) do {} while (false)
#endif

#define TEST_AlignOf_impl(type, alignment) \
    do { \
        QCOMPARE(Q_ALIGNOF(type), size_t(alignment)); \
        /* Compare to native operator for compilers that support it,
           otherwise...  erm... check consistency! :-) */ \
        QCOMPARE(QT_EMULATED_ALIGNOF(type), Q_ALIGNOF(type)); \
    } while (false)
    /**/

void tst_QGlobal::qAlignOf()
{
    // Built-in types, except 64-bit integers and double
    TEST_AlignOf(char, 1);
    TEST_AlignOf(signed char, 1);
    TEST_AlignOf(unsigned char, 1);
    TEST_AlignOf(qint8, 1);
    TEST_AlignOf(quint8, 1);
    TEST_AlignOf(qint16, 2);
    TEST_AlignOf(quint16, 2);
    TEST_AlignOf(qint32, 4);
    TEST_AlignOf(quint32, 4);
    TEST_AlignOf(void *, sizeof(void *));

    // Depends on platform and compiler, disabling test for now
    // TEST_AlignOf(long double, 16);

    // Empty struct
    TEST_AlignOf(Empty, 1);

    // Function pointers
    TEST_AlignOf(fun, Q_ALIGNOF(void *));
    TEST_AlignOf(memFun, Q_ALIGNOF(void *));


    // 64-bit integers and double
    TEST_AlignOf_impl(qint64, 8);
    TEST_AlignOf_impl(quint64, 8);
    TEST_AlignOf_impl(double, 8);

    TEST_AlignOf_impl(qint64 &, 8);
    TEST_AlignOf_impl(quint64 &, 8);
    TEST_AlignOf_impl(double &, 8);

    TEST_AlignOf_RValueRef(qint64 &&, 8);
    TEST_AlignOf_RValueRef(quint64 &&, 8);
    TEST_AlignOf_RValueRef(double &&, 8);

    // 32-bit x86 ABI idiosyncrasies
#if defined(Q_PROCESSOR_X86_32) && !defined(Q_OS_WIN)
    TEST_AlignOf_impl(AlignmentInStruct<qint64>, 4);
#else
    TEST_AlignOf_impl(AlignmentInStruct<qint64>, 8);
#endif

    TEST_AlignOf_impl(AlignmentInStruct<quint64>, Q_ALIGNOF(AlignmentInStruct<qint64>));
    TEST_AlignOf_impl(AlignmentInStruct<double>, Q_ALIGNOF(AlignmentInStruct<qint64>));

    // 32-bit x86 ABI, Clang disagrees with gcc
#if !defined(Q_PROCESSOR_X86_32) || !defined(Q_CC_CLANG)
    TEST_AlignOf_impl(qint64 [5],       Q_ALIGNOF(qint64));
#else
    TEST_AlignOf_impl(qint64 [5],       Q_ALIGNOF(AlignmentInStruct<qint64>));
#endif

    TEST_AlignOf_impl(qint64 (&) [5],   Q_ALIGNOF(qint64 [5]));
    TEST_AlignOf_impl(quint64 [5],      Q_ALIGNOF(quint64 [5]));
    TEST_AlignOf_impl(quint64 (&) [5],  Q_ALIGNOF(quint64 [5]));
    TEST_AlignOf_impl(double [5],       Q_ALIGNOF(double [5]));
    TEST_AlignOf_impl(double (&) [5],   Q_ALIGNOF(double [5]));
}

#undef TEST_AlignOf
#undef TEST_AlignOf_RValueRef
#undef TEST_AlignOf_impl

void tst_QGlobal::integerForSize()
{
    // compile-only test:
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<1>::Signed) == 1);
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<2>::Signed) == 2);
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<4>::Signed) == 4);
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<8>::Signed) == 8);

    Q_STATIC_ASSERT(sizeof(QIntegerForSize<1>::Unsigned) == 1);
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<2>::Unsigned) == 2);
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<4>::Unsigned) == 4);
    Q_STATIC_ASSERT(sizeof(QIntegerForSize<8>::Unsigned) == 8);
}

typedef QPair<const char *, const char *> stringpair;
Q_DECLARE_METATYPE(stringpair)

void tst_QGlobal::qprintable()
{
    QFETCH(QVector<stringpair>, localestrings);
    QFETCH(int, utf8index);

    QVERIFY(utf8index >= 0 && utf8index < localestrings.count());
    if (utf8index < 0 || utf8index >= localestrings.count())
        return;

    const char *const utf8string = localestrings.at(utf8index).second;

    QString string = QString::fromUtf8(utf8string);

    for (const stringpair &pair : qAsConst(localestrings)) {
        QTextCodec *codec = QTextCodec::codecForName(pair.first);
        if (!codec)
            continue;
        QTextCodec::setCodecForLocale(codec);
        // test qPrintable()
        QVERIFY(qstrcmp(qPrintable(string), pair.second) == 0);
        for (const stringpair &pair2 : qAsConst(localestrings)) {
            if (pair2.second == pair.second)
                continue;
            QVERIFY(qstrcmp(qPrintable(string), pair2.second) != 0);
        }
        // test qUtf8Printable()
        QVERIFY(qstrcmp(qUtf8Printable(string), utf8string) == 0);
        for (const stringpair &pair2 : qAsConst(localestrings)) {
            if (qstrcmp(pair2.second, utf8string) == 0)
                continue;
            QVERIFY(qstrcmp(qUtf8Printable(string), pair2.second) != 0);
        }
    }

    QTextCodec::setCodecForLocale(0);
}

void tst_QGlobal::qprintable_data()
{
    QTest::addColumn<QVector<stringpair> >("localestrings");
    QTest::addColumn<int>("utf8index"); // index of utf8 string

    // Unicode: HIRAGANA LETTER A, I, U, E, O (U+3442, U+3444, U+3446, U+3448, U+344a)
    static const char *const utf8string = "\xe3\x81\x82\xe3\x81\x84\xe3\x81\x86\xe3\x81\x88\xe3\x81\x8a";
    static const char *const eucjpstring = "\xa4\xa2\xa4\xa4\xa4\xa6\xa4\xa8\xa4\xaa";
    static const char *const sjisstring = "\x82\xa0\x82\xa2\x82\xa4\x82\xa6\x82\xa8";

    QVector<stringpair> japanesestrings;
    japanesestrings << stringpair("UTF-8", utf8string)
                    << stringpair("EUC-JP", eucjpstring)
                    << stringpair("Shift_JIS", sjisstring);

    QTest::newRow("Japanese") << japanesestrings << 0;

}

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


QTEST_APPLESS_MAIN(tst_QGlobal)
#include "tst_qglobal.moc"
