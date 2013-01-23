/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QtCore>
#include <QtTest/QtTest>

#include <algorithm>

#define BASECLASS_NOT_ABSTRACT
#include "baseclass.h"
#include "derivedclass.h"

QT_USE_NAMESPACE

class tst_Compiler : public QObject
{
Q_OBJECT
private slots:
    void template_methods();
    void template_constructors();
    void template_subclasses();
    void methodSpecialization();
    void constructorSpecialization();
    void staticTemplateMethods();
    void staticTemplateMethodSpecialization();
    void detectDataStream();
    void detectEnums();
    void overrideCFunction();
    void stdSortQList();
    void stdSortQVector();
    void templateCallOrder();
    void virtualFunctionNoLongerPureVirtual();
    void charSignedness() const;
    void privateStaticTemplateMember() const;
    void staticConstUnionWithInitializerList() const;
};

#if defined(Q_CC_HPACC)
# define DONT_TEST_TEMPLATE_CONSTRUCTORS
# define DONT_TEST_CONSTRUCTOR_SPECIALIZATION
# define DONT_TEST_DATASTREAM_DETECTION
#endif

#if defined(Q_CC_SUN)
# define DONT_TEST_STL_SORTING
#endif

class TemplateMethodClass
{
public:
    template <class T>
    T foo() { return 42; }
};

void tst_Compiler::template_methods()
{
    TemplateMethodClass t;

    QCOMPARE(t.foo<int>(), 42);
    QCOMPARE(t.foo<long>(), 42l);
    QCOMPARE(t.foo<double>(), 42.0);
}

#ifndef DONT_TEST_TEMPLATE_CONSTRUCTORS
class TemplateConstructorClass
{
public:
    template <class T>
    TemplateConstructorClass(const T& t) { i = int(t); }

    int i;
};

void tst_Compiler::template_constructors()
{
    TemplateConstructorClass t1(42);
    TemplateConstructorClass t2(42l);
    TemplateConstructorClass t3(42.0);

    QCOMPARE(t1.i, 42);
    QCOMPARE(t2.i, 42);
    QCOMPARE(t3.i, 42);
}
#else
void tst_Compiler::template_constructors()
{ QSKIP("Compiler doesn't do template constructors"); }
#endif

template <typename T>
struct OuterClass
{
    template <typename U>
    struct InnerClass
    {
        U convert(const T &t) { return static_cast<U>(t); }
    };
};

void tst_Compiler::template_subclasses()
{
    OuterClass<char>::InnerClass<int> c1;
    QCOMPARE(c1.convert('a'), int('a'));

    OuterClass<QRect>::InnerClass<QRectF> c2;
    QCOMPARE(c2.convert(QRect(1, 2, 3, 4)), QRectF(QRect(1, 2, 3, 4)));
}

class TemplateMethodClass2
{
public:
    template <class T>
    T foo() { return 42; }
};

template<>
int TemplateMethodClass2::foo<int>()
{ return 43; }

void tst_Compiler::methodSpecialization()
{
    TemplateMethodClass2 t;

    QCOMPARE(t.foo<int>(), 43);
    QCOMPARE(t.foo<long>(), 42l);
    QCOMPARE(t.foo<double>(), 42.0);
}

#ifndef DONT_TEST_CONSTRUCTOR_SPECIALIZATION
class TemplateConstructorClass2
{
public:
    template <class T>
    TemplateConstructorClass2(const T &t) { i = int(t); }

    int i;
};

template<>
TemplateConstructorClass2::TemplateConstructorClass2(const int &t) { i = t + 1; }

void tst_Compiler::constructorSpecialization()
{
    TemplateConstructorClass2 t1(42);
    TemplateConstructorClass2 t2(42l);
    TemplateConstructorClass2 t3(42.0);

    QCOMPARE(t1.i, 43);
    QCOMPARE(t2.i, 42);
    QCOMPARE(t3.i, 42);
}
#else
void tst_Compiler::constructorSpecialization()
{ QSKIP("Compiler doesn't do constructor specialization"); }
#endif

class StaticTemplateClass
{
public:
    template <class T>
    static T foo() { return 42; }
};

void tst_Compiler::staticTemplateMethods()
{
    QCOMPARE(StaticTemplateClass::foo<int>(), 42);
    QCOMPARE(StaticTemplateClass::foo<uint>(), 42u);
}

class StaticTemplateClass2
{
public:
    template <class T>
    static T foo() { return 42; }
};

template<>
double StaticTemplateClass2::foo<double>() { return 18.5; }

void tst_Compiler::staticTemplateMethodSpecialization()
{
    QCOMPARE(StaticTemplateClass2::foo<int>(), 42);
    QCOMPARE(StaticTemplateClass2::foo<uint>(), 42u);
    QCOMPARE(StaticTemplateClass2::foo<double>(), 18.5);
}

#ifndef DONT_TEST_DATASTREAM_DETECTION
/******* DataStream tester *********/
namespace QtTestInternal
{
    struct EmptyStruct {};
    struct LowPreferenceStruct { LowPreferenceStruct(...); };

    EmptyStruct operator<<(QDataStream &, const LowPreferenceStruct &);
    EmptyStruct operator>>(QDataStream &, const LowPreferenceStruct &);

    template<typename T>
    struct DataStreamChecker
    {
        static EmptyStruct hasStreamHelper(const EmptyStruct &);
        static QDataStream hasStreamHelper(const QDataStream &);
        static QDataStream &dsDummy();
        static T &dummy();

#ifdef BROKEN_COMPILER
        static const bool HasDataStream =
            sizeof(hasStreamHelper(dsDummy() << dummy())) == sizeof(QDataStream)
            && sizeof(hasStreamHelper(dsDummy() >> dummy())) == sizeof(QDataStream);
#else
        enum {
            HasOutDataStream = sizeof(hasStreamHelper(dsDummy() >> dummy())) == sizeof(QDataStream),
            HasInDataStream = sizeof(hasStreamHelper(dsDummy() << dummy())) == sizeof(QDataStream),
            HasDataStream = HasOutDataStream & HasInDataStream
        };
#endif
    };

    template<bool>
    struct DataStreamOpHelper
    {
        template <typename T>
        struct Getter {
            static QMetaType::SaveOperator saveOp() { return 0; }
        };
    };

    template<>
    struct DataStreamOpHelper<true>
    {
        template <typename T>
        struct Getter {
            static QMetaType::SaveOperator saveOp()
            {
                return ::QtMetaTypePrivate::QMetaTypeFunctionHelper<T>::Save;
            }
        };

    };

    template<typename T>
    inline QMetaType::SaveOperator getSaveOperator(T * = 0)
    {
        typedef typename DataStreamOpHelper<DataStreamChecker<T>::HasDataStream>::template Getter<T> GetterHelper;
        return GetterHelper::saveOp();
    }
};

struct MyString: public QString {};
struct Qxxx {};

void tst_Compiler::detectDataStream()
{
    QVERIFY(QtTestInternal::DataStreamChecker<int>::HasDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<uint>::HasDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<char *>::HasDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<const int>::HasInDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<const int>::HasOutDataStream == false);
    QVERIFY(QtTestInternal::DataStreamChecker<const int>::HasDataStream == false);
    QVERIFY(QtTestInternal::DataStreamChecker<double>::HasDataStream == true);

    QVERIFY(QtTestInternal::DataStreamChecker<QString>::HasDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<MyString>::HasDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<Qxxx>::HasDataStream == false);

    QVERIFY(QtTestInternal::getSaveOperator<int>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<uint>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<char *>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<double>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<QString>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<MyString>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<Qxxx>() == 0);
}
#else
void tst_Compiler::detectDataStream()
{ QSKIP("Compiler doesn't evaluate templates correctly"); }
#endif

enum Enum1 { Foo = 0, Bar = 1 };
enum Enum2 {};
enum Enum3 { Something = 1 };

template <typename T> char QTypeInfoEnumHelper(T);
template <typename T> void *QTypeInfoEnumHelper(...);

template <typename T>
struct QTestTypeInfo
{
    enum { IsEnum = sizeof(QTypeInfoEnumHelper<T>(0)) == sizeof(void*) };
};

void tst_Compiler::detectEnums()
{
    QVERIFY(QTestTypeInfo<Enum1>::IsEnum);
    QVERIFY(QTestTypeInfo<Enum2>::IsEnum);
    QVERIFY(QTestTypeInfo<Enum3>::IsEnum);
    QVERIFY(!QTestTypeInfo<int>::IsEnum);
    QVERIFY(!QTestTypeInfo<char>::IsEnum);
    QVERIFY(!QTestTypeInfo<uint>::IsEnum);
    QVERIFY(!QTestTypeInfo<short>::IsEnum);
    QVERIFY(!QTestTypeInfo<ushort>::IsEnum);
    QVERIFY(!QTestTypeInfo<void*>::IsEnum);
    QVERIFY(!QTestTypeInfo<QString>::IsEnum);
    QVERIFY(QTestTypeInfo<Qt::Key>::IsEnum);
    QVERIFY(QTestTypeInfo<Qt::ToolBarArea>::IsEnum);
    QVERIFY(!QTestTypeInfo<Qt::ToolBarAreas>::IsEnum);
    QVERIFY(QTestTypeInfo<Qt::MatchFlag>::IsEnum);
    QVERIFY(!QTestTypeInfo<Qt::MatchFlags>::IsEnum);
}
static int indicator = 0;

// this is a silly C function
extern "C" {
    void someCFunc(void *) { indicator = 42; }
}

// this is the catch-template that will be called if the C function doesn't exist
template <typename T>
void someCFunc(T *) { indicator = 10; }

void tst_Compiler::overrideCFunction()
{
    someCFunc((void*)0);
    QCOMPARE(indicator, 42);
}

#ifndef DONT_TEST_STL_SORTING
void tst_Compiler::stdSortQList()
{
    QList<int> list;
    list << 4 << 2;
    std::sort(list.begin(), list.end());
    QCOMPARE(list.value(0), 2);
    QCOMPARE(list.value(1), 4);

    QList<QString> slist;
    slist << "b" << "a";
    std::sort(slist.begin(), slist.end());
    QCOMPARE(slist.value(0), QString("a"));
    QCOMPARE(slist.value(1), QString("b"));
}

void tst_Compiler::stdSortQVector()
{
    QVector<int> vector;
    vector << 4 << 2;
    std::sort(vector.begin(), vector.end());
    QCOMPARE(vector.value(0), 2);
    QCOMPARE(vector.value(1), 4);

    QVector<QString> strvec;
    strvec << "b" << "a";
    std::sort(strvec.begin(), strvec.end());
    QCOMPARE(strvec.value(0), QString("a"));
    QCOMPARE(strvec.value(1), QString("b"));
}
#else
void tst_Compiler::stdSortQList()
{ QSKIP("Compiler's STL broken"); }
void tst_Compiler::stdSortQVector()
{ QSKIP("Compiler's STL broken"); }
#endif

// the C func will set it to 1, the template to 2
static int whatWasCalled = 0;

void callOrderFunc(void *)
{
    whatWasCalled = 1;
}

template <typename T>
void callOrderFunc(T *)
{
    whatWasCalled = 2;
}

template <typename T>
void callOrderNoCFunc(T *)
{
    whatWasCalled = 3;
}

/*
   This test will check what will get precendence - the C function
   or the template.

   It also makes sure this template "override" will compile on all systems
   and not result in ambiguities.
*/
void tst_Compiler::templateCallOrder()
{
    QCOMPARE(whatWasCalled, 0);

    // call it with a void *
    void *f = 0;
    callOrderFunc(f);
    QCOMPARE(whatWasCalled, 1);
    whatWasCalled = 0;

    char *c = 0;
    /* call it with a char * - AMBIGOUS, fails on several compilers
    callOrderFunc(c);
    QCOMPARE(whatWasCalled, 1);
    whatWasCalled = 0;
    */

    // now try the case when there is no C function
    callOrderNoCFunc(f);
    QCOMPARE(whatWasCalled, 3);
    whatWasCalled = 0;

    callOrderNoCFunc(c);
    QCOMPARE(whatWasCalled, 3);
    whatWasCalled = 0;
}

// test to see if removing =0 from a pure virtual function is BC
void tst_Compiler::virtualFunctionNoLongerPureVirtual()
{
#ifdef BASECLASS_NOT_ABSTRACT
    // has a single virtual function, not pure virtual, can call it
    BaseClass baseClass;
    QTest::ignoreMessage(QtDebugMsg, "BaseClass::wasAPureVirtualFunction()");
    baseClass.wasAPureVirtualFunction();
#endif

    // DerivedClass inherits from BaseClass, and function is declared
    // pure virtual, make sure we can still call it
    DerivedClass derivedClass;
    QTest::ignoreMessage(QtDebugMsg, "DerivedClass::wasAPureVirtualFunction()");
    derivedClass.wasAPureVirtualFunction();
}

template<typename T> const char *resolveCharSignedness();

template<>
const char *resolveCharSignedness<char>()
{
    return "char";
}

template<>
const char *resolveCharSignedness<unsigned char>()
{
    return "unsigned char";
}

template<>
const char *resolveCharSignedness<signed char>()
{
    return "signed char";
}

void tst_Compiler::charSignedness() const
{
    QCOMPARE("char",            resolveCharSignedness<char>());
    QCOMPARE("unsigned char",   resolveCharSignedness<unsigned char>());
    QCOMPARE("signed char",     resolveCharSignedness<signed char>());
}

class PrivateStaticTemplateMember
{
public:
    long regularMember()
    {
        return helper<long, int>(3);
    }

private:
    template<typename A, typename B>
    static A helper(const B b)
    {
        return A(b);
    }
};

void tst_Compiler::privateStaticTemplateMember() const
{
    PrivateStaticTemplateMember v;

    QCOMPARE(long(3), v.regularMember());
}


#if !defined(Q_CC_MIPS)

// make sure we can use a static initializer with a union and then use
// the second member of the union
static const union { unsigned char c[8]; double d; } qt_be_inf_bytes = { { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 } };
static const union { unsigned char c[8]; double d; } qt_le_inf_bytes = { { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f } };
static inline double qt_inf()
{
    return (QSysInfo::ByteOrder == QSysInfo::BigEndian
            ? qt_be_inf_bytes.d
            : qt_le_inf_bytes.d);
}

#else

static const unsigned char qt_be_inf_bytes[] = { 0x7f, 0xf0, 0, 0, 0, 0, 0, 0 };
static const unsigned char qt_le_inf_bytes[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x7f };
static inline double qt_inf()
{
    const uchar *bytes;
    bytes = (QSysInfo::ByteOrder == QSysInfo::BigEndian
             ? qt_be_inf_bytes
             : qt_le_inf_bytes);

    union { uchar c[8]; double d; } returnValue;
    memcpy(returnValue.c, bytes, sizeof(returnValue.c));
    return returnValue.d;
}

#endif

void tst_Compiler::staticConstUnionWithInitializerList() const
{
    double d = qt_inf();
    QVERIFY(qIsInf(d));
}

QTEST_MAIN(tst_Compiler)
#include "tst_compiler.moc"
