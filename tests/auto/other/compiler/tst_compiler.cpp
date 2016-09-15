/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include <QtCore/QtCore>
#include <QtTest/QtTest>

#include <algorithm>

#define BASECLASS_NOT_ABSTRACT
#include "baseclass.h"
#include "derivedclass.h"

#ifdef Q_COMPILER_ATOMICS
#  include <atomic>
#endif

QT_USE_NAMESPACE

class tst_Compiler : public QObject
{
Q_OBJECT
private slots:
    /* C++98 & C++03 base functionality */
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
    void templateFriends();

    /* C++11 features */
    void cxx11_alignas();
    void cxx11_alignof();
    void cxx11_alignas_alignof();
    void cxx11_atomics();
    void cxx11_attributes();
    void cxx11_auto_function();
    void cxx11_auto_type();
    void cxx11_class_enum();
    void cxx11_constexpr();
    void cxx11_decltype();
    void cxx11_default_members();
    void cxx11_delete_members();
    void cxx11_delegating_constructors();
    void cxx11_explicit_conversions();
    void cxx11_explicit_overrides();
    void cxx11_extern_templates();
    void cxx11_inheriting_constructors();
    void cxx11_initializer_lists();
    void cxx11_lambda();
    void cxx11_nonstatic_member_init();
    void cxx11_noexcept();
    void cxx11_nullptr();
    void cxx11_range_for();
    void cxx11_raw_strings();
    void cxx11_ref_qualifiers();
    void cxx11_rvalue_refs();
    void cxx11_static_assert();
    void cxx11_template_alias();
    void cxx11_thread_local();
    void cxx11_udl();
    void cxx11_unicode_strings();
    void cxx11_uniform_init();
    void cxx11_unrestricted_unions();
    void cxx11_variadic_macros();
    void cxx11_variadic_templates();

    /* C++14 compiler features */
    void cxx14_binary_literals();
    void cxx14_init_captures();
    void cxx14_generic_lambdas();
    void cxx14_constexpr();
    void cxx14_decltype_auto();
    void cxx14_return_type_deduction();
    void cxx14_aggregate_nsdmi();
    void cxx14_variable_templates();

    /* Future / Technical specification compiler features */
    void runtimeArrays();
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
    QVERIFY(QtTestInternal::DataStreamChecker<int>::HasDataStream);
    QVERIFY(QtTestInternal::DataStreamChecker<uint>::HasDataStream);
    QVERIFY(QtTestInternal::DataStreamChecker<char *>::HasDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<const int>::HasInDataStream == true);
    QVERIFY(QtTestInternal::DataStreamChecker<const int>::HasOutDataStream == false);
    QVERIFY(QtTestInternal::DataStreamChecker<const int>::HasDataStream == false);
    QVERIFY(QtTestInternal::DataStreamChecker<double>::HasDataStream);

    QVERIFY(QtTestInternal::DataStreamChecker<QString>::HasDataStream);
    QVERIFY(QtTestInternal::DataStreamChecker<MyString>::HasDataStream);
    QVERIFY(!QtTestInternal::DataStreamChecker<Qxxx>::HasDataStream);

    QVERIFY(QtTestInternal::getSaveOperator<int>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<uint>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<char *>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<double>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<QString>() != 0);
    QVERIFY(QtTestInternal::getSaveOperator<MyString>() != 0);
    QVERIFY(!QtTestInternal::getSaveOperator<Qxxx>());
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

#ifndef Q_NO_TEMPLATE_FRIENDS
template <typename T> class TemplateFriends
{
    T value;
public:
    TemplateFriends(T value) : value(value) {}

    template <typename X> void copy(TemplateFriends<X> other)
    { value = other.value; }

    template <typename X> friend class TemplateFriends;
};

void tst_Compiler::templateFriends()
{
    TemplateFriends<int> ti(42);
    TemplateFriends<long> tl(0);
    tl.copy(ti);
}
#else
void tst_Compiler::templateFriends()
{
    QSKIP("Compiler does not support template friends");
}
#endif

void tst_Compiler::cxx11_alignas()
{
#ifndef Q_COMPILER_ALIGNAS
    QSKIP("Compiler does not support C++11 feature");
#else
    struct S {
        alignas(double) char c;
    };
    QCOMPARE(Q_ALIGNOF(S), Q_ALIGNOF(double));
#endif
}

void tst_Compiler::cxx11_alignof()
{
#ifndef Q_COMPILER_ALIGNOF
    QSKIP("Compiler does not support C++11 feature");
#else
    size_t alignchar = alignof(char);
    size_t aligndouble = alignof(double);
    QVERIFY(alignchar >= 1);
    QVERIFY(alignchar <= aligndouble);
#endif
}

void tst_Compiler::cxx11_alignas_alignof()
{
#if !defined(Q_COMPILER_ALIGNAS) && !defined(Q_COMPILER_ALIGNOF)
    QSKIP("Compiler does not support C++11 feature");
#else
    alignas(alignof(double)) char c;
    Q_UNUSED(c);
#endif
}

void tst_Compiler::cxx11_atomics()
{
#ifndef Q_COMPILER_ATOMICS
    QSKIP("Compiler does not support C++11 feature");
#else
    std::atomic<int> i;
    i.store(42, std::memory_order_seq_cst);
    QCOMPARE(i.load(std::memory_order_acquire), 42);

    std::atomic<short> s;
    s.store(42);
    QCOMPARE(s.load(), short(42));

    std::atomic_flag flag;
    flag.clear();
    QVERIFY(!flag.test_and_set());
    QVERIFY(flag.test_and_set());
#endif
}

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wignored-attributes")
QT_WARNING_DISABLE_CLANG("-Wunused-local-typedefs")
QT_WARNING_DISABLE_GCC("-Wattributes")
QT_WARNING_DISABLE_GCC("-Wunused-local-typedefs")

#ifndef __has_cpp_attribute
#  define __has_cpp_attribute(x) 0
#endif
#ifdef Q_COMPILER_ATTRIBUTES
[[noreturn]] void attribute_f1();
void attribute_f2 [[noreturn]] ();
#  if (defined(__cpp_namespace_attributes) && __cpp_namespace_attributes >= 201411) && __has_cpp_attribute(deprecated)
namespace NS [[deprecated]] { }
#  endif
#endif

void tst_Compiler::cxx11_attributes()
{
#ifndef Q_COMPILER_ATTRIBUTES
    QSKIP("Compiler does not support C++11 feature");
#else
    // Attributes in function parameters and using clauses cause MSVC 2015 to crash
    // https://connect.microsoft.com/VisualStudio/feedback/details/2011594
#  if (!defined(Q_CC_MSVC) || _MSC_FULL_VER >= 190023811) && !defined(Q_CC_INTEL)
    void f([[ ]] int);
    [[ ]] using namespace QtPrivate;
    [[ ]] try {
    } catch ([[]] int) {
    }
#  endif

    struct [[ ]] A { };
    struct B : A {
        [[ ]] int m_i : 32;
        [[noreturn]] void f() const { ::exit(0); }

#  ifdef Q_COMPILER_DEFAULT_DELETE_MEMBERS
        [[ ]] ~B() = default;
        [[ ]] B(const B &) = delete;
#  endif
    };
#  if __has_cpp_attribute(deprecated)
    struct [[deprecated]] C { };
#  endif
    enum [[ ]] E { };
    [[ ]] void [[ ]] * [[ ]] * [[ ]] ptr = 0;
    int B::* [[ ]] pmm = 0;

#  if __has_cpp_attribute(deprecated)
    enum [[deprecated]] E2 {
#    if defined(__cpp_enumerator_attributes) && __cpp_enumerator_attributes >= 201411
        value [[deprecated]] = 0
#    endif
    };
#  endif
#  ifdef Q_COMPILER_LAMBDA
    []()[[ ]] {}();
#  endif
#  ifdef Q_COMPILER_TEMPLATE_ALIAS
    using B2 [[ ]] = B;
#  endif

    [[ ]] goto end;
#  ifdef Q_CC_GNU
    // Attributes in gnu:: namespace
    [[gnu::unused]] end:
        ;
    [[gnu::unused]] struct D {} d;
    struct D e [[gnu::used, gnu::unused]];
    [[gnu::aligned(8)]] int i [[ ]];
    int array[][[]] = { 1 };
#  else
    // Non GNU, so use an empty attribute
    [[ ]] end:
        ;
    [[ ]] struct D {} d;
    struct D e [[ ]];
    [[ ]] int i [[ ]];
    int array[][[]] = { 1 };
#  endif

    int & [[ ]] lref = i;
    int && [[ ]] rref = 1;
    [[ ]] (void)1;
    [[ ]] for (i = 0; i < 2; ++i)
        ;

    Q_UNUSED(ptr);
    Q_UNUSED(pmm);
    Q_UNUSED(d);
    Q_UNUSED(e);
    Q_UNUSED(i);
    Q_UNUSED(array);
    Q_UNUSED(lref);
    Q_UNUSED(rref);
#endif
}
QT_WARNING_POP

#ifdef Q_COMPILER_AUTO_FUNCTION
auto autoFunction() -> unsigned
{
    return 1;
}
#endif

void tst_Compiler::cxx11_auto_function()
{
#ifndef Q_COMPILER_AUTO_FUNCTION
    QSKIP("Compiler does not support C++11 feature");
#else
    QCOMPARE(autoFunction(), 1u);
#endif
}

void tst_Compiler::cxx11_auto_type()
{
#ifndef Q_COMPILER_AUTO_TYPE
    QSKIP("Compiler does not support C++11 feature");
#else
    auto i = 1;
    auto x = qrand();
    auto l = 1L;
    auto s = QStringLiteral("Hello World");

    QCOMPARE(i, 1);
    Q_UNUSED(x);
    QCOMPARE(l, 1L);
    QCOMPARE(s.toLower(), QString("hello world"));
#endif
}

void tst_Compiler::cxx11_class_enum()
{
#ifndef Q_COMPILER_CLASS_ENUM
    QSKIP("Compiler does not support C++11 feature");
#else
    enum class X { EnumValue };
    X x = X::EnumValue;
    QCOMPARE(x, X::EnumValue);

    enum class Y : short { Val = 2 };
    enum Z : long { ValLong = LONG_MAX };
#endif
}

#ifdef Q_COMPILER_CONSTEXPR
constexpr int constexprValue()
{
    return 42;
}
#endif

void tst_Compiler::cxx11_constexpr()
{
#ifndef Q_COMPILER_CONSTEXPR
    QSKIP("Compiler does not support C++11 feature");
#else
    static constexpr QBasicAtomicInt atomic = Q_BASIC_ATOMIC_INITIALIZER(1);
    static constexpr int i = constexprValue();
    QCOMPARE(i, constexprValue());
    QCOMPARE(atomic.load(), 1);
#endif
}

void tst_Compiler::cxx11_decltype()
{
#ifndef Q_COMPILER_DECLTYPE
    QSKIP("Compiler does not support C++11 feature");
#else
    decltype(qrand()) i = 0;
    QCOMPARE(i, 0);
#endif
}

void tst_Compiler::cxx11_default_members()
{
#ifndef Q_COMPILER_DEFAULT_MEMBERS
    QSKIP("Compiler does not support C++11 feature");
#else
    class DefaultMembers
    {
    protected:
        DefaultMembers() = default;
    public:
        DefaultMembers(int) {}
    };
    class DefaultMembersChild: public DefaultMembers
    {
        DefaultMembersChild(const DefaultMembersChild &) : DefaultMembers() {}
    public:
        DefaultMembersChild():DefaultMembers() {};
        DefaultMembersChild(DefaultMembersChild &&) = default;
    };
    DefaultMembersChild dm;
    DefaultMembersChild dm2 = std::move(dm);
    Q_UNUSED(dm2);
#endif
}

void tst_Compiler::cxx11_delete_members()
{
#ifndef Q_COMPILER_DELETE_MEMBERS
    QSKIP("Compiler does not support C++11 feature");
#else
    class DeleteMembers
    {
    protected:
        DeleteMembers() = delete;
    public:
        DeleteMembers(const DeleteMembers &) = delete;
        ~DeleteMembers() = delete;
    };
#endif
}

void tst_Compiler::cxx11_delegating_constructors()
{
#ifndef Q_COMPILER_DELEGATING_CONSTRUCTORS
    QSKIP("Compiler does not support C++11 feature");
#else
    struct DC {
        DC(int i) : i(i) {}
        DC() : DC(0) {}
        int i;
    };

    DC dc;
    QCOMPARE(dc.i, 0);
#endif
}

void tst_Compiler::cxx11_explicit_conversions()
{
#ifndef Q_COMPILER_EXPLICIT_CONVERSIONS
    QSKIP("Compiler does not support C++11 feature");
#else
    struct EC {
        explicit operator int() const { return 0; }
        operator long long() const { return 1; }
    };
    EC ec;

    int i(ec);
    QCOMPARE(i, 0);

    int i2 = ec;
    QCOMPARE(i2, 1);
#endif
}

void tst_Compiler::cxx11_explicit_overrides()
{
#ifndef Q_COMPILER_EXPLICIT_OVERRIDES
    QSKIP("Compiler does not support C++11 feature");
#else
    struct Base {
        virtual ~Base() {}
        virtual void f() {}
    };
    struct Derived final : public Base {
        virtual void f() final override {}
    };
#endif
}

#ifdef Q_COMPILER_EXTERN_TEMPLATES
template <typename T> T externTemplate() { return T(0); }
extern template int externTemplate<int>();
#endif

void tst_Compiler::cxx11_extern_templates()
{
#ifndef Q_COMPILER_EXTERN_TEMPLATES
    QSKIP("Compiler does not support C++11 feature");
#else
    QCOMPARE(externTemplate<int>(), 42);
#endif
}

void tst_Compiler::cxx11_inheriting_constructors()
{
#ifndef Q_COMPILER_INHERITING_CONSTRUCTORS
    QSKIP("Compiler does not support C++11 feature");
#else
    struct Base {
        int i;
        Base() : i(0) {}
        Base(int i) : i(i) {}
    };
    struct Derived : public Base {
        using Base::Base;
    };

    Derived d(1);
    QCOMPARE(d.i, 1);
#endif
}

void tst_Compiler::cxx11_initializer_lists()
{
#ifndef Q_COMPILER_INITIALIZER_LISTS
    QSKIP("Compiler does not support C++11 feature");
#else
    QList<int> l = { 1, 2, 3, 4, 5 };
    QCOMPARE(l.length(), 5);
    QCOMPARE(l.at(0), 1);
    QCOMPARE(l.at(4), 5);
#endif
}

struct CallFunctor
{
    template <typename Functor> static int f(Functor f)
    { return f();}
};

void tst_Compiler::cxx11_lambda()
{
#ifndef Q_COMPILER_LAMBDA
    QSKIP("Compiler does not support C++11 feature");
#else
    QCOMPARE(CallFunctor::f([]() { return 42; }), 42);
#endif
}

void tst_Compiler::cxx11_nonstatic_member_init()
{
#ifndef Q_COMPILER_NONSTATIC_MEMBER_INIT
    QSKIP("Compiler does not support C++11 feature");
#else
    struct S {
        int i = 42;
        long l = 47;
        char c;
        S() : l(-47), c(0) {}
    };
    S s;

    QCOMPARE(s.i, 42);
    QCOMPARE(s.l, -47L);
    QCOMPARE(s.c, '\0');
#endif
}

void tst_Compiler::cxx11_noexcept()
{
#ifndef Q_COMPILER_NOEXCEPT
    QSKIP("Compiler does not support C++11 feature");
#else
    extern void noexcept_f() noexcept;
    extern void g() noexcept(noexcept(noexcept_f()));
    QCOMPARE(noexcept(cxx11_noexcept()), false);
    QCOMPARE(noexcept(noexcept_f), true);
    QCOMPARE(noexcept(g), true);
#endif
}

void tst_Compiler::cxx11_nullptr()
{
#ifndef Q_COMPILER_NULLPTR
    QSKIP("Compiler does not support C++11 feature");
#else
    void *v = nullptr;
    char *c = nullptr;
    const char *cc = nullptr;
    volatile char *vc = nullptr;
    std::nullptr_t np = nullptr;
    v = np;

    Q_UNUSED(v);
    Q_UNUSED(c);
    Q_UNUSED(cc);
    Q_UNUSED(vc);
#endif
}

namespace SomeNamespace {
class AdlOnly {
    QVector<int> v;
public:
    AdlOnly() : v(5) { std::fill_n(v.begin(), v.size(), 42); }

private:
    friend QVector<int>::const_iterator begin(const AdlOnly &x) { return x.v.begin(); }
    friend QVector<int>::const_iterator end(const AdlOnly &x) { return x.v.end(); }
    friend QVector<int>::iterator begin(AdlOnly &x) { return x.v.begin(); }
    friend QVector<int>::iterator end(AdlOnly &x) { return x.v.end(); }
};
}

void tst_Compiler::cxx11_range_for()
{
#ifndef Q_COMPILER_RANGE_FOR
    QSKIP("Compiler does not support C++11 feature");
#else
    QList<int> l;
    l << 1 << 2 << 3;
    for (int i : l)
        Q_UNUSED(i);

    l.clear();
    l << 1;
    for (int i : l)
        QCOMPARE(i, 1);

    QList<long> ll;
    l << 2;
    for (int i : ll)
        QCOMPARE(i, 2);

    {
        const int array[] = { 0, 1, 2, 3, 4 };
        int i = 0;
        for (const int &e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (int e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (const int e : array)
            QCOMPARE(e, array[i++]);
#ifdef Q_COMPILER_AUTO_TYPE
        i = 0;
        for (const auto &e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (auto &e : array) // auto deducing const
            QCOMPARE(e, array[i++]);
        i = 0;
        for (auto e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (const auto e : array)
            QCOMPARE(e, array[i++]);
#endif
    }

    {
        int array[] = { 0, 1, 2, 3, 4 };
        const int array2[] = { 10, 11, 12, 13, 14 };
        int i = 0;
        for (const int &e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (int &e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (int e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (const int e : array)
            QCOMPARE(e, array[i++]);
#ifdef Q_COMPILER_AUTO_TYPE
        i = 0;
        for (const auto &e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (auto &e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (auto e : array)
            QCOMPARE(e, array[i++]);
        i = 0;
        for (const auto e : array)
            QCOMPARE(e, array[i++]);
#endif
        for (int &e : array)
            e += 10;
        i = 0;
        for (const int &e : array)
            QCOMPARE(e, array2[i++]);
    }

    {
        const SomeNamespace::AdlOnly x;
        for (const int &e : x)
            QCOMPARE(e, 42);
    }

    {
        SomeNamespace::AdlOnly x;
        for (const int &e : x)
            QCOMPARE(e, 42);
        for (int &e : x)
            e += 10;
        for (const int &e : x)
            QCOMPARE(e, 52);
    }
#endif
}

void tst_Compiler::cxx11_raw_strings()
{
#ifndef Q_COMPILER_RAW_STRINGS
    QSKIP("Compiler does not support C++11 feature");
#else
    static const char xml[] = R"(<?xml version="1.0" encoding="UTF-8" ?>)";
    static const char raw[] = R"***(*"*)***";
    QCOMPARE(strlen(raw), size_t(3));
    QCOMPARE(raw[1], '"');
    Q_UNUSED(xml);
#endif
}

void tst_Compiler::cxx11_ref_qualifiers()
{
#ifndef Q_COMPILER_REF_QUALIFIERS
    QSKIP("Compiler does not support C++11 feature");
#else
#  ifndef Q_COMPILER_RVALUE_REFS
#    error "Impossible condition: ref qualifiers are supported but not rvalue refs"
#  endif
    // also applies to function pointers
    QByteArray (QString:: *lvalueref)() const & = &QString::toLatin1;
    QByteArray (QString:: *rvalueref)() && = &QString::toLatin1;

    QString s("Hello");
    QCOMPARE((s.*lvalueref)(), QByteArray("Hello"));
    QCOMPARE((std::move(s).*rvalueref)(), QByteArray("Hello"));

    // tests internal behavior:
    QVERIFY(s.isEmpty());
#endif
}

class MoveDefinedQString {
    QString s;
public:
    MoveDefinedQString() : s() {}
    explicit MoveDefinedQString(const QString &s) : s(s) {}
    MoveDefinedQString(const MoveDefinedQString &other) : s(other.s) {}
#ifdef Q_COMPILER_RVALUE_REFS
    MoveDefinedQString(MoveDefinedQString &&other) : s(std::move(other.s)) { other.s.clear(); }
    MoveDefinedQString &operator=(MoveDefinedQString &&other)
    { s = std::move(other.s); other.s.clear(); return *this; }
#endif
    MoveDefinedQString &operator=(const MoveDefinedQString &other) { s = other.s; return *this; }

private:
    friend bool operator==(const MoveDefinedQString &lhs, const MoveDefinedQString &rhs)
    { return lhs.s == rhs.s; }
    friend bool operator!=(const MoveDefinedQString &lhs, const MoveDefinedQString &rhs)
    { return !operator==(lhs, rhs); }
    friend char* toString(const MoveDefinedQString &mds)
    { using namespace QTest; return toString(mds.s); }
};

void tst_Compiler::cxx11_rvalue_refs()
{
#ifndef Q_COMPILER_RVALUE_REFS
    QSKIP("Compiler does not support C++11 feature");
#else
    // we require std::move:
    {
        int i = 1;
        i = std::move(i);

        MoveDefinedQString s("Hello");
        MoveDefinedQString t = std::move(s);
        QCOMPARE(t, MoveDefinedQString("Hello"));
        QCOMPARE(s, MoveDefinedQString());

        s = t;
        t = std::move(s);
        QCOMPARE(t, MoveDefinedQString("Hello"));
        QCOMPARE(s, MoveDefinedQString());

        MoveDefinedQString &&r = std::move(t); // no actual move!
        QCOMPARE(r, MoveDefinedQString("Hello"));
        QCOMPARE(t, MoveDefinedQString("Hello")); // so 't' is unchanged
    }

    // we require std::forward:
    {
        MoveDefinedQString s("Hello");
        MoveDefinedQString s2 = std::forward<MoveDefinedQString>(s); // forward as rvalue
        QCOMPARE(s2, MoveDefinedQString("Hello"));
        QCOMPARE(s, MoveDefinedQString());

        MoveDefinedQString s3 = std::forward<MoveDefinedQString&>(s2); // forward as lvalue
        QCOMPARE(s2, MoveDefinedQString("Hello"));
        QCOMPARE(s3, MoveDefinedQString("Hello"));
    }

    // supported by MSVC only from November 2013 CTP, but only check for VC2015:
# if !defined(Q_CC_MSVC) || defined(Q_CC_INTEL) || _MSC_VER >= 1900 // VS14 == VC2015
    // we require automatic generation of move special member functions:
    {
        struct M { MoveDefinedQString s1, s2; };
        M m1 = { MoveDefinedQString("Hello"), MoveDefinedQString("World") };
        QCOMPARE(m1.s1, MoveDefinedQString("Hello"));
        QCOMPARE(m1.s2, MoveDefinedQString("World"));
        M m2 = std::move(m1);
        QCOMPARE(m1.s1, MoveDefinedQString());
        QCOMPARE(m1.s2, MoveDefinedQString());
        QCOMPARE(m2.s1, MoveDefinedQString("Hello"));
        QCOMPARE(m2.s2, MoveDefinedQString("World"));
        M m3;
        QCOMPARE(m3.s1, MoveDefinedQString());
        QCOMPARE(m3.s2, MoveDefinedQString());
        m3 = std::move(m2);
        QCOMPARE(m2.s1, MoveDefinedQString());
        QCOMPARE(m2.s2, MoveDefinedQString());
        QCOMPARE(m3.s1, MoveDefinedQString("Hello"));
        QCOMPARE(m3.s2, MoveDefinedQString("World"));
    }
# endif // MSVC < 2015
#endif
}

void tst_Compiler::cxx11_static_assert()
{
#ifndef Q_COMPILER_STATIC_ASSERT
    QSKIP("Compiler does not support C++11 feature");
#else
    static_assert(true, "Message");
#endif
}

#ifdef Q_COMPILER_TEMPLATE_ALIAS
template <typename T> using Map = QMap<QString, T>;
#endif

void tst_Compiler::cxx11_template_alias()
{
#ifndef Q_COMPILER_TEMPLATE_ALIAS
    QSKIP("Compiler does not support C++11 feature");
#else
    Map<QVariant> m;
    m.insert("Hello", "World");
    QCOMPARE(m.value("Hello").toString(), QString("World"));

    using X = int;
    X i = 0;
    Q_UNUSED(i);
#endif
}

#ifdef Q_COMPILER_THREAD_LOCAL
static thread_local int stl = 42;
thread_local int gtl = 42;
#endif

void tst_Compiler::cxx11_thread_local()
{
#ifndef Q_COMPILER_THREAD_LOCAL
    QSKIP("Compiler does not support C++11 feature");
#else
    thread_local int v = 1;
    QVERIFY(v);
    QVERIFY(stl);
    QVERIFY(gtl);

    thread_local QString s = "Hello";
    QVERIFY(!s.isEmpty());
#endif
}

#ifdef Q_COMPILER_UDL
QString operator"" _tstqstring(const char *str, size_t len)
{
    return QString::fromUtf8(str, len) + " UDL";
}
#endif

void tst_Compiler::cxx11_udl()
{
#ifndef Q_COMPILER_UDL
    QSKIP("Compiler does not support C++11 feature");
#else
    QString s = "Hello World"_tstqstring;
    QCOMPARE(s, QString("Hello World UDL"));
#endif
}

void tst_Compiler::cxx11_unicode_strings()
{
#ifndef Q_COMPILER_UNICODE_STRINGS
    QSKIP("Compiler does not support C++11 feature");
#else
    static const char16_t u[] = u"\u200BHello\u00A0World";
    QCOMPARE(u[0], char16_t(0x200B));

    static const char32_t U[] = U"\ufffe";
    QCOMPARE(U[0], char32_t(0xfffe));

    QCOMPARE(u"\U00010000"[0], char16_t(0xD800));
    QCOMPARE(u"\U00010000"[1], char16_t(0xDC00));
#endif
}

static void noop(QPair<int, int>) {}
void tst_Compiler::cxx11_uniform_init()
{
#ifndef Q_COMPILER_UNIFORM_INIT
    QSKIP("Compiler does not support C++11 feature");
    noop(QPair<int,int>());
#else
    QString s{"Hello"};
    int i{};
    noop(QPair<int,int>{1,1});
    noop({i,1});
#endif
}

void tst_Compiler::cxx11_unrestricted_unions()
{
#ifndef Q_COMPILER_UNRESTRICTED_UNIONS
    QSKIP("Compiler does not support C++11 feature");
#else
    union U {
        QString s;
        U() {}
        U(const QString &s) : s(s) {}
        ~U() {}
    };
    U u;
    std::aligned_storage<sizeof(QString), Q_ALIGNOF(QString)> as;
    Q_UNUSED(u);
    Q_UNUSED(as);

    U u2("hello");
    u2.s.~QString();
#endif
}

void tst_Compiler::cxx11_variadic_macros()
{
#ifndef Q_COMPILER_VARIADIC_MACROS
    QSKIP("Compiler does not support C++11 feature");
#else
#  define TEST_VARARG(x, ...) __VA_ARGS__
    QCOMPARE(TEST_VARARG(0, 1), 1);
#endif
}

#ifdef Q_COMPILER_VARIADIC_TEMPLATES
template <typename... Args> struct VariadicTemplate {};
#endif

void tst_Compiler::cxx11_variadic_templates()
{
#ifndef Q_COMPILER_VARIADIC_TEMPLATES
    QSKIP("Compiler does not support C++11 feature");
#else
    VariadicTemplate<> v0;
    VariadicTemplate<int> v1;
    VariadicTemplate<int, int, int, int,
                     int, int, int, int> v8;
    Q_UNUSED(v0);
    Q_UNUSED(v1);
    Q_UNUSED(v8);
#endif
}

void tst_Compiler::cxx14_binary_literals()
{
#if __cpp_binary_literals-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    int i = 0b11001001;
    QCOMPARE(i, 0xC9);
#endif
}

void tst_Compiler::cxx14_init_captures()
{
#if __cpp_init_captures-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    QCOMPARE([x = 42]() { return x; }(), 42);
#endif
}

void tst_Compiler::cxx14_generic_lambdas()
{
#if __cpp_generic_lambdas-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    auto identity = [](auto x) { return x; };
    QCOMPARE(identity(42), 42);
    QCOMPARE(identity(42U), 42U);
    QCOMPARE(identity(42L), 42L);
#endif
}

#if __cpp_constexpr-0 >= 201304
constexpr int relaxedConstexpr(int i)
{
    i *= 2;
    i += 2;
    return i;
}
#endif

void tst_Compiler::cxx14_constexpr()
{
#if __cpp_constexpr-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    QCOMPARE(relaxedConstexpr(0), 2);
    QCOMPARE(relaxedConstexpr(2), 6);
#endif
}

void tst_Compiler::cxx14_decltype_auto()
{
#if __cpp_decltype_auto-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    QList<int> l;
    l << 1;
    decltype(auto) value = l[0];
    value = 2;
    QCOMPARE(l.at(0), 2);
#endif
}

#if __cpp_return_type_deduction >= 201304
auto returnTypeDeduction(bool choice)
{
    if (choice)
        return 1U;
    return returnTypeDeduction(!choice);
}
#endif

void tst_Compiler::cxx14_return_type_deduction()
{
#if __cpp_return_type_deduction-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    QCOMPARE(returnTypeDeduction(false), 1U);
#endif
}

void tst_Compiler::cxx14_aggregate_nsdmi()
{
#if __cpp_aggregate_nsdmi-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    struct S { int i, j = i; };
    S s = { 1 };
    QCOMPARE(s.j, 1);
#endif
}

#if __cpp_variable_templates >= 201304
template <typename T> constexpr T variableTemplate = T(42);
#endif
void tst_Compiler::cxx14_variable_templates()
{
#if __cpp_variable_templates-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    QCOMPARE(variableTemplate<int>, 42);
    QCOMPARE(variableTemplate<long>, 42L);
    QCOMPARE(variableTemplate<unsigned>, 42U);
    QCOMPARE(variableTemplate<unsigned long long>, 42ULL);
#endif
}

void tst_Compiler::runtimeArrays()
{
#if __cpp_runtime_arrays-0 < 201304
    QSKIP("Compiler does not support this C++14 feature");
#else
    int i[qrand() & 0x1f];
    Q_UNUSED(i);
#endif
}

QTEST_MAIN(tst_Compiler)
#include "tst_compiler.moc"
