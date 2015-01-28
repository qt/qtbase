/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Olivier Goffart <ogoffart@woboq.com>
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

#include <qdebug.h>
#include <qglobal.h>
#include <QtCore/QProcess>
#include <QtTest/QtTest>

class tst_qmessagehandler : public QObject
{
    Q_OBJECT
public:
    tst_qmessagehandler();

public slots:
    void initTestCase();

private slots:
    void cleanup();

    void defaultHandler();
    void installMessageHandler();
    void installMsgHandler();
    void installBothHandler();

#ifdef QT_BUILD_INTERNAL
    void cleanupFuncinfo_data();
    void cleanupFuncinfo();
#endif

    void qMessagePattern_data();
    void qMessagePattern();
    void setMessagePattern();

    void formatLogMessage_data();
    void formatLogMessage();

private:
    QString m_appDir;
    QStringList m_baseEnvironment;
};

static QtMsgType s_type;
const char *s_file;
int s_line;
const char *s_function;
static QString s_message;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    s_type = type;
    s_file = context.file;
    s_line = context.line;
    s_function = context.function;
    s_message = msg;
}

void customMsgHandler(QtMsgType type, const char *msg)
{
    s_type = type;
    s_file = 0;
    s_line = 0;
    s_function = 0;
    s_message = QString::fromLocal8Bit(msg);
}

tst_qmessagehandler::tst_qmessagehandler()
{
    // ensure it's unset, otherwise we'll have trouble
    qputenv("QT_MESSAGE_PATTERN", "");
}

void tst_qmessagehandler::initTestCase()
{
    m_appDir = QFINDTESTDATA("app");
    QVERIFY2(!m_appDir.isEmpty(), qPrintable(
        QString::fromLatin1("Couldn't find helper app dir starting from %1.").arg(QDir::currentPath())));

#ifndef QT_NO_PROCESS
    m_baseEnvironment = QProcess::systemEnvironment();
    for (int i = 0; i < m_baseEnvironment.count(); ++i) {
        if (m_baseEnvironment.at(i).startsWith("QT_MESSAGE_PATTERN=")) {
            m_baseEnvironment.removeAt(i);
            break;
        }
    }
#endif // !QT_NO_PROCESS
}

void tst_qmessagehandler::cleanup()
{
    qInstallMsgHandler(0);
    qInstallMessageHandler((QtMessageHandler)0);
    s_type = QtFatalMsg;
    s_file = 0;
    s_line = 0;
    s_function = 0;
}

void tst_qmessagehandler::defaultHandler()
{
    // check that the default works
    QTest::ignoreMessage(QtDebugMsg, "defaultHandler");
    qDebug("defaultHandler");
}

void tst_qmessagehandler::installMessageHandler()
{
    QtMessageHandler oldHandler = qInstallMessageHandler(customMessageHandler);

    qDebug("installMessageHandler"); int line = __LINE__;

    QCOMPARE(s_type, QtDebugMsg);
    QCOMPARE(s_message, QString::fromLocal8Bit("installMessageHandler"));
    QCOMPARE(s_file, __FILE__);
    QCOMPARE(s_function, Q_FUNC_INFO);
    QCOMPARE(s_line, line);

    QtMessageHandler myHandler = qInstallMessageHandler(oldHandler);
    QCOMPARE((void*)myHandler, (void*)customMessageHandler);
}

void tst_qmessagehandler::installMsgHandler()
{
    QtMsgHandler oldHandler = qInstallMsgHandler(customMsgHandler);

    qDebug("installMsgHandler");

    QCOMPARE(s_type, QtDebugMsg);
    QCOMPARE(s_message, QString::fromLocal8Bit("installMsgHandler"));
    QCOMPARE(s_file, (const char*)0);
    QCOMPARE(s_function, (const char*)0);
    QCOMPARE(s_line, 0);

    QtMsgHandler myHandler = qInstallMsgHandler(oldHandler);
    QCOMPARE((void*)myHandler, (void*)customMsgHandler);
}

void tst_qmessagehandler::installBothHandler()
{
    qInstallMessageHandler(customMessageHandler);
    qInstallMsgHandler(customMsgHandler);

    qDebug("installBothHandler"); int line = __LINE__;

    QCOMPARE(s_type, QtDebugMsg);
    QCOMPARE(s_message, QString::fromLocal8Bit("installBothHandler"));
    QCOMPARE(s_file, __FILE__);
    QCOMPARE(s_function, Q_FUNC_INFO);
    QCOMPARE(s_line, line);
}

# define ADD(x)          QTest::newRow(x) << Q_FUNC_INFO << x;

class TestClass1
{
public:
    enum Something { foo };
    char c;

    void func_void() { ADD("TestClass1::func_void"); }
    int func_int() { ADD("TestClass1::func_int"); return 0; }
    unsigned func_unsigned() { ADD("TestClass1::func_unsigned"); return 0; }
    long func_long() { ADD("TestClass1::func_long"); return 0; }
    long long func_ll() { ADD("TestClass1::func_ll"); return 0; }
    unsigned long long func_ull() { ADD("TestClass1::func_ull"); return 0; }
    char func_char() { ADD("TestClass1::func_char"); return 0; }
    signed char func_schar() { ADD("TestClass1::func_schar"); return 0; }
    unsigned char func_uchar() { ADD("TestClass1::func_uchar"); return 0; }
    char &func_Rchar() { ADD("TestClass1::func_Rchar"); return c; }
    char *func_Pchar() { ADD("TestClass1::func_Pchar"); return 0; }
    const char *func_KPchar() { ADD("TestClass1::func_KPchar"); return 0; }
    const volatile char *func_VKPchar() { ADD("TestClass1::func_VKPchar"); return 0; }
    const volatile unsigned long long * const volatile func_KVPKVull() { ADD("TestClass1::func_KVPKVull"); return 0; }
    const void * const volatile *func_KPKVvoid() { ADD("TestClass1::func_KPKVvoid"); return 0; }

    QList<int> func_ai() { ADD("TestClass1::func_ai"); return QList<int>(); }
    QList<unsigned long long const volatile*> func_aptr() { ADD("TestClass1::func_aptr"); return QList<unsigned long long const volatile*>(); }

    QList<Something> func_aenum() { ADD("TestClass1::func_aenum"); return QList<Something>(); }
    QList<QList<const void *> > func_aaptr() { ADD("TestClass1::func_aaptr"); return QList<QList<const void *> >(); }

    QMap<int, Something> func_ienummap() { ADD("TestClass1::func_ienummap"); return QMap<int, Something>(); }

    template<typename T>
    T* func_template1() { ADD("TestClass1::func_template1"); return 0; }
    template<Something val>
    long func_template2() { ADD("TestClass1::func_template2"); return long(val); }

    typedef unsigned long long * ( *fptr)();
    typedef unsigned long long * (TestClass1::* pmf)();
    typedef fptr (TestClass1::* uglypmf)();
    fptr func_fptr() { ADD("TestClass1::func_fptr"); return 0; }
    pmf func_pmf() { ADD("TestClass1::func_pmf"); return 0; }
    uglypmf func_uglypmf(uglypmf = 0) { ADD("TestClass1::func_uglypmf"); return 0; }
    QMap<QString, uglypmf> func_uglypmf2() { ADD("TestClass1::func_uglypmf2"); return QMap<QString, uglypmf>(); }

    void operator()() { ADD("TestClass1::operator()"); }
    int operator<(int) { ADD("TestClass1::operator<"); return 0; }
    int operator>(int) { ADD("TestClass1::operator>"); return 0; }
    int operator<=(int) { ADD("TestClass1::operator<="); return 0; }
    int operator>=(int) { ADD("TestClass1::operator>="); return 0; }
    int operator=(int) { ADD("TestClass1::operator="); return 0; }
    int operator+(int) { ADD("TestClass1::operator+"); return 0; }
    int operator-(int) { ADD("TestClass1::operator-"); return 0; }
    int operator*(int) { ADD("TestClass1::operator*"); return 0; }
    int operator/(int) { ADD("TestClass1::operator/"); return 0; }
    int operator%(int) { ADD("TestClass1::operator%"); return 0; }
    int x;
    int &operator++() { ADD("TestClass1::operator++"); return x; }
    int operator++(int) { ADD("TestClass1::operator++"); return 0; }
    int &operator--() { ADD("TestClass1::operator--"); return x; }
    int operator--(int) { ADD("TestClass1::operator--"); return 0; }

    int nested_struct()
    {
        struct Nested { void nested() { ADD("TestClass1::nested_struct"); } };
        Nested().nested();
        return 0;
    }
    int nested_struct_const() const
    {
        struct Nested { void nested() { ADD("TestClass1::nested_struct_const"); } };
        Nested().nested();
        return 0;
    }

#ifdef Q_COMPILER_REF_QUALIFIERS
    int lvalue() & { ADD("TestClass1::lvalue"); return 0; }
    int const_lvalue() const & { ADD("TestClass1::const_lvalue"); return 0; }
    int rvalue() && { ADD("TestClass1::rvalue"); return 0; }
    int const_rvalue() const && { ADD("TestClass1::const_rvalue"); return 0; }
#endif
#ifdef Q_COMPILER_DECLTYPE
    int decltype_param(int x = 0, decltype(x) = 0) { ADD("TestClass1::decltype_param"); return x; }
    template<typename T> int decltype_template_param(T x = 0, decltype(x) = 0)
    { ADD("TestClass1::decltype_template_param"); return x; }
    template<typename T> void decltype_template_param2(T x, decltype(x + QString()))
    { ADD("TestClass1::decltype_template_param2"); }
#  ifdef Q_COMPILER_AUTO_FUNCTION
    auto decltype_return(int x = 0) -> decltype(x)
    { ADD("TestClass1::decltype_return"); return x; }
    template <typename T> auto decltype_template_return(T x = 0) -> decltype(x)
    { ADD("TestClass1::decltype_template_return"); return x; }
#  endif
#endif

public:
    TestClass1()
        {
            // instantiate
            func_void();
            func_int();
            func_unsigned();
            func_long();
            func_ll();
            func_ull();
            func_char();
            func_schar();
            func_uchar();
            func_Rchar();
            func_Pchar();
            func_KPchar();
            func_VKPchar();
            func_KVPKVull();
            func_KPKVvoid();
            func_ai();
            func_aptr();
            func_aenum();
            func_aaptr();
            func_ienummap();
            func_template1<TestClass1>();
            func_template2<foo>();
            func_fptr();
            func_pmf();
            func_uglypmf();
            func_uglypmf2();
            operator()();
            operator<(0);
            operator>(0);
            operator<=(0);
            operator>=(0);
            operator=(0);
            operator+(0);
            operator-(0);
            operator*(0);
            operator/(0);
            operator%(0);
            operator++();
            operator++(0);
            operator--();
            operator--(0);

            nested_struct();
            nested_struct_const();

#ifdef Q_COMPILER_REF_QUALIFIERS
            lvalue();
            const_lvalue();
            std::move(*this).rvalue();
            std::move(*this).const_rvalue();
#endif
#ifdef Q_COMPILER_DECLTYPE
            decltype_param();
            decltype_template_param(0);
            decltype_template_param2(QByteArray(), QString());
#  ifdef Q_COMPILER_AUTO_FUNCTION
            decltype_return();
            decltype_template_return(0);
#  endif
#endif
        }
};

template<typename T> class TestClass2
{
    long func_long() { ADD("TestClass2::func_long"); return 0; }
    template<typename S>
    T* func_template1() { ADD("TestClass2::func_template1"); return 0; }
    template<TestClass1::Something val>
    long func_template2() { ADD("TestClass2::func_template2"); return long(val); }
public:
    TestClass2()
        {
            func_long();
            func_template1<TestClass2>();
            func_template2<TestClass1::foo>();
        }
};

template<typename T, TestClass1::Something v> class TestClass3
{
    long func_long() { ADD("TestClass3::func_long"); return 0; }
    template<typename S>
    S* func_template1() { ADD("TestClass3::func_template1"); return 0; }
    template<TestClass1::Something val>
    long func_template2() { ADD("TestClass3::func_template2"); return long(val); }
public:
    struct Foo { TestClass3 foo; };
    TestClass3()
        {
            func_long();
            func_template1<TestClass2<T> >();
            func_template2<TestClass1::foo>();
        }
};

class TestClass4
{
    TestClass1 c1;

    TestClass2<std::map<long, const void *> > func2()
        { ADD("TestClass4::func2"); return TestClass2<std::map<long, const void *> >(); }
    TestClass3<std::map<std::list<int>, const void *>, TestClass1::foo>::Foo func3()
        { ADD("TestClass4::func3"); return TestClass3<std::map<std::list<int>, const void *>, TestClass1::foo>::Foo(); }
public:
    TestClass4()
        {
            func2();
            func3();
            ADD("TestClass4::TestClass4");
        }
    ~TestClass4()
        {
            ADD("TestClass4::~TestClass4");
        }
};


#ifdef QT_BUILD_INTERNAL
void tst_qmessagehandler::cleanupFuncinfo_data()
{
    QTest::addColumn<QString>("funcinfo");
    QTest::addColumn<QString>("expected");

    TestClass4 c4;

    QTest::newRow("msvc_01")
        << "void __thiscall TestClass1::func_void(void)"
        << "TestClass1::func_void";
    QTest::newRow("gcc_01")
        << "void TestClass1::func_void()"
        << "TestClass1::func_void";

    QTest::newRow("msvc_02")
        << "int __thiscall TestClass1::func_int(void)"
        << "TestClass1::func_int";
    QTest::newRow("gcc_02")
        << "int TestClass1::func_int()"
        << "TestClass1::func_int";

    QTest::newRow("msvc_03")
        << "unsigned int __thiscall TestClass1::func_unsigned(void)"
        << "TestClass1::func_unsigned";
    QTest::newRow("gcc_03")
        << "unsigned int TestClass1::func_unsigned()"
        << "TestClass1::func_unsigned";

    QTest::newRow("msvc_04")
        << "long __thiscall TestClass1::func_long(void)"
        << "TestClass1::func_long";
    QTest::newRow("gcc_04")
        << "long int TestClass1::func_long()"
        << "TestClass1::func_long";

    QTest::newRow("msvc_05")
        << "__int64 __thiscall TestClass1::func_ll(void)"
        << "TestClass1::func_ll";
    QTest::newRow("gcc_05")
        << "long long int TestClass1::func_ll()"
        << "TestClass1::func_ll";

    QTest::newRow("msvc_06")
        << "unsigned __int64 __thiscall TestClass1::func_ull(void)"
        << "TestClass1::func_ull";
    QTest::newRow("gcc_06")
        << "long long unsigned int TestClass1::func_ull()"
        << "TestClass1::func_ull";

    QTest::newRow("msvc_07")
        << "char __thiscall TestClass1::func_char(void)"
        << "TestClass1::func_char";
    QTest::newRow("gcc_07")
        << "char TestClass1::func_char()"
        << "TestClass1::func_char";

    QTest::newRow("msvc_08")
        << "signed char __thiscall TestClass1::func_schar(void)"
        << "TestClass1::func_schar";
    QTest::newRow("gcc_08")
        << "signed char TestClass1::func_schar()"
        << "TestClass1::func_schar";

    QTest::newRow("msvc_09")
        << "unsigned char __thiscall TestClass1::func_uchar(void)"
        << "TestClass1::func_uchar";
    QTest::newRow("gcc_09")
        << "unsigned char TestClass1::func_uchar()"
        << "TestClass1::func_uchar";

    QTest::newRow("msvc_09a")
        << "char &__thiscall TestClass1::func_Rchar(void)"
        << "TestClass1::func_Rchar";
    QTest::newRow("gcc_09a")
        << "char& TestClass1::func_Rchar()"
        << "TestClass1::func_Rchar";
    QTest::newRow("clang_09a")
        << "char &TestClass1::func_Rchar()"
        << "TestClass1::func_Rchar";

    QTest::newRow("msvc_10")
        << "char *__thiscall TestClass1::func_Pchar(void)"
        << "TestClass1::func_Pchar";
    QTest::newRow("gcc_10")
        << "char* TestClass1::func_Pchar()"
        << "TestClass1::func_Pchar";
    QTest::newRow("clang_10")
        << "char *TestClass1::func_Pchar()"
        << "TestClass1::func_Pchar";

    QTest::newRow("msvc_11")
        << "const char *__thiscall TestClass1::func_KPchar(void)"
        << "TestClass1::func_KPchar";
    QTest::newRow("gcc_11")
        << "const char* TestClass1::func_KPchar()"
        << "TestClass1::func_KPchar";

    QTest::newRow("msvc_12")
        << "volatile const char *__thiscall TestClass1::func_VKPchar(void)"
        << "TestClass1::func_VKPchar";
    QTest::newRow("gcc_12")
        << "const volatile char* TestClass1::func_VKPchar()"
        << "TestClass1::func_VKPchar";

    QTest::newRow("msvc_13")
        << "volatile const unsigned __int64 *volatile const __thiscall TestClass1::func_KVPKVull(void)"
        << "TestClass1::func_KVPKVull";
    QTest::newRow("gcc_13")
        << "const volatile long long unsigned int* const volatile TestClass1::func_KVPKVull()"
        << "TestClass1::func_KVPKVull";

    QTest::newRow("msvc_14")
        << "const void *volatile const *__thiscall TestClass1::func_KPKVvoid(void)"
        << "TestClass1::func_KPKVvoid";
    QTest::newRow("gcc_14")
        << "const void* const volatile* TestClass1::func_KPKVvoid()"
        << "TestClass1::func_KPKVvoid";

    QTest::newRow("msvc_15")
        << "class QList<int> __thiscall TestClass1::func_ai(void)"
        << "TestClass1::func_ai";
    QTest::newRow("gcc_15")
        << "QList<int> TestClass1::func_ai()"
        << "TestClass1::func_ai";

    QTest::newRow("msvc_16")
        << "class QList<unsigned __int64 const volatile *> __thiscall TestClass1::func_aptr(void)"
        << "TestClass1::func_aptr";
    QTest::newRow("gcc_16")
        << "QList<const volatile long long unsigned int*> TestClass1::func_aptr()"
        << "TestClass1::func_aptr";

    QTest::newRow("msvc_17")
        << "class QList<enum TestClass1::Something> __thiscall TestClass1::func_aenum(void)"
        << "TestClass1::func_aenum";
    QTest::newRow("gcc_17")
        << "QList<TestClass1::Something> TestClass1::func_aenum()"
        << "TestClass1::func_aenum";

    QTest::newRow("msvc_18")
        << "class QList<class QList<void const *> > __thiscall TestClass1::func_aaptr(void)"
        << "TestClass1::func_aaptr";
    QTest::newRow("gcc_18")
        << "QList<QList<const void*> > TestClass1::func_aaptr()"
        << "TestClass1::func_aaptr";

    QTest::newRow("msvc_19")
        << "class QMap<int,enum TestClass1::Something> __thiscall TestClass1::func_ienummap(void)"
        << "TestClass1::func_ienummap";
    QTest::newRow("gcc_19")
        << "QMap<int, TestClass1::Something> TestClass1::func_ienummap()"
        << "TestClass1::func_ienummap";

    QTest::newRow("msvc_20")
        << "class TestClass1 *__thiscall TestClass1::func_template1<class TestClass1>(void)"
        << "TestClass1::func_template1";
    QTest::newRow("gcc_20")
        << "T* TestClass1::func_template1() [with T = TestClass1]"
        << "TestClass1::func_template1";

    QTest::newRow("msvc_21")
        << "long __thiscall TestClass1::func_template2<foo>(void)"
        << "TestClass1::func_template2";
    QTest::newRow("gcc_21")
        << "long int TestClass1::func_template2() [with TestClass1::Something val = foo]"
        << "TestClass1::func_template2";

    QTest::newRow("msvc_22")
        << "unsigned __int64 *(__cdecl *__thiscall TestClass1::func_fptr(void))(void)"
        << "TestClass1::func_fptr";
    QTest::newRow("gcc_22")
        << "long long unsigned int* (* TestClass1::func_fptr())()"
        << "TestClass1::func_fptr";

    QTest::newRow("msvc_23")
        << "unsigned __int64 *(__thiscall TestClass1::* __thiscall TestClass1::func_pmf(void))(void)"
        << "TestClass1::func_pmf";
    QTest::newRow("gcc_23")
        << "long long unsigned int* (TestClass1::* TestClass1::func_pmf())()"
        << "TestClass1::func_pmf";

    QTest::newRow("msvc_24")
        << "unsigned __int64 *(__cdecl *(__thiscall TestClass1::* __thiscall TestClass1::func_uglypmf(unsigned __int64 *(__cdecl *(__thiscall TestClass1::* )(void))(void)))(void))(void)"
        << "TestClass1::func_uglypmf";
    QTest::newRow("gcc_24")
        << "long long unsigned int* (* (TestClass1::* TestClass1::func_uglypmf(long long unsigned int* (* (TestClass1::*)())()))())()"
        << "TestClass1::func_uglypmf";

    QTest::newRow("msvc_25")
        << "class QMap<class QString,unsigned __int64 * (__cdecl*(__thiscall TestClass1::*)(void))(void)> __thiscall TestClass1::func_uglypmf2(void)"
        << "TestClass1::func_uglypmf2";
    QTest::newRow("gcc_25")
        << "QMap<QString, long long unsigned int* (* (TestClass1::*)())()> TestClass1::func_uglypmf2()"
        << "TestClass1::func_uglypmf2";

    QTest::newRow("msvc_26")
        << "class TestClass2<class std::map<long,void const *,struct std::less<long>,class std::allocator<struct std::pair<long const ,void const *> > > > __thiscall TestClass4::func2(void)"
        << "TestClass4::func2";
    QTest::newRow("gcc_26")
        << "TestClass2<std::map<long int, const void*, std::less<long int>, std::allocator<std::pair<const long int, const void*> > > > TestClass4::func2()"
        << "TestClass4::func2";

    QTest::newRow("msvc_27")
        << "long __thiscall TestClass2<class std::map<long,void const *,struct std::less<long>,class std::allocator<struct std::pair<long const ,void const *> > > >::func_long(void)"
        << "TestClass2::func_long";
    QTest::newRow("gcc_27")
        << "long int TestClass2<T>::func_long() [with T = std::map<long int, const void*, std::less<long int>, std::allocator<std::pair<const long int, const void*> > >]"
        << "TestClass2::func_long";

    QTest::newRow("msvc_28")
        << "class std::map<long,void const *,struct std::less<long>,class std::allocator<struct std::pair<long const ,void const *> > > *__thiscall TestClass2<class std::map<long,void const *,struct std::less<long>,class std::allocator<struct std::pair<long const ,void const *> > > >::func_template1<class TestClass2<class std::map<long,void const *,struct std::less<long>,class std::allocator<struct std::pair<long const ,void const *> > > >>(void)"
        << "TestClass2::func_template1";
    QTest::newRow("gcc_21")
        << "T* TestClass2<T>::func_template1() [with S = TestClass2<std::map<long int, const void*, std::less<long int>, std::allocator<std::pair<const long int, const void*> > > >, T = std::map<long int, const void*, std::less<long int>, std::allocator<std::pair<const long int, const void*> > >]"
        << "TestClass2::func_template1";

    QTest::newRow("msvc_29")
        << "long __thiscall TestClass2<class std::map<long,void const *,struct std::less<long>,class std::allocator<struct std::pair<long const ,void const *> > > >::func_template2<foo>(void)"
        << "TestClass2::func_template2";
    QTest::newRow("gcc_29")
        << "long int TestClass2<T>::func_template2() [with TestClass1::Something val = foo, T = std::map<long int, const void*, std::less<long int>, std::allocator<std::pair<const long int, const void*> > >]"
        << "TestClass2::func_template2";

    QTest::newRow("msvc_30")
        << "struct TestClass3<class std::map<class std::list<int,class std::allocator<int> >,void const *,struct std::less<class std::list<int,class std::allocator<int> > >,class std::allocator<struct std::pair<class std::list<int,class std::allocator<int> > const ,void const *> > >,0>::Foo __thiscall TestClass4::func3(void)"
        << "TestClass4::func3";
    QTest::newRow("gcc_30")
        << "TestClass3<std::map<std::list<int, std::allocator<int> >, const void*, std::less<std::list<int, std::allocator<int> > >, std::allocator<std::pair<const std::list<int, std::allocator<int> >, const void*> > >, foo>::Foo TestClass4::func3()"
        << "TestClass4::func3";

    QTest::newRow("msvc_31")
        << "long __thiscall TestClass3<class std::map<class std::list<int,class std::allocator<int> >,void const *,struct std::less<class std::list<int,class std::allocator<int> > >,class std::allocator<struct std::pair<class std::list<int,class std::allocator<int> > const ,void const *> > >,0>::func_long(void)"
        << "TestClass3::func_long";
    QTest::newRow("gcc_31")
        << "long int TestClass3<T, v>::func_long() [with T = std::map<std::list<int, std::allocator<int> >, const void*, std::less<std::list<int, std::allocator<int> > >, std::allocator<std::pair<const std::list<int, std::allocator<int> >, const void*> > >, TestClass1::Something v = foo]"
        << "TestClass3::func_long";

    QTest::newRow("msvc_32")
        << "class TestClass2<class std::map<class std::list<int,class std::allocator<int> >,void const *,struct std::less<class std::list<int,class std::allocator<int> > >,class std::allocator<struct std::pair<class std::list<int,class std::allocator<int> > const ,void const *> > > > *__thiscall TestClass3<class std::map<class std::list<int,class std::allocator<int> >,void const *,struct std::less<class std::list<int,class std::allocator<int> > >,class std::allocator<struct std::pair<class std::list<int,class std::allocator<int> > const ,void const *> > >,0>::func_template1<class TestClass2<class std::map<class std::list<int,class std::allocator<int> >,void const *,struct std::less<class std::list<int,class std::allocator<int> > >,class std::allocator<struct std::pair<class std::list<int,class std::allocator<int> > const ,void const *> > > >>(void)"
        << "TestClass3::func_template1";
    QTest::newRow("gcc_32")
        << "S* TestClass3<T, v>::func_template1() [with S = TestClass2<std::map<std::list<int, std::allocator<int> >, const void*, std::less<std::list<int, std::allocator<int> > >, std::allocator<std::pair<const std::list<int, std::allocator<int> >, const void*> > > >, T = std::map<std::list<int, std::allocator<int> >, const void*, std::less<std::list<int, std::allocator<int> > >, std::allocator<std::pair<const std::list<int, std::allocator<int> >, const void*> > >, TestClass1::Something v = foo]"
        << "TestClass3::func_template1";

    QTest::newRow("msvc_33")
        << "long __thiscall TestClass3<class std::map<class std::list<int,class std::allocator<int> >,void const *,struct std::less<class std::list<int,class std::allocator<int> > >,class std::allocator<struct std::pair<class std::list<int,class std::allocator<int> > const ,void const *> > >,0>::func_template2<foo>(void)"
        << "TestClass3::func_template2";
    QTest::newRow("gcc_33")
        << "long int TestClass3<T, v>::func_template2() [with TestClass1::Something val = foo, T = std::map<std::list<int, std::allocator<int> >, const void*, std::less<std::list<int, std::allocator<int> > >, std::allocator<std::pair<const std::list<int, std::allocator<int> >, const void*> > >, TestClass1::Something v = foo]"
        << "TestClass3::func_template2";

    QTest::newRow("msvc_34")
        << "__thiscall TestClass4::TestClass4(void)"
        << "TestClass4::TestClass4";
    QTest::newRow("gcc_34")
        << "TestClass4::TestClass4()"
        << "TestClass4::TestClass4";

    QTest::newRow("msvc_35")
        << "__thiscall TestClass4::~TestClass4(void)"
        << "TestClass4::~TestClass4";
    QTest::newRow("gcc_35")
        << "TestClass4::~TestClass4()"
        << "TestClass4::~TestClass4";

    QTest::newRow("gcc_36")
        << "void TestClass1::operator()()"
        << "TestClass1::operator()";

    QTest::newRow("gcc_37")
        << "long int TestClass1::func_template2() [with TestClass1::Something val = (TestClass1::Something)0u]"
        << "TestClass1::func_template2";

    QTest::newRow("gcc_38")
        << "int TestClass1::operator<(int)"
        << "TestClass1::operator<";

    QTest::newRow("gcc_39")
        << "int TestClass1::operator>(int)"
        << "TestClass1::operator>";

    QTest::newRow("objc_1")
        << "-[SomeClass someMethod:withArguments:]"
        << "-[SomeClass someMethod:withArguments:]";

    QTest::newRow("objc_2")
        << "+[SomeClass withClassMethod:withArguments:]"
        << "+[SomeClass withClassMethod:withArguments:]";

    QTest::newRow("objc_3")
        << "-[SomeClass someMethodWithoutArguments]"
        << "-[SomeClass someMethodWithoutArguments]";

    QTest::newRow("objc_4")
        << "__31-[SomeClass someMethodSchedulingBlock]_block_invoke"
        << "__31-[SomeClass someMethodSchedulingBlock]_block_invoke";
}
#endif

#ifdef QT_BUILD_INTERNAL
QT_BEGIN_NAMESPACE
extern QByteArray qCleanupFuncinfo(QByteArray);
QT_END_NAMESPACE
#endif

#ifdef QT_BUILD_INTERNAL
void tst_qmessagehandler::cleanupFuncinfo()
{
    QFETCH(QString, funcinfo);

//    qDebug() << funcinfo.toLatin1();
    QByteArray result = qCleanupFuncinfo(funcinfo.toLatin1());
    QEXPECT_FAIL("TestClass1::nested_struct", "Nested function processing is broken", Continue);
    QEXPECT_FAIL("TestClass1::nested_struct_const", "Nested function processing is broken", Continue);
    QTEST(QString::fromLatin1(result), "expected");
}
#endif

void tst_qmessagehandler::qMessagePattern_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<QList<QByteArray> >("expected");

    // %{file} is tricky because of shadow builds
    QTest::newRow("basic") << "%{type} %{appname} %{line} %{function} %{message}" << true << (QList<QByteArray>()
            << "debug  44 T::T static constructor"
            //  we can't be sure whether the QT_MESSAGE_PATTERN is already destructed
            << "static destructor"
            << "debug tst_qlogging 65 MyClass::myFunction from_a_function 34"
            << "debug tst_qlogging 75 main qDebug"
            << "info tst_qlogging 76 main qInfo"
            << "warning tst_qlogging 77 main qWarning"
            << "critical tst_qlogging 78 main qCritical"
            << "warning tst_qlogging 81 main qDebug with category"
            << "debug tst_qlogging 85 main qDebug2");


    QTest::newRow("invalid") << "PREFIX: %{unknown} %{message}" << false << (QList<QByteArray>()
            << "QT_MESSAGE_PATTERN: Unknown placeholder %{unknown}"
            << "PREFIX:  qDebug");

    // test the if condition
    QTest::newRow("ifs") << "[%{if-debug}D%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{if-category}%{category}: %{endif}%{message}"
        << true << (QList<QByteArray>()
            << "[D] static constructor"
            //  we can't be sure whether the QT_MESSAGE_PATTERN is already destructed
            << "static destructor"
            << "[D] qDebug"
            << "[W] qWarning"
            << "[C] qCritical"
            << "[W] category: qDebug with category"
            << "[D] qDebug2");

    // test few errors cases
    QTest::newRow("ifs-invalid1") << "PREFIX: %{unknown} %{endif}  %{if-warning}"
        << false << (QList<QByteArray>()
            << "QT_MESSAGE_PATTERN: Unknown placeholder %{unknown}"
            << "QT_MESSAGE_PATTERN: %{endif} without an %{if-*}"
            << "QT_MESSAGE_PATTERN: missing %{endif}");

    QTest::newRow("ifs-invalid2") << "A %{if-debug}DEBUG%{if-warning}WARNING%{endif} %{message}  "
        << false << (QList<QByteArray>()
            << "QT_MESSAGE_PATTERN: %{if-*} cannot be nested"
            << "A DEBUG qDebug  "
            << "A  qWarning  ");

    QTest::newRow("pid-tid") << "%{pid}/%{threadid}: %{message}"
         << true << QList<QByteArray>(); // can't match anything, just test validity
    QTest::newRow("qthreadptr") << "ThreadId:%{qthreadptr}: %{message}"
         << true << (QList<QByteArray>()
              << "ThreadId:0x");

    // This test won't work when midnight is too close... wait a bit
    while (QTime::currentTime() > QTime(23, 59, 30))
        QTest::qWait(10000);
    QTest::newRow("time") << "/%{time yyyy - MM - d}/%{message}"
        << true << (QList<QByteArray>()
            << ('/' + QDateTime::currentDateTime().toString("yyyy - MM - d").toUtf8() + "/qDebug"));

    // %{time}  should have a padding of 6 so if it takes less than 10 seconds to show
    // the first message, there should be 5 spaces
    QTest::newRow("time-process") << "<%{time process}>%{message}" << true << (QList<QByteArray>()
            << "<     ");

#ifdef __GLIBC__
#ifdef QT_NAMESPACE
#define QT_NAMESPACE_STR QT_STRINGIFY(QT_NAMESPACE::)
#else
#define QT_NAMESPACE_STR ""
#endif

#ifndef QT_NO_DEBUG
    QTest::newRow("backtrace") << "[%{backtrace}] %{message}" << true << (QList<QByteArray>()
            // MyClass::qt_static_metacall is explicitly marked as hidden in the Q_OBJECT macro
            << "[MyClass::myFunction|MyClass::mySlot1|?app?|" QT_NAMESPACE_STR "QMetaMethod::invoke|" QT_NAMESPACE_STR "QMetaObject::invokeMethod] from_a_function 34");
#endif

    QTest::newRow("backtrace depth,separator") << "[%{backtrace depth=2 separator=\"\n\"}] %{message}" << true << (QList<QByteArray>()
            << "[MyClass::myFunction\nMyClass::mySlot1] from_a_function 34"
            << "[T::T\n");
#endif

}


void tst_qmessagehandler::qMessagePattern()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    QFETCH(QString, pattern);
    QFETCH(bool, valid);
    QFETCH(QList<QByteArray>, expected);

    QProcess process;
    const QString appExe = m_appDir + "/app";

    //
    // test QT_MESSAGE_PATTERN
    //
    QStringList environment = m_baseEnvironment;
    environment.prepend("QT_MESSAGE_PATTERN=\"" + pattern + "\"");
    process.setEnvironment(environment);

    process.start(appExe);
    QVERIFY2(process.waitForStarted(), qPrintable(
        QString::fromLatin1("Could not start %1: %2").arg(appExe, process.errorString())));
    QByteArray pid = QByteArray::number(process.processId());
    process.waitForFinished();

    QByteArray output = process.readAllStandardError();
//    qDebug() << output;
    QVERIFY(!output.isEmpty());
    QCOMPARE(!output.contains("QT_MESSAGE_PATTERN"), valid);

    foreach (const QByteArray &e, expected) {
        if (!output.contains(e)) {
            qDebug() << output;
            qDebug() << "expected: " << e;
            QVERIFY(output.contains(e));
        }
    }
    if (pattern.startsWith("%{pid}"))
        QVERIFY2(output.startsWith('"' + pid), "PID: " + pid + "\noutput:\n" + output);
#endif
}

void tst_qmessagehandler::setMessagePattern()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else

    //
    // test qSetMessagePattern
    //

    QProcess process;
    const QString appExe = m_appDir + "/app";

    // make sure there is no QT_MESSAGE_PATTERN in the environment
    QStringList environment = m_baseEnvironment;
    QMutableListIterator<QString> iter(environment);
    while (iter.hasNext()) {
        if (iter.next().startsWith("QT_MESSAGE_PATTERN"))
            iter.remove();
    }
    process.setEnvironment(environment);

    process.start(appExe);
    QVERIFY2(process.waitForStarted(), qPrintable(
        QString::fromLatin1("Could not start %1: %2").arg(appExe, process.errorString())));
    process.waitForFinished();

    QByteArray output = process.readAllStandardError();
    //qDebug() << output;
    QByteArray expected = "static constructor\n"
            "[debug] qDebug\n"
            "[info] qInfo\n"
            "[warning] qWarning\n"
            "[critical] qCritical\n"
            "[warning] qDebug with category\n";
#ifdef Q_OS_WIN
    output.replace("\r\n", "\n");
#endif
    QCOMPARE(QString::fromLatin1(output), QString::fromLatin1(expected));
#endif // !QT_NO_PROCESS
}

Q_DECLARE_METATYPE(QtMsgType)

void tst_qmessagehandler::formatLogMessage_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("result");

    QTest::addColumn<QtMsgType>("type");
    QTest::addColumn<QByteArray>("file");
    QTest::addColumn<int>("line");
    QTest::addColumn<QByteArray>("function");
    QTest::addColumn<QByteArray>("category");
    QTest::addColumn<QString>("message");

#define BA QByteArrayLiteral

    QTest::newRow("basic") << "%{type} %{file} %{line} %{function} %{message}"
                           << "debug main.cpp 1 func msg"
                           << QtDebugMsg << BA("main.cpp") << 1 << BA("func") << BA("") << "msg";

    // test the if conditions
    QString format = "[%{if-debug}D%{endif}%{if-info}I%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{if-category}%{category}: %{endif}%{message}";
    QTest::newRow("if-debug")
            << format << "[D] msg"
            << QtDebugMsg << BA("") << 0 << BA("func") << QByteArray() << "msg";
    QTest::newRow("if_info")
            << format << "[I] msg"
            << QtInfoMsg << BA("") << 0 << BA("func") << QByteArray() << "msg";
    QTest::newRow("if_warning")
            << format << "[W] msg"
            << QtWarningMsg << BA("") << 0 << BA("func") << QByteArray() << "msg";
    QTest::newRow("if_critical")
            << format << "[C] msg"
            << QtCriticalMsg << BA("") << 0 << BA("func") << QByteArray() << "msg";
    QTest::newRow("if_fatal")
            << format << "[F] msg"
            << QtFatalMsg << BA("") << 0 << BA("func") << QByteArray() << "msg";
    QTest::newRow("if_cat")
            << format << "[F] cat: msg"
            << QtFatalMsg << BA("") << 0 << BA("func") << BA("cat") << "msg";
}

void tst_qmessagehandler::formatLogMessage()
{
    QFETCH(QString, pattern);
    QFETCH(QString, result);

    QFETCH(QtMsgType, type);
    QFETCH(QByteArray, file);
    QFETCH(int, line);
    QFETCH(QByteArray, function);
    QFETCH(QByteArray, category);
    QFETCH(QString, message);

    qSetMessagePattern(pattern);
    QMessageLogContext ctxt(file, line, function, category.isEmpty() ? 0 : category.data());
    QString r = qFormatLogMessage(type, ctxt, message);
    QCOMPARE(r, result);
}


QTEST_MAIN(tst_qmessagehandler)
#include "tst_qlogging.moc"
