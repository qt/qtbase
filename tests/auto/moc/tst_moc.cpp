/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/



#include <QtTest/QtTest>
#include <stdio.h>
#include <qobject.h>

#include "using-namespaces.h"
#include "assign-namespace.h"
#include "no-keywords.h"
#include "single_function_keyword.h"
#include "backslash-newlines.h"
#include "slots-with-void-template.h"
#include "pure-virtual-signals.h"
#include "qinvokable.h"
// msvc and friends crap out on it
#if !defined(Q_CC_GNU) || defined(Q_OS_IRIX) || defined(Q_OS_WIN)
#define SKIP_NEWLINE_TEST
#endif
#if !defined(SKIP_NEWLINE_TEST)
#include "os9-newlines.h"
// msvc and friends crap out on this file too,
// it seems to contain Mac 9 EOLs, and not windows EOLs.
#include "win-newlines.h"
#endif
#include "escapes-in-string-literals.h"
#include "cstyle-enums.h"


#if defined(PARSE_BOOST)
#include "parse-boost.h"
#endif

// No such thing as "long long" in Microsoft's compiler 13.0 and before
#if defined Q_CC_MSVC && _MSC_VER <= 1310
#  define NOLONGLONG
#endif

QT_USE_NAMESPACE

struct MyStruct {};
struct MyStruct2 {};

struct SuperClass {};

// Try to avoid inserting for instance a comment with a quote between the following line and the Q_OBJECT
// That will make the test give a false positive.
const char* test_multiple_number_of_escapes =   "\\\"";
namespace MyNamespace
{
    class TestSuperClass : public QObject
    {
        Q_OBJECT
        public:
            inline TestSuperClass() {}
    };
}

namespace String
{
    typedef QString Type;
}

namespace Int
{
    typedef int Type;
}

typedef struct {
    int doNotConfuseMoc;
} OldStyleCStruct;

class Sender : public QObject
{
    Q_OBJECT

public:
    void sendValue(const String::Type& value)
    {
        emit send(value);
    }
    void sendValue(const Int::Type& value)
    {
        emit send(value);
    }

signals:
    void send(const String::Type&);
    void send(const Int::Type&);
};

class Receiver : public QObject
{
    Q_OBJECT
public:
    Receiver() : stringCallCount(0), intCallCount(0) {}

    int stringCallCount;
    int intCallCount;

public slots:
    void receive(const String::Type&) { stringCallCount++; }
    void receive(const Int::Type&)    { intCallCount++; }
};

#define MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES

#define DONT_CONFUSE_MOC(klass) klass
#define DONT_CONFUSE_MOC_EVEN_MORE(klass, dummy, dummy2) klass

Q_DECLARE_METATYPE(MyStruct)
Q_DECLARE_METATYPE(MyStruct*)

namespace myNS {
    struct Points
    {
        Points() : p1(0xBEEF), p2(0xBABE) { }
        int p1, p2;
    };
}

Q_DECLARE_METATYPE(myNS::Points)

class TestClassinfoWithEscapes: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("escaped", "\"bar\"")
    Q_CLASSINFO("\"escaped\"", "foo")
public slots:
    void slotWithAReallyLongName(int)
    { }
};

struct ForwardDeclaredStruct;

struct StructQObject : public QObject
{
    Q_OBJECT
public:
    void foo(struct ForwardDeclaredStruct *);
};

void StructQObject::foo(struct ForwardDeclaredStruct *)
{
    struct Inner {
        bool field;
    };

    struct Inner unusedVariable;
}

class TestClass : public MyNamespace::TestSuperClass, public DONT_CONFUSE_MOC(MyStruct),
                  public DONT_CONFUSE_MOC_EVEN_MORE(MyStruct2, dummy, ignored)
{
    Q_OBJECT
    Q_CLASSINFO("help", QT_TR_NOOP("Opening this will let you configure something"))
    Q_PROPERTY(short int shortIntProperty READ shortIntProperty)
    Q_PROPERTY(unsigned short int unsignedShortIntProperty READ unsignedShortIntProperty)
    Q_PROPERTY(signed short int signedShortIntProperty READ signedShortIntProperty)
    Q_PROPERTY(long int longIntProperty READ longIntProperty)
    Q_PROPERTY(unsigned long int unsignedLongIntProperty READ unsignedLongIntProperty)
    Q_PROPERTY(signed long int signedLongIntProperty READ signedLongIntProperty)
    Q_PROPERTY(long double longDoubleProperty READ longDoubleProperty)
    Q_PROPERTY(myNS::Points points READ points WRITE setPoints)

    Q_CLASSINFO("Multi"
                "line",
                ""
                "This is a "
                "multiline Q_CLASSINFO"
                "")

    // a really really long string that we have to cut into pieces in the generated stringdata
    // table, otherwise msvc craps out
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.kde.KCookieServer\" >\n"
"    <method name=\"findCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"cookies\" />\n"
"    </method>\n"
"    <method name=\"findDomains\" >\n"
"      <arg direction=\"out\" type=\"as\" name=\"domains\" />\n"
"    </method>\n"
"    <method name=\"findCookies\" >\n"
"      <arg direction=\"in\" type=\"ai\" name=\"fields\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"domain\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"fqdn\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"path\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\" />\n"
"      <arg direction=\"out\" type=\"as\" name=\"cookies\" />\n"
"      <annotation value=\"QList&lt;int>\" name=\"com.trolltech.QtDBus.QtTypeName.In0\" />\n"
"    </method>\n"
"    <method name=\"findDOMCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"cookies\" />\n"
"    </method>\n"
"    <method name=\"addCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"ay\" name=\"cookieHeader\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\"  />\n"
"    </method>\n"
"    <method name=\"deleteCookie\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"domain\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"fqdn\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"path\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"name\" />\n"
"    </method>\n"
"    <method name=\"deleteCookiesFromDomain\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"domain\" />\n"
"    </method>\n"
"    <method name=\"deleteSessionCookies\" >\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"    </method>\n"
"    <method name=\"deleteSessionCookiesFor\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"fqdn\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"    </method>\n"
"    <method name=\"deleteAllCookies\" />\n"
"    <method name=\"addDOMCookies\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"ay\" name=\"cookieHeader\" />\n"
"      <arg direction=\"in\" type=\"x\" name=\"windowId\" />\n"
"    </method>\n"
"    <method name=\"setDomainAdvice\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"in\" type=\"s\" name=\"advice\" />\n"
"    </method>\n"
"    <method name=\"getDomainAdvice\" >\n"
"      <arg direction=\"in\" type=\"s\" name=\"url\" />\n"
"      <arg direction=\"out\" type=\"s\" name=\"advice\" />\n"
"    </method>\n"
"    <method name=\"reloadPolicy\" />\n"
"    <method name=\"shutdown\" />\n"
"  </interface>\n"
        "")

public:
    inline TestClass() {}

private slots:
    inline void dummy1() MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES {}
    inline void dummy2() MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES const {}
    inline void dummy3() const MACRO_WITH_POSSIBLE_COMPILER_SPECIFIC_ATTRIBUTES {}

#ifndef NOLONGLONG
    void slotWithULongLong(unsigned long long) {}
    void slotWithULongLongP(unsigned long long*) {}
    void slotWithULong(unsigned long) {}
    void slotWithLongLong(long long) {}
    void slotWithLong(long) {}
#endif

    void slotWithColonColonType(::Int::Type) {}

    TestClass &slotWithReferenceReturnType() { return *this; }

#if (0 && 1) || 1
    void expressionEvaluationShortcut1() {}
#endif
#if (1 || 0) && 0
#else
    void expressionEvaluationShortcut2() {}
#endif

public slots:
    void slotWithArray(const double[3]) {}
    void slotWithNamedArray(const double namedArray[3]) {}
    void slotWithMultiArray(const double[3][4]) {}

    short int shortIntProperty() { return 0; }
    unsigned short int unsignedShortIntProperty() { return 0; }
    signed short int signedShortIntProperty() { return 0; }
    long int longIntProperty() { return 0; }
    unsigned long int unsignedLongIntProperty() { return 0; }
    signed long int signedLongIntProperty() { return 0; }
    long double longDoubleProperty() { return 0.0; }

    myNS::Points points() { return m_points; }
    void setPoints(myNS::Points points) { m_points = points; }

signals:
    void signalWithArray(const double[3]);
    void signalWithNamedArray(const double namedArray[3]);

private slots:
    // for tst_Moc::preprocessorConditionals
#if 0
    void invalidSlot() {}
#else
    void slotInElse() {}
#endif

#if 1
    void slotInIf() {}
#else
    void invalidSlot() {}
#endif

#if 0
    void invalidSlot() {}
#elif 0
#else
    void slotInLastElse() {}
#endif

#if 0
    void invalidSlot() {}
#elif 1
    void slotInElif() {}
#else
    void invalidSlot() {}
#endif


    friend class Receiver; // task #85783
signals:
    friend class Sender; // task #85783

public slots:
    void const slotWithSillyConst() {}

public:
    Q_INVOKABLE void const slotWithSillyConst2() {}

    // that one however should be fine
public slots:
    void slotWithVoidStar(void *) {}

private:
     myNS::Points m_points;

private slots:
     inline virtual void blub1() {}
     virtual inline void blub2() {}
};

class PropertyTestClass : public QObject
{
    Q_OBJECT
public:

    enum TestEnum { One, Two, Three };

    Q_ENUMS(TestEnum)
};

class PropertyUseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PropertyTestClass::TestEnum foo READ foo)
public:

    inline PropertyTestClass::TestEnum foo() const { return PropertyTestClass::One; }
};

#if defined(Q_MOC_RUN)
// Task #119503
#define _TASK_119503
#if !_TASK_119503
#endif
#endif

static QString srcify(const char *path)
{
#ifndef Q_OS_IRIX
    return QString(SRCDIR) + QLatin1Char('/') + QLatin1String(path);
#else
    return QString(QLatin1String(path));
#endif
}

class CtorTestClass : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE CtorTestClass(QObject *parent = 0);

    CtorTestClass(int foo);

    inline Q_INVOKABLE CtorTestClass(const QString &str)
        { m_str = str; }

    QString m_str;

protected:
    CtorTestClass(int foo, int bar, int baz);
private:
    CtorTestClass(float, float) {}
};

CtorTestClass::CtorTestClass(QObject *parent)
    : QObject(parent) {}

CtorTestClass::CtorTestClass(int, int, int) {}


class tst_Moc : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool user1 READ user1 USER true )
    Q_PROPERTY(bool user2 READ user2 USER false)
    Q_PROPERTY(bool user3 READ user3 USER userFunction())

public:
    inline tst_Moc() {}

private slots:
    void initTestCase();

    void slotWithException() throw(MyStruct);
    void dontStripNamespaces();
    void oldStyleCasts();
    void warnOnExtraSignalSlotQualifiaction();
    void uLongLong();
    void inputFileNameWithDotsButNoExtension();
    void userProperties();
    void supportConstSignals();
    void task87883();
    void multilineComments();
    void classinfoWithEscapes();
    void trNoopInClassInfo();
    void ppExpressionEvaluation();
    void arrayArguments();
    void preprocessorConditionals();
    void blackslashNewlines();
    void slotWithSillyConst();
    void testExtraData();
    void namespaceTypeProperty();
    void slotsWithVoidTemplate();
    void structQObject();
    void namespacedFlags();
    void warnOnMultipleInheritance();
    void forgottenQInterface();
    void os9Newline();
    void winNewline();
    void escapesInStringLiterals();
    void frameworkSearchPath();
    void cstyleEnums();
    void defineMacroViaCmdline();
    void invokable();
    void singleFunctionKeywordSignalAndSlot();
    void templateGtGt();
    void qprivateslots();
    void qprivateproperties();
    void inlineSlotsWithThrowDeclaration();
    void warnOnPropertyWithoutREAD();
    void constructors();
    void typenameWithUnsigned();
    void warnOnVirtualSignal();
    void QTBUG5590_dummyProperty();
    void QTBUG12260_defaultTemplate();
    void notifyError();
    void QTBUG17635_invokableAndProperty();
    void revisions();
    void warnings_data();
    void warnings();
    void privateClass();

signals:
    void sigWithUnsignedArg(unsigned foo);
    void sigWithSignedArg(signed foo);
    void sigWithConstSignedArg(const signed foo);
    void sigWithVolatileConstSignedArg(volatile const signed foo);
    void sigWithCustomType(const MyStruct);
    void constSignal1() const;
    void constSignal2(int arg) const;

private:
    bool user1() { return true; };
    bool user2() { return false; };
    bool user3() { return false; };
    bool userFunction(){ return false; };
    template <class T> void revisions_T();

private:
    QString qtIncludePath;
    class PrivateClass;
};

void tst_Moc::initTestCase()
{
#if defined(Q_OS_UNIX) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("qmake", QStringList() << "-query" << "QT_INSTALL_HEADERS");
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray output = proc.readAllStandardOutput();
    QVERIFY(!output.isEmpty());
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    qtIncludePath = QString::fromLocal8Bit(output).trimmed();
    QFileInfo fi(qtIncludePath);
    QVERIFY(fi.exists());
    QVERIFY(fi.isDir());
#endif
}

void tst_Moc::slotWithException() throw(MyStruct)
{
    // be happy
    QVERIFY(true);
}

void tst_Moc::dontStripNamespaces()
{
    Sender sender;
    Receiver receiver;

    connect(&sender, SIGNAL(send(const String::Type &)),
            &receiver, SLOT(receive(const String::Type &)));
    connect(&sender, SIGNAL(send(const Int::Type &)),
            &receiver, SLOT(receive(const Int::Type &)));

    sender.sendValue(String::Type("Hello"));
    QCOMPARE(receiver.stringCallCount, 1);
    QCOMPARE(receiver.intCallCount, 0);
    sender.sendValue(Int::Type(42));
    QCOMPARE(receiver.stringCallCount, 1);
    QCOMPARE(receiver.intCallCount, 1);
}


void tst_Moc::oldStyleCasts()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("moc", QStringList(srcify("/oldstyle-casts.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QCOMPARE(proc.readAllStandardError(), QByteArray());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-Wold-style-cast" << "-I" << "."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()), QString());
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::warnOnExtraSignalSlotQualifiaction()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("moc", QStringList(srcify("extraqualification.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, QString(SRCDIR) +
                QString("/extraqualification.h:53: Warning: Function declaration Test::badFunctionDeclaration contains extra qualification. Ignoring as signal or slot.\n") +
                QString(SRCDIR) + QString("/extraqualification.h:56: Warning: parsemaybe: Function declaration Test::anotherOne contains extra qualification. Ignoring as signal or slot.\n"));
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::uLongLong()
{
#ifndef NOLONGLONG
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    int idx = mobj->indexOfSlot("slotWithULong(ulong)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithULongLong(unsigned long long)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithULongLongP(unsigned long long*)");
    QVERIFY(idx != -1);

    idx = mobj->indexOfSlot("slotWithLong(long)");
    QVERIFY(idx != -1);
    idx = mobj->indexOfSlot("slotWithLongLong(long long)");
    QVERIFY(idx != -1);
#else
    QSKIP("long long doesn't work on MSVC6 & .NET 2002, also skipped on 2003 due to compiler version issue with moc", SkipAll);
#endif
}

void tst_Moc::inputFileNameWithDotsButNoExtension()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.setWorkingDirectory(QString(SRCDIR) + "/task71021");
    proc.start("moc", QStringList("../Header"));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QCOMPARE(proc.readAllStandardError(), QByteArray());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-I" << ".."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()), QString());
    QCOMPARE(proc.exitCode(), 0);
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::userProperties()
{
    const QMetaObject *mobj = metaObject();
    QMetaProperty property = mobj->property(mobj->indexOfProperty("user1"));
    QVERIFY(property.isValid());
    QVERIFY(property.isUser());

    property = mobj->property(mobj->indexOfProperty("user2"));
    QVERIFY(property.isValid());
    QVERIFY(!property.isUser());

    property = mobj->property(mobj->indexOfProperty("user3"));
    QVERIFY(property.isValid());
    QVERIFY(!property.isUser(this));
}

void tst_Moc::supportConstSignals()
{
    QSignalSpy spy1(this, SIGNAL(constSignal1()));
    QVERIFY(spy1.isEmpty());
    emit constSignal1();
    QCOMPARE(spy1.count(), 1);

    QSignalSpy spy2(this, SIGNAL(constSignal2(int)));
    QVERIFY(spy2.isEmpty());
    emit constSignal2(42);
    QCOMPARE(spy2.count(), 1);
    QCOMPARE(spy2.at(0).at(0).toInt(), 42);
}

#include "task87883.h"

void tst_Moc::task87883()
{
    QVERIFY(Task87883::staticMetaObject.className());
}

#include "c-comments.h"

void tst_Moc::multilineComments()
{
    QVERIFY(IfdefedClass::staticMetaObject.className());
}

void tst_Moc::classinfoWithEscapes()
{
    const QMetaObject *mobj = &TestClassinfoWithEscapes::staticMetaObject;
    QCOMPARE(mobj->methodCount() - mobj->methodOffset(), 1);

    QMetaMethod mm = mobj->method(mobj->methodOffset());
    QCOMPARE(mm.signature(), "slotWithAReallyLongName(int)");
}

void tst_Moc::trNoopInClassInfo()
{
    TestClass t;
    const QMetaObject *mobj = t.metaObject();
    QVERIFY(mobj);
    QCOMPARE(mobj->classInfoCount(), 3);
    QCOMPARE(mobj->indexOfClassInfo("help"), 0);
    QCOMPARE(QString(mobj->classInfo(0).value()), QString("Opening this will let you configure something"));
}

void tst_Moc::ppExpressionEvaluation()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    int idx = mobj->indexOfSlot("expressionEvaluationShortcut1()");
    QVERIFY(idx != -1);

    idx = mobj->indexOfSlot("expressionEvaluationShortcut2()");
    QVERIFY(idx != -1);
}

void tst_Moc::arrayArguments()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("slotWithArray(const double[3])") != -1);
    QVERIFY(mobj->indexOfSlot("slotWithNamedArray(const double[3])") != -1);
    QVERIFY(mobj->indexOfSlot("slotWithMultiArray(const double[3][4])") != -1);
    QVERIFY(mobj->indexOfSignal("signalWithArray(const double[3])") != -1);
    QVERIFY(mobj->indexOfSignal("signalWithNamedArray(const double[3])") != -1);
}

void tst_Moc::preprocessorConditionals()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("slotInElse()") != -1);
    QVERIFY(mobj->indexOfSlot("slotInIf()") != -1);
    QVERIFY(mobj->indexOfSlot("slotInLastElse()") != -1);
    QVERIFY(mobj->indexOfSlot("slotInElif()") != -1);
}

void tst_Moc::blackslashNewlines()
{
    BackslashNewlines tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("works()") != -1);
    QVERIFY(mobj->indexOfSlot("buggy()") == -1);
}

void tst_Moc::slotWithSillyConst()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("slotWithSillyConst()") != -1);
    QVERIFY(mobj->indexOfMethod("slotWithSillyConst2()") != -1);
    QVERIFY(mobj->indexOfSlot("slotWithVoidStar(void*)") != -1);
}

void tst_Moc::testExtraData()
{
    const QMetaObject *mobj = &PropertyTestClass::staticMetaObject;
    QCOMPARE(mobj->enumeratorCount(), 1);
    QCOMPARE(QByteArray(mobj->enumerator(0).name()), QByteArray("TestEnum"));

    mobj = &PropertyUseClass::staticMetaObject;
    const int idx = mobj->indexOfProperty("foo");
    QVERIFY(idx != -1);
    const QMetaProperty prop = mobj->property(idx);
    QVERIFY(prop.isValid());
    QVERIFY(prop.isEnumType());
    const QMetaEnum en = prop.enumerator();
    QCOMPARE(QByteArray(en.name()), QByteArray("TestEnum"));
}

void tst_Moc::namespaceTypeProperty()
{
    qRegisterMetaType<myNS::Points>("myNS::Points");
    TestClass tst;
    QByteArray ba = QByteArray("points");
    QVariant v = tst.property(ba);
    QVERIFY(v.isValid());
    myNS::Points p = qVariantValue<myNS::Points>(v);
    QCOMPARE(p.p1, 0xBEEF);
    QCOMPARE(p.p2, 0xBABE);
    p.p1 = 0xCAFE;
    p.p2 = 0x1EE7;
    QVERIFY(tst.setProperty(ba, qVariantFromValue(p)));
    myNS::Points pp = qVariantValue<myNS::Points>(tst.property(ba));
    QCOMPARE(p.p1, pp.p1);
    QCOMPARE(p.p2, pp.p2);
}

void tst_Moc::slotsWithVoidTemplate()
{
    SlotsWithVoidTemplateTest test;
    QVERIFY(QObject::connect(&test, SIGNAL(myVoidSignal(void)),
                             &test, SLOT(dummySlot(void))));
    QVERIFY(QObject::connect(&test, SIGNAL(mySignal(const TestTemplate<void> &)),
                             &test, SLOT(anotherSlot(const TestTemplate<void> &))));
}

void tst_Moc::structQObject()
{
    StructQObject o;
    QCOMPARE(QByteArray(o.metaObject()->className()), QByteArray("StructQObject"));
}

#include "namespaced-flags.h"

Q_DECLARE_METATYPE(QList<Foo::Bar::Flags>);

void tst_Moc::namespacedFlags()
{
    Foo::Baz baz;
    Foo::Bar bar;

    bar.setFlags(Foo::Bar::Read | Foo::Bar::Write);
    QVERIFY(baz.flags() != bar.flags());

    const QVariant v = bar.property("flags");
    QVERIFY(v.isValid());
    QVERIFY(baz.setProperty("flags", v));
    QVERIFY(baz.flags() == bar.flags());

    QList<Foo::Bar::Flags> l;
    l << baz.flags();
    QVariant v2 = baz.setProperty("flagsList", QVariant::fromValue(l));
    QCOMPARE(l, baz.flagsList());
    QCOMPARE(l, qvariant_cast<QList<Foo::Bar::Flags> >(baz.property("flagsList")));
}

void tst_Moc::warnOnMultipleInheritance()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    QStringList args;
    args << "-I" << qtIncludePath + "/QtGui"
         << srcify("warn-on-multiple-qobject-subclasses.h");
    proc.start("moc", args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, QString(SRCDIR) +
                QString("/warn-on-multiple-qobject-subclasses.h:53: Warning: Class Bar inherits from two QObject subclasses QWidget and Foo. This is not supported!\n"));
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::forgottenQInterface()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    QStringList args;
    args << "-I" << qtIncludePath + "/QtCore"
         << srcify("forgotten-qinterface.h");
    proc.start("moc", args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, QString(SRCDIR) +
                QString("/forgotten-qinterface.h:55: Warning: Class Test implements the interface MyInterface but does not list it in Q_INTERFACES. qobject_cast to MyInterface will not work!\n"));
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::os9Newline()
{
#if !defined(SKIP_NEWLINE_TEST)
    const QMetaObject &mo = Os9Newlines::staticMetaObject;
    QVERIFY(mo.indexOfSlot("testSlot()") != -1);
    QFile f(srcify("os9-newlines.h"));
    QVERIFY(f.open(QIODevice::ReadOnly)); // no QIODevice::Text!
    QByteArray data = f.readAll();
    f.close();
    QVERIFY(!data.contains('\n'));
    QVERIFY(data.contains('\r'));
#endif
}

void tst_Moc::winNewline()
{
#if !defined(SKIP_NEWLINE_TEST)
    const QMetaObject &mo = WinNewlines::staticMetaObject;
    QVERIFY(mo.indexOfSlot("testSlot()") != -1);
    QFile f(srcify("win-newlines.h"));
    QVERIFY(f.open(QIODevice::ReadOnly)); // no QIODevice::Text!
    QByteArray data = f.readAll();
    f.close();
    for (int i = 0; i < data.count(); ++i) {
        if (data.at(i) == QLatin1Char('\r')) {
            QVERIFY(i < data.count() - 1);
            ++i;
            QVERIFY(data.at(i) == '\n');
        } else {
            QVERIFY(data.at(i) != '\n');
        }
    }
#endif
}

void tst_Moc::escapesInStringLiterals()
{
    const QMetaObject &mo = StringLiterals::staticMetaObject;
    QCOMPARE(mo.classInfoCount(), 3);

    int idx = mo.indexOfClassInfo("Test");
    QVERIFY(idx != -1);
    QMetaClassInfo info = mo.classInfo(idx);
    QCOMPARE(QByteArray(info.value()),
             QByteArray("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\x53"));

    QVERIFY(idx != -1);
    idx = mo.indexOfClassInfo("Test2");
    info = mo.classInfo(idx);
    QCOMPARE(QByteArray(info.value()),
             QByteArray("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\123"));

    QVERIFY(idx != -1);
    idx = mo.indexOfClassInfo("Test3");
    info = mo.classInfo(idx);
    QCOMPARE(QByteArray(info.value()),
             QByteArray("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\nb"));
}

void tst_Moc::frameworkSearchPath()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_UNIX) && !defined(QT_NO_PROCESS)
    QStringList args;
    args << "-F" << srcify(".")
         << srcify("interface-from-framework.h")
         ;

    QProcess proc;
    proc.start("moc", args);
    bool finished = proc.waitForFinished();
    if (!finished)
        qWarning("waitForFinished failed. QProcess error: %d", (int)proc.error());
    QVERIFY(finished);
    if (proc.exitCode() != 0) {
        qDebug() << proc.readAllStandardError();
    }
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
#else
    QSKIP("Only tested/relevant on unixy platforms", SkipAll);
#endif
}

void tst_Moc::cstyleEnums()
{
    const QMetaObject &obj = CStyleEnums::staticMetaObject;
    QCOMPARE(obj.enumeratorCount(), 1);
    QMetaEnum metaEnum = obj.enumerator(0);
    QCOMPARE(metaEnum.name(), "Baz");
    QCOMPARE(metaEnum.keyCount(), 2);
    QCOMPARE(metaEnum.key(0), "Foo");
    QCOMPARE(metaEnum.key(1), "Bar");
}

void tst_Moc::templateGtGt()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("moc", QStringList(srcify("template-gtgt.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QVERIFY(mocWarning.isEmpty());
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::defineMacroViaCmdline()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;

    QStringList args;
    args << "-DFOO";
    args << srcify("macro-on-cmdline.h");

    proc.start("moc", args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::invokable()
{
    {
        const QMetaObject &mobj = InvokableBeforeReturnType::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 5);
        QVERIFY(mobj.method(4).signature() == QByteArray("foo()"));
    }

    {
        const QMetaObject &mobj = InvokableBeforeInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QVERIFY(mobj.method(4).signature() == QByteArray("foo()"));
        QVERIFY(mobj.method(5).signature() == QByteArray("bar()"));
    }
}

void tst_Moc::singleFunctionKeywordSignalAndSlot()
{
    {
        const QMetaObject &mobj = SingleFunctionKeywordBeforeReturnType::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QVERIFY(mobj.method(4).signature() == QByteArray("mySignal()"));
        QVERIFY(mobj.method(5).signature() == QByteArray("mySlot()"));
    }

    {
        const QMetaObject &mobj = SingleFunctionKeywordBeforeInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QVERIFY(mobj.method(4).signature() == QByteArray("mySignal()"));
        QVERIFY(mobj.method(5).signature() == QByteArray("mySlot()"));
    }

    {
        const QMetaObject &mobj = SingleFunctionKeywordAfterInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QVERIFY(mobj.method(4).signature() == QByteArray("mySignal()"));
        QVERIFY(mobj.method(5).signature() == QByteArray("mySlot()"));
    }
}

#include "qprivateslots.h"

void tst_Moc::qprivateslots()
{
    TestQPrivateSlots tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("_q_privateslot()") != -1);
    QVERIFY(mobj->indexOfMethod("method1()") != -1); //tast204730
}

class PrivatePropertyTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo)
    Q_PRIVATE_PROPERTY(d, int bar READ bar WRITE setBar)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, int plop READ plop WRITE setPlop)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d_func(), int baz READ baz WRITE setBaz)
    class MyDPointer {
    public:
        MyDPointer() : mBar(0), mPlop(0) {}
        int bar() { return mBar ; }
        void setBar(int value) { mBar = value; }
        int plop() { return mPlop ; }
        void setPlop(int value) { mPlop = value; }
        int baz() { return mBaz ; }
        void setBaz(int value) { mBaz = value; }
    private:
        int mBar;
        int mPlop;
        int mBaz;
    };
public:
    PrivatePropertyTest() : mFoo(0), d (new MyDPointer) {}
    int foo() { return mFoo ; }
    void setFoo(int value) { mFoo = value; }
    MyDPointer *d_func() {return d;}
private:
    int mFoo;
    MyDPointer *d;
};


void tst_Moc::qprivateproperties()
{
    PrivatePropertyTest test;

    test.setProperty("foo", 1);
    QCOMPARE(test.property("foo"), qVariantFromValue(1));

    test.setProperty("bar", 2);
    QCOMPARE(test.property("bar"), qVariantFromValue(2));

    test.setProperty("plop", 3);
    QCOMPARE(test.property("plop"), qVariantFromValue(3));

    test.setProperty("baz", 4);
    QCOMPARE(test.property("baz"), qVariantFromValue(4));

}

#include "task189996.h"

void InlineSlotsWithThrowDeclaration::c() throw() {}

void tst_Moc::inlineSlotsWithThrowDeclaration()
{
    InlineSlotsWithThrowDeclaration tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("a()") != -1);
    QVERIFY(mobj->indexOfSlot("b()") != -1);
    QVERIFY(mobj->indexOfSlot("c()") != -1);
    QVERIFY(mobj->indexOfSlot("d()") != -1);
    QVERIFY(mobj->indexOfSlot("e()") != -1);
}

void tst_Moc::warnOnPropertyWithoutREAD()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("moc", QStringList(srcify("warn-on-property-without-read.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, QString(SRCDIR) +
                QString("/warn-on-property-without-read.h:46: Warning: Property declaration foo has no READ accessor function. The property will be invalid.\n"));
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

void tst_Moc::constructors()
{
    const QMetaObject *mo = &CtorTestClass::staticMetaObject;
    QCOMPARE(mo->constructorCount(), 3);
    {
        QMetaMethod mm = mo->constructor(0);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.signature(), "CtorTestClass(QObject*)");
        QCOMPARE(mm.typeName(), "");
        QList<QByteArray> paramNames = mm.parameterNames();
        QCOMPARE(paramNames.size(), 1);
        QCOMPARE(paramNames.at(0), QByteArray("parent"));
        QList<QByteArray> paramTypes = mm.parameterTypes();
        QCOMPARE(paramTypes.size(), 1);
        QCOMPARE(paramTypes.at(0), QByteArray("QObject*"));
    }
    {
        QMetaMethod mm = mo->constructor(1);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.signature(), "CtorTestClass()");
        QCOMPARE(mm.typeName(), "");
        QCOMPARE(mm.parameterNames().size(), 0);
        QCOMPARE(mm.parameterTypes().size(), 0);
    }
    {
        QMetaMethod mm = mo->constructor(2);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.signature(), "CtorTestClass(QString)");
        QCOMPARE(mm.typeName(), "");
        QList<QByteArray> paramNames = mm.parameterNames();
        QCOMPARE(paramNames.size(), 1);
        QCOMPARE(paramNames.at(0), QByteArray("str"));
        QList<QByteArray> paramTypes = mm.parameterTypes();
        QCOMPARE(paramTypes.size(), 1);
        QCOMPARE(paramTypes.at(0), QByteArray("QString"));
    }

    QCOMPARE(mo->indexOfConstructor("CtorTestClass(QObject*)"), 0);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass()"), 1);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass(QString)"), 2);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass2(QObject*)"), -1);
    QCOMPARE(mo->indexOfConstructor("CtorTestClass(float,float)"), -1);

    QObject *o1 = mo->newInstance();
    QVERIFY(o1 != 0);
    QCOMPARE(o1->parent(), (QObject*)0);
    QVERIFY(qobject_cast<CtorTestClass*>(o1) != 0);

    QObject *o2 = mo->newInstance(Q_ARG(QObject*, o1));
    QVERIFY(o2 != 0);
    QCOMPARE(o2->parent(), o1);

    QString str = QString::fromLatin1("hello");
    QObject *o3 = mo->newInstance(Q_ARG(QString, str));
    QVERIFY(o3 != 0);
    QCOMPARE(qobject_cast<CtorTestClass*>(o3)->m_str, str);

    {
        //explicit constructor
        QObject *o = QObject::staticMetaObject.newInstance();
        QVERIFY(o);
        delete o;
    }
}

#include "task234909.h"

#include "task240368.h"

void tst_Moc::typenameWithUnsigned()
{
    TypenameWithUnsigned tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("a(uint)") != -1);
    QVERIFY(mobj->indexOfSlot("b(uint)") != -1);
    QVERIFY(mobj->indexOfSlot("c(uint*)") != -1);
    QVERIFY(mobj->indexOfSlot("d(uint*)") != -1);
    QVERIFY(mobj->indexOfSlot("e(uint&)") != -1);
    QVERIFY(mobj->indexOfSlot("f(uint&)") != -1);
    QVERIFY(mobj->indexOfSlot("g(unsigned1)") != -1);
    QVERIFY(mobj->indexOfSlot("h(unsigned1)") != -1);
    QVERIFY(mobj->indexOfSlot("i(uint,unsigned1)") != -1);
    QVERIFY(mobj->indexOfSlot("j(unsigned1,uint)") != -1);
    QVERIFY(mobj->indexOfSlot("k(unsignedQImage)") != -1);
    QVERIFY(mobj->indexOfSlot("l(unsignedQImage)") != -1);
}


void tst_Moc::warnOnVirtualSignal()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("moc", QStringList(srcify("pure-virtual-signals.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, QString(SRCDIR) + QString("/pure-virtual-signals.h:48: Warning: Signals cannot be declared virtual\n") +
                         QString(SRCDIR) + QString("/pure-virtual-signals.h:50: Warning: Signals cannot be declared virtual\n"));
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}


class QTBUG5590_DummyObject: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dummy)
};

class QTBUG5590_PropertyObject: public QTBUG5590_DummyObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(int value2 READ value2 WRITE setValue2)

    public:
        QTBUG5590_PropertyObject() :  m_value(85), m_value2(40) { }
        int value() const { return m_value; }
        void setValue(int value) { m_value = value; }
        int value2() const { return m_value2; }
        void setValue2(int value) { m_value2 = value; }
    private:
        int m_value, m_value2;
};

void tst_Moc::QTBUG5590_dummyProperty()
{
    QTBUG5590_PropertyObject o;
    QCOMPARE(o.property("value").toInt(), 85);
    QCOMPARE(o.property("value2").toInt(), 40);
    o.setProperty("value", 32);
    QCOMPARE(o.value(), 32);
    o.setProperty("value2", 82);
    QCOMPARE(o.value2(), 82);
}

class QTBUG7421_ReturnConstTemplate: public QObject
{ Q_OBJECT
public slots:
        const QList<int> returnConstTemplate1() { return QList<int>(); }
        QList<int> const returnConstTemplate2() { return QList<int>(); }
        const int returnConstInt() { return 0; }
        const QString returnConstString(const QString s) { return s; }
        QString const returnConstString2( QString const s) { return s; }
};

class QTBUG9354_constInName: public QObject
{ Q_OBJECT
public slots:
    void slotChooseScientificConst0(struct science_constant const &) {};
    void foo(struct science_const const &) {};
    void foo(struct constconst const &) {};
    void foo(struct constconst *) {};
    void foo(struct const_ *) {};
};


template<typename T1, typename T2>
class TestTemplate2
{
};

class QTBUG11647_constInTemplateParameter : public QObject
{ Q_OBJECT
public slots:
    void testSlot(TestTemplate2<const int, const short*>) {}
    void testSlot2(TestTemplate2<int, short const * const >) {}
    void testSlot3(TestTemplate2<TestTemplate2 < const int, const short* > const *,
                                TestTemplate2< TestTemplate2 < void, int > , unsigned char *> > ) {}

signals:
    void testSignal(TestTemplate2<const int, const short*>);
};

class QTBUG12260_defaultTemplate_Object : public QObject
{ Q_OBJECT
public slots:
#if !(defined(Q_CC_GNU) && __GNUC__ == 4 && __GNUC_MINOR__ <= 3) || defined(Q_MOC_RUN)
    void doSomething(QHash<QString, QVariant> values = QHash<QString, QVariant>() ) { Q_UNUSED(values); }
#else
    // we want to test the previous function, but gcc < 4.4 seemed to have a bug similar to the one moc has.
    typedef QHash<QString, QVariant> WorkaroundGCCBug;
    void doSomething(QHash<QString, QVariant> values = WorkaroundGCCBug() ) { Q_UNUSED(values); }
#endif

    void doAnotherThing(bool a = (1 < 3), bool b = (1 > 4)) { Q_UNUSED(a); Q_UNUSED(b); }
};


void tst_Moc::QTBUG12260_defaultTemplate()
{
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doSomething(QHash<QString,QVariant>)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doAnotherThing(bool,bool)") != -1);
}

void tst_Moc::notifyError()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start("moc", QStringList(srcify("error-on-wrong-notify.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 1);
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(mocOut.isEmpty());
    QString mocError = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocError, QString(SRCDIR) +
        QString("/error-on-wrong-notify.h:52: Error: NOTIFY signal 'fooChanged' of property 'foo' does not exist in class ClassWithWrongNOTIFY.\n"));
#else
    QSKIP("Only tested on linux/gcc", SkipAll);
#endif
}

class QTBUG_17635_InvokableAndProperty : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(int numberOfEggs READ numberOfEggs)
    Q_PROPERTY(int numberOfChickens READ numberOfChickens)
    Q_INVOKABLE QString getEgg(int index) { return QString::fromLatin1("Egg"); }
    Q_INVOKABLE QString getChicken(int index) { return QString::fromLatin1("Chicken"); }
    int numberOfEggs() { return 2; }
    int numberOfChickens() { return 4; }
};

void tst_Moc::QTBUG17635_invokableAndProperty()
{
    //Moc used to fail parsing Q_INVOKABLE if they were dirrectly following a Q_PROPERTY;
    QTBUG_17635_InvokableAndProperty mc;
    QString val;
    QMetaObject::invokeMethod(&mc, "getEgg", Q_RETURN_ARG(QString, val), Q_ARG(int, 10));
    QCOMPARE(val, QString::fromLatin1("Egg"));
    QMetaObject::invokeMethod(&mc, "getChicken", Q_RETURN_ARG(QString, val), Q_ARG(int, 10));
    QCOMPARE(val, QString::fromLatin1("Chicken"));
    QVERIFY(mc.metaObject()->indexOfProperty("numberOfEggs") != -1);
    QVERIFY(mc.metaObject()->indexOfProperty("numberOfChickens") != -1);
}

// If changed, update VersionTestNotify below
class VersionTest : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ foo)
    Q_PROPERTY(int prop2 READ foo REVISION 2)
    Q_ENUMS(TestEnum);

public:
    int foo() const { return 0; }

    Q_INVOKABLE void method1() {}
    Q_INVOKABLE Q_REVISION(4) void method2() {}

    enum TestEnum { One, Two };

public slots:
    void slot1() {}
    Q_REVISION(3) void slot2() {}

signals:
    void signal1();
    Q_REVISION(5) void signal2();

public slots Q_REVISION(6):
    void slot3() {}
    void slot4() {}

signals Q_REVISION(7):
    void signal3();
    void signal4();
};

// If changed, update VersionTest above
class VersionTestNotify : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int prop1 READ foo NOTIFY fooChanged)
    Q_PROPERTY(int prop2 READ foo REVISION 2)
    Q_ENUMS(TestEnum);

public:
    int foo() const { return 0; }

    Q_INVOKABLE void method1() {}
    Q_INVOKABLE Q_REVISION(4) void method2() {}

    enum TestEnum { One, Two };

public slots:
    void slot1() {}
    Q_REVISION(3) void slot2() {}

signals:
    void fooChanged();
    void signal1();
    Q_REVISION(5) void signal2();

public slots Q_REVISION(6):
    void slot3() {}
    void slot4() {}

signals Q_REVISION(7):
    void signal3();
    void signal4();
};

template <class T>
void tst_Moc::revisions_T()
{
    int idx = T::staticMetaObject.indexOfProperty("prop1");
    QVERIFY(T::staticMetaObject.property(idx).revision() == 0);
    idx = T::staticMetaObject.indexOfProperty("prop2");
    QVERIFY(T::staticMetaObject.property(idx).revision() == 2);

    idx = T::staticMetaObject.indexOfMethod("method1()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 0);
    idx = T::staticMetaObject.indexOfMethod("method2()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 4);

    idx = T::staticMetaObject.indexOfSlot("slot1()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 0);
    idx = T::staticMetaObject.indexOfSlot("slot2()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 3);

    idx = T::staticMetaObject.indexOfSlot("slot3()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 6);
    idx = T::staticMetaObject.indexOfSlot("slot4()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 6);

    idx = T::staticMetaObject.indexOfSignal("signal1()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 0);
    idx = T::staticMetaObject.indexOfSignal("signal2()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 5);

    idx = T::staticMetaObject.indexOfSignal("signal3()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 7);
    idx = T::staticMetaObject.indexOfSignal("signal4()");
    QVERIFY(T::staticMetaObject.method(idx).revision() == 7);

    idx = T::staticMetaObject.indexOfEnumerator("TestEnum");
    QCOMPARE(T::staticMetaObject.enumerator(idx).keyCount(), 2);
    QCOMPARE(T::staticMetaObject.enumerator(idx).key(0), "One");
}

// test using both class that has properties with and without NOTIFY signals
void tst_Moc::revisions()
{
    revisions_T<VersionTest>();
    revisions_T<VersionTestNotify>();
}

void tst_Moc::warnings_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<int>("exitCode");
    QTest::addColumn<QString>("expectedStdOut");
    QTest::addColumn<QString>("expectedStdErr");

    // empty input should result in "no relevant classes" note
    QTest::newRow("No relevant classes")
        << QByteArray(" ")
        << QStringList()
        << 0
        << QString()
        << QString("standard input:0: Note: No relevant classes found. No output generated.");

    // passing "-nn" should suppress "no relevant classes" note
    QTest::newRow("-nn")
        << QByteArray(" ")
        << (QStringList() << "-nn")
        << 0
        << QString()
        << QString();

    // passing "-nw" should also suppress "no relevant classes" note
    QTest::newRow("-nw")
        << QByteArray(" ")
        << (QStringList() << "-nw")
        << 0
        << QString()
        << QString();

    // This should output a warning
    QTest::newRow("Invalid property warning")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << QStringList()
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1: Warning: Property declaration x has no READ accessor function. The property will be invalid.");

    // Passing "-nn" should NOT suppress the warning
    QTest::newRow("Invalid property warning")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << (QStringList() << "-nn")
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1: Warning: Property declaration x has no READ accessor function. The property will be invalid.");

    // Passing "-nw" should suppress the warning
    QTest::newRow("Invalid property warning")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << (QStringList() << "-nw")
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString();

    // This should output an error
    QTest::newRow("Does not inherit QObject")
        << QByteArray("class X { Q_OBJECT };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:1: Error: Class contains Q_OBJECT macro but does not inherit from QObject");

    // "-nn" should not suppress the error
    QTest::newRow("Does not inherit QObject with -nn")
        << QByteArray("class X { Q_OBJECT };")
        << (QStringList() << "-nn")
        << 1
        << QString()
        << QString("standard input:1: Error: Class contains Q_OBJECT macro but does not inherit from QObject");

    // "-nw" should not suppress the error
    QTest::newRow("Does not inherit QObject with -nn")
        << QByteArray("class X { Q_OBJECT };")
        << (QStringList() << "-nw")
        << 1
        << QString()
        << QString("standard input:1: Error: Class contains Q_OBJECT macro but does not inherit from QObject");
}

void tst_Moc::warnings()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled", SkipAll);
#endif

    QFETCH(QByteArray, input);
    QFETCH(QStringList, args);
    QFETCH(int, exitCode);
    QFETCH(QString, expectedStdOut);
    QFETCH(QString, expectedStdErr);

#ifdef Q_CC_MSVC
    // for some reasons, moc compiled with MSVC uses a different output format
    QRegExp lineNumberRe(":(\\d+):");
    lineNumberRe.setMinimal(true);
    expectedStdErr.replace(lineNumberRe, "(\\1):");
#endif

    QProcess proc;
    proc.start("moc", args);
    QVERIFY(proc.waitForStarted());

    QCOMPARE(proc.write(input), qint64(input.size()));

    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());

    QCOMPARE(proc.exitCode(), exitCode);
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

    // magic value "IGNORE_ALL_STDOUT" ignores stdout
    if (expectedStdOut != "IGNORE_ALL_STDOUT")
        QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed(), expectedStdOut);
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()).trimmed(), expectedStdErr);

    }

class tst_Moc::PrivateClass : public QObject {
    Q_PROPERTY(int someProperty READ someSlot WRITE someSlot2)
Q_OBJECT
Q_SIGNALS:
    void someSignal();
public Q_SLOTS:
    int someSlot() { return 1; }
    void someSlot2(int) {}
public:
    Q_INVOKABLE PrivateClass()  {}
};

void tst_Moc::privateClass()
{
    QVERIFY(PrivateClass::staticMetaObject.indexOfConstructor("PrivateClass()") == 0);
    QVERIFY(PrivateClass::staticMetaObject.indexOfSignal("someSignal()") > 0);
}


QTEST_APPLESS_MAIN(tst_Moc)
#include "tst_moc.moc"

