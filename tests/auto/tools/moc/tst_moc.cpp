/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
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
#include <stdio.h>
#include <qobject.h>
#include <qmetaobject.h>

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
#include "cxx11-enums.h"
#include "cxx11-final-classes.h"
#include "cxx11-explicit-override-control.h"

#include "parse-defines.h"
#include "related-metaobjects-in-namespaces.h"
#include "related-metaobjects-in-gadget.h"
#include "related-metaobjects-name-conflict.h"

#include "non-gadget-parent-class.h"
#include "grand-parent-gadget-class.h"
#include "namespace.h"

#ifdef Q_MOC_RUN
// check that moc can parse these constructs, they are being used in Windows winsock2.h header
#define STRING_HASH_HASH(x) ("foo" ## x ## "bar")
const char *string_hash_hash = STRING_HASH_HASH("baz");
#endif

Q_DECLARE_METATYPE(const QMetaObject*);

namespace TestNonQNamespace {

struct TestGadget {
    Q_GADGET
    Q_CLASSINFO("key", "value")
public:
    enum class TestGEnum1 {
        Key1 = 11,
        Key2
    };
    Q_ENUM(TestGEnum1)

    enum class TestGEnum2 {
        Key1 = 17,
        Key2
    };
    Q_ENUM(TestGEnum2)
};

}

namespace TestQNamespace {
    Q_NAMESPACE
    enum class TestEnum1 {
        Key1 = 11,
        Key2
    };
    Q_ENUM_NS(TestEnum1)

    enum class TestEnum2 {
        Key1 = 17,
        Key2
    };
    Q_ENUM_NS(TestEnum2)

    // try to dizzy moc by adding a struct in between
    struct TestGadget {
        Q_GADGET
    public:
        enum class TestGEnum1 {
            Key1 = 13,
            Key2
        };
        enum class TestGEnum2 {
            Key1 = 23,
            Key2
        };
        Q_ENUM(TestGEnum1)
        Q_ENUM(TestGEnum2)
    };

    enum class TestFlag1 {
        None = 0,
        Flag1 = 1,
        Flag2 = 2,
        Any = Flag1 | Flag2
    };
    Q_FLAG_NS(TestFlag1)

    enum class TestFlag2 {
        None = 0,
        Flag1 = 4,
        Flag2 = 8,
        Any = Flag1 | Flag2
    };
    Q_FLAG_NS(TestFlag2)
}

QT_USE_NAMESPACE

template <bool b> struct QTBUG_31218 {};
struct QTBUG_31218_Derived : QTBUG_31218<-1<0> {};

#if defined(Q_MOC_RUN)
 class QTBUG_45790 : Bug() { };
#endif

class CreatableGadget
{
    Q_GADGET
public:
    Q_INVOKABLE CreatableGadget()
    {
        CreatableGadget::qt_static_metacall((QObject*)this, QMetaObject::ReadProperty, -1, Q_NULLPTR);
    }
};

CreatableGadget creatableGadget; // Force the compiler to use the constructor

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

namespace {

    class GadgetInUnnamedNS
    {
        Q_GADGET
        Q_PROPERTY(int x READ x WRITE setX)
        Q_PROPERTY(int y READ y WRITE setY)
    public:
        explicit GadgetInUnnamedNS(int x, int y) : m_x(x), m_y(y) {}
        int x() const { return m_x; }
        int y() const { return m_y; }
        void setX(int x) { m_x = x; }
        void setY(int y) { m_y = y; }

    private:
        int m_x, m_y;
    };

    class ObjectInUnnamedNS : public QObject
    {
        Q_OBJECT
    public:
        explicit ObjectInUnnamedNS(QObject *parent = Q_NULLPTR) : QObject(parent) {}
    };

}

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

    bool operator< ( const Sender & ) const { /* QTBUG-36834 */ return true;}
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
    Q_CLASSINFO("cpp c*/omment", "f/*oo")
    Q_CLASSINFO("endswith\\", "Or?\?/")
    Q_CLASSINFO("newline\n inside\n", "Or \r")
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

    void slotWithULongLong(unsigned long long) {}
    void slotWithULongLongP(unsigned long long*) {}
    void slotWithULong(unsigned long) {}
    void slotWithLongLong(long long) {}
    void slotWithLong(long) {}

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
    void slotWithNamedArray(const double namedArray[3]) { Q_UNUSED(namedArray); }
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
    void signalWithIterator(QList<QUrl>::iterator);
    void signalWithListPointer(QList<QUrl>*); //QTBUG-31002

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

#define MACRO_DEFINED

#if !(defined MACRO_UNDEF || defined MACRO_DEFINED) || 1
    void signalInIf1();
#else
    void doNotExist();
#endif
#if !(!defined MACRO_UNDEF || !defined MACRO_DEFINED) && 1
    void doNotExist();
#else
    void signalInIf2();
#endif
#if !(!defined (MACRO_DEFINED) || !defined (MACRO_UNDEF)) && 1
    void doNotExist();
#else
    void signalInIf3();
#endif

# //QTBUG-22717
 # /*  */
#

 # \

//
public slots:
    void const slotWithSillyConst() {}

public:
    Q_INVOKABLE void const slotWithSillyConst2() {}
    Q_INVOKABLE QObject& myInvokableReturningRef()
    { return *this; }
    Q_INVOKABLE const QObject& myInvokableReturningConstRef() const
    { return *this; }


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

    Q_ENUM(TestEnum)
};

class PropertyUseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(PropertyTestClass::TestEnum foo READ foo)
public:

    inline PropertyTestClass::TestEnum foo() const { return PropertyTestClass::One; }
};

class EnumSourceClass : public QObject
{
    Q_OBJECT

public:
    enum TestEnum { Value = 37 };

    Q_ENUM(TestEnum)
};

class EnumUserClass : public QObject
{
    Q_OBJECT

public:
    Q_ENUMS(EnumSourceClass::TestEnum)
};

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

class PrivatePropertyTest;

class tst_Moc : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool user1 READ user1 USER true )
    Q_PROPERTY(bool user2 READ user2 USER false)
    Q_PROPERTY(bool user3 READ user3 USER userFunction())
    Q_PROPERTY(QString member1 MEMBER sMember)
    Q_PROPERTY(QString member2 MEMBER sMember READ member2)
    Q_PROPERTY(QString member3 MEMBER sMember WRITE setMember3)
    Q_PROPERTY(QString member4 MEMBER sMember NOTIFY member4Changed)
    Q_PROPERTY(QString member5 MEMBER sMember NOTIFY member5Changed)
    Q_PROPERTY(QString member6 MEMBER sConst CONSTANT)
    Q_PROPERTY(QString sub1 MEMBER (sub.m_string))
    Q_PROPERTY(QString sub2 READ (sub.string) WRITE (sub.setString))

public:
    inline tst_Moc() : sConst("const") {}

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
    void testExtraDataForEnum();
    void namespaceTypeProperty();
    void slotsWithVoidTemplate();
    void structQObject();
    void namespacedFlags();
    void warnOnMultipleInheritance();
    void ignoreOptionClashes();
    void forgottenQInterface();
    void os9Newline();
    void winNewline();
    void escapesInStringLiterals();
    void frameworkSearchPath();
    void cstyleEnums();
    void defineMacroViaCmdline();
    void defineMacroViaForcedInclude();
    void defineMacroViaForcedIncludeRelative();
    void environmentIncludePaths_data();
    void environmentIncludePaths();
    void specifyMetaTagsFromCmdline();
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
    void cxx11Enums_data();
    void cxx11Enums();
    void returnRefs();
    void memberProperties_data();
    void memberProperties();
    void memberProperties2();
    void privateSignalConnection();
    void finalClasses_data();
    void finalClasses();
    void explicitOverrideControl_data();
    void explicitOverrideControl();
    void autoPropertyMetaTypeRegistration();
    void autoMethodArgumentMetaTypeRegistration();
    void autoSignalSpyMetaTypeRegistration();
    void parseDefines();
    void preprocessorOnly();
    void unterminatedFunctionMacro();
    void QTBUG32933_relatedObjectsDontIncludeItself();
    void writeEnumFromUnrelatedClass();
    void relatedMetaObjectsWithinNamespaces();
    void relatedMetaObjectsInGadget();
    void relatedMetaObjectsNameConflict_data();
    void relatedMetaObjectsNameConflict();
    void strignLiteralsInMacroExtension();
    void unnamedNamespaceObjectsAndGadgets();
    void veryLongStringData();
    void gadgetHierarchy();
    void optionsFileError_data();
    void optionsFileError();
    void testQNamespace();

signals:
    void sigWithUnsignedArg(unsigned foo);
    void sigWithSignedArg(signed foo);
    void sigWithConstSignedArg(const signed foo);
    void sigWithVolatileConstSignedArg(volatile const signed foo);
    void sigWithCustomType(const MyStruct);
    void constSignal1() const;
    void constSignal2(int arg) const;
    void member4Changed();
    void member5Changed(const QString &newVal);

private:
    bool user1() { return true; };
    bool user2() { return false; };
    bool user3() { return false; };
    bool userFunction(){ return false; };
    template <class T> void revisions_T();
    QString member2() const { return sMember; }
    void setMember3( const QString &sVal ) { sMember = sVal; }

private:
    QString m_moc;
    QString m_sourceDirectory;
    QString qtIncludePath;
    class PrivateClass;
    QString sMember;
    const QString sConst;
    PrivatePropertyTest *pPPTest;

    struct {
        QString m_string;
        void setString(const QString &s) { m_string = s; }
        QString string() { return m_string; }
    } sub;

};

void tst_Moc::initTestCase()
{
    QString binpath = QLibraryInfo::location(QLibraryInfo::BinariesPath);
    QString qmake = QString("%1/qmake").arg(binpath);
    m_moc = QString("%1/moc").arg(binpath);

    const QString testHeader = QFINDTESTDATA("backslash-newlines.h");
    QVERIFY(!testHeader.isEmpty());
    m_sourceDirectory = QFileInfo(testHeader).absolutePath();
#if defined(Q_OS_UNIX) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start(qmake, QStringList() << "-query" << "QT_INSTALL_HEADERS");
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
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start(m_moc, QStringList(m_sourceDirectory + QStringLiteral("/oldstyle-casts.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QCOMPARE(proc.readAllStandardError(), QByteArray());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-Wold-style-cast" << "-I" << "."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-fPIC" << "-std=c++11" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()), QString());
    QCOMPARE(proc.exitCode(), 0);
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::warnOnExtraSignalSlotQualifiaction()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/extraqualification.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":43: Warning: Function declaration Test::badFunctionDeclaration contains extra qualification. Ignoring as signal or slot.\n") +
                header + QString(":46: Warning: parsemaybe: Function declaration Test::anotherOne contains extra qualification. Ignoring as signal or slot.\n"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::uLongLong()
{
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
}

void tst_Moc::inputFileNameWithDotsButNoExtension()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.setWorkingDirectory(m_sourceDirectory + QStringLiteral("/task71021"));
    proc.start(m_moc, QStringList("../Header"));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QCOMPARE(proc.readAllStandardError(), QByteArray());

    QStringList args;
    args << "-c" << "-x" << "c++" << "-I" << ".."
         << "-I" << qtIncludePath << "-o" << "/dev/null" << "-fPIC" << "-std=c++11" << "-";
    proc.start("gcc", args);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()), QString());
    QCOMPARE(proc.exitCode(), 0);
#else
    QSKIP("Only tested on linux/gcc");
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

    QCOMPARE(mobj->classInfoCount(), 5);
    QCOMPARE(mobj->classInfo(2).name(), "cpp c*/omment");
    QCOMPARE(mobj->classInfo(2).value(), "f/*oo");
    QCOMPARE(mobj->classInfo(3).name(), "endswith\\");
    QCOMPARE(mobj->classInfo(3).value(), "Or?\?/");
    QCOMPARE(mobj->classInfo(4).name(), "newline\n inside\n");
    QCOMPARE(mobj->classInfo(4).value(), "Or \r");

    QMetaMethod mm = mobj->method(mobj->methodOffset());
    QCOMPARE(mm.methodSignature(), QByteArray("slotWithAReallyLongName(int)"));
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
    QVERIFY(mobj->indexOfSignal("signalInIf1()") != -1);
    QVERIFY(mobj->indexOfSignal("signalInIf2()") != -1);
    QVERIFY(mobj->indexOfSignal("signalInIf3()") != -1);
    QCOMPARE(mobj->indexOfSignal("doNotExist()"), -1);
}

void tst_Moc::blackslashNewlines()
{
    BackslashNewlines tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfSlot("works()") != -1);
    QCOMPARE(mobj->indexOfSlot("buggy()"), -1);
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

// QTBUG-20639 - Accept non-local enums for QML signal/slot parameters.
void tst_Moc::testExtraDataForEnum()
{
    const QMetaObject *mobjSource = &EnumSourceClass::staticMetaObject;
    QCOMPARE(mobjSource->enumeratorCount(), 1);
    QCOMPARE(QByteArray(mobjSource->enumerator(0).name()), QByteArray("TestEnum"));

    const QMetaObject *mobjUser = &EnumUserClass::staticMetaObject;
    QCOMPARE(mobjUser->enumeratorCount(), 0);

    const QMetaObject * const *objects = mobjUser->d.relatedMetaObjects;
    QVERIFY(objects);
    QCOMPARE(objects[0], mobjSource);
    QVERIFY(!objects[1]);
}

void tst_Moc::namespaceTypeProperty()
{
    qRegisterMetaType<myNS::Points>("myNS::Points");
    TestClass tst;
    QByteArray ba = QByteArray("points");
    QVariant v = tst.property(ba);
    QVERIFY(v.isValid());
    myNS::Points p = qvariant_cast<myNS::Points>(v);
    QCOMPARE(p.p1, 0xBEEF);
    QCOMPARE(p.p2, 0xBABE);
    p.p1 = 0xCAFE;
    p.p2 = 0x1EE7;
    QVERIFY(tst.setProperty(ba, QVariant::fromValue(p)));
    myNS::Points pp = qvariant_cast<myNS::Points>(tst.property(ba));
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
    QVERIFY(QObject::connect(&test, SIGNAL(myVoidSignal2()),
                             &test, SLOT(dummySlot2())));
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
    QCOMPARE(baz.flags(), bar.flags());

    QList<Foo::Bar::Flags> l;
    l << baz.flags();
    QVariant v2 = baz.setProperty("flagsList", QVariant::fromValue(l));
    QCOMPARE(l, baz.flagsList());
    QCOMPARE(l, qvariant_cast<QList<Foo::Bar::Flags> >(baz.property("flagsList")));
}

void tst_Moc::warnOnMultipleInheritance()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    QStringList args;
    const QString header = m_sourceDirectory + QStringLiteral("/warn-on-multiple-qobject-subclasses.h");
    args << "-I" << qtIncludePath + "/QtGui" << header;
    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":43: Warning: Class Bar inherits from two QObject subclasses QWindow and Foo. This is not supported!\n"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::ignoreOptionClashes()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    QStringList args;
    const QString header = m_sourceDirectory + QStringLiteral("/interface-from-include.h");
    const QString includeDir = m_sourceDirectory + "/Test.framework/Headers";
    // given --ignore-option-clashes, -pthread should be ignored, but the -I path should not be.
    args << "--ignore-option-clashes" << "-pthread" << "-I" << includeDir << "-fno-builtin" << header;
    proc.start(m_moc, args);
    bool finished = proc.waitForFinished();
    if (!finished)
        qWarning("waitForFinished failed. QProcess error: %d", (int)proc.error());
    QVERIFY(finished);
    if (proc.exitCode() != 0) {
        qDebug() << proc.readAllStandardError();
    }
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    QByteArray mocOut = proc.readAllStandardOutput();

    // If -pthread wasn't ignored, it was parsed as a prefix of "thread/", which breaks compilation.
    QStringList gccArgs;
    gccArgs << "-c" << "-x" << "c++" << "-I" << ".."
         << "-I" << qtIncludePath << "-I" << includeDir << "-o" << "/dev/null"
         << "-fPIC" << "-std=c++11" <<  "-";
    proc.start("gcc", gccArgs);
    QVERIFY(proc.waitForStarted());
    proc.write(mocOut);
    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()), QString());
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::forgottenQInterface()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    QStringList args;
    const QString header = m_sourceDirectory + QStringLiteral("/forgotten-qinterface.h");
    args << "-I" << qtIncludePath + "/QtCore" << header;
    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":45: Warning: Class Test implements the interface MyInterface but does not list it in Q_INTERFACES. qobject_cast to MyInterface will not work!\n"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::os9Newline()
{
#if !defined(SKIP_NEWLINE_TEST)
    const QMetaObject &mo = Os9Newlines::staticMetaObject;
    QVERIFY(mo.indexOfSlot("testSlot()") != -1);
    QFile f(m_sourceDirectory + QStringLiteral("/os9-newlines.h"));
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
    QFile f(m_sourceDirectory + QStringLiteral("/win-newlines.h"));
    QVERIFY(f.open(QIODevice::ReadOnly)); // no QIODevice::Text!
    QByteArray data = f.readAll();
    f.close();
    for (int i = 0; i < data.count(); ++i) {
        if (data.at(i) == QLatin1Char('\r')) {
            QVERIFY(i < data.count() - 1);
            ++i;
            QCOMPARE(data.at(i), '\n');
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
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_UNIX) && !defined(QT_NO_PROCESS)
    QStringList args;
    args << "-F" << m_sourceDirectory + QStringLiteral("/.")
         << m_sourceDirectory + QStringLiteral("/interface-from-framework.h")
         ;

    QProcess proc;
    proc.start(m_moc, args);
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
    QSKIP("Only tested/relevant on unixy platforms");
#endif
}

void tst_Moc::cstyleEnums()
{
    const QMetaObject &obj = CStyleEnums::staticMetaObject;
    QCOMPARE(obj.enumeratorCount(), 2);
    QMetaEnum metaEnum = obj.enumerator(0);
    QCOMPARE(metaEnum.name(), "Baz");
    QCOMPARE(metaEnum.keyCount(), 2);
    QCOMPARE(metaEnum.key(0), "Foo");
    QCOMPARE(metaEnum.key(1), "Bar");

    QMetaEnum metaEnum2 = obj.enumerator(1);
    QCOMPARE(metaEnum2.name(), "Baz2");
    QCOMPARE(metaEnum2.keyCount(), 2);
    QCOMPARE(metaEnum2.key(0), "Foo2");
    QCOMPARE(metaEnum2.key(1), "Bar2");
}

void tst_Moc::templateGtGt()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start(m_moc, QStringList(m_sourceDirectory + QStringLiteral("/template-gtgt.h")));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QVERIFY(mocWarning.isEmpty());
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::defineMacroViaCmdline()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;

    QStringList args;
    args << "-DFOO";
    args << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::defineMacroViaForcedInclude()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;

    QStringList args;
    args << "--include" << m_sourceDirectory + QLatin1String("/subdir/extradefines.h");
    args << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

void tst_Moc::defineMacroViaForcedIncludeRelative()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;

    QStringList args;
    args << "--include" << QStringLiteral("extradefines.h") << "-I" + m_sourceDirectory + "/subdir";
    args << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on linux/gcc");
#endif
}


void tst_Moc::environmentIncludePaths_data()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QTest::addColumn<QString>("cmdline");
    QTest::addColumn<QString>("varname");

    QTest::newRow("INCLUDE") << "--compiler-flavor=msvc" << "INCLUDE";
    QTest::newRow("CPATH1") << QString() << "CPATH";
    QTest::newRow("CPATH2") << "--compiler-flavor=unix" << "CPATH";
    QTest::newRow("CPLUS_INCLUDE_PATH1") << QString() << "CPLUS_INCLUDE_PATH";
    QTest::newRow("CPLUS_INCLUDE_PATH2") << "--compiler-flavor=unix" << "CPLUS_INCLUDE_PATH";
#endif
}

void tst_Moc::environmentIncludePaths()
{
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QFETCH(QString, cmdline);
    QFETCH(QString, varname);

    QStringList args;
    if (!cmdline.isEmpty())
        args << cmdline;
    args << "--include" << QStringLiteral("extradefines.h")
         << m_sourceDirectory + QStringLiteral("/macro-on-cmdline.h");

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.remove("INCLUDE");
    env.remove("CPATH");
    env.remove("CPLUS_INCLUDE_PATH");
    env.insert(varname, m_sourceDirectory + "/subdir");

    QProcess proc;
    proc.setProcessEnvironment(env);
    proc.start(m_moc, args);
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QCOMPARE(proc.readAllStandardError(), QByteArray());
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

// tst_Moc::specifyMetaTagsFromCmdline()
// plugin_metadata.h contains a plugin which we register here. Since we're not building this
// application as a plugin, we need top copy some of the initializer code found in qplugin.h:
extern "C" QObject *qt_plugin_instance();
extern "C" const char *qt_plugin_query_metadata();
class StaticPluginInstance{
public:
    StaticPluginInstance() {
        QStaticPlugin plugin = { &qt_plugin_instance, &qt_plugin_query_metadata };
        qRegisterStaticPluginFunction(plugin);
    }
};
static StaticPluginInstance staticInstance;

void tst_Moc::specifyMetaTagsFromCmdline() {
    foreach (const QStaticPlugin &plugin, QPluginLoader::staticPlugins()) {
        const QString iid = plugin.metaData().value(QLatin1String("IID")).toString();
        if (iid == QLatin1String("test.meta.tags")) {
            const QJsonArray metaTagsUriList = plugin.metaData().value("uri").toArray();
            QCOMPARE(metaTagsUriList.size(), 2);

            // The following uri-s are set in the pro file using
            // -Muri=com.company.app -Muri=com.company.app.private
            QCOMPARE(metaTagsUriList[0].toString(), QLatin1String("com.company.app"));
            QCOMPARE(metaTagsUriList[1].toString(), QLatin1String("com.company.app.private"));
            return;
        }
    }
    QFAIL("Could not find plugin with IID 'test.meta.tags'");
}

void tst_Moc::invokable()
{
    {
        const QMetaObject &mobj = InvokableBeforeReturnType::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 6);
        QCOMPARE(mobj.method(5).methodSignature(), QByteArray("foo()"));
    }

    {
        const QMetaObject &mobj = InvokableBeforeInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 7);
        QCOMPARE(mobj.method(5).methodSignature(), QByteArray("foo()"));
        QCOMPARE(mobj.method(6).methodSignature(), QByteArray("bar()"));
    }
}

void tst_Moc::singleFunctionKeywordSignalAndSlot()
{
    {
        const QMetaObject &mobj = SingleFunctionKeywordBeforeReturnType::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 7);
        QCOMPARE(mobj.method(5).methodSignature(), QByteArray("mySignal()"));
        QCOMPARE(mobj.method(6).methodSignature(), QByteArray("mySlot()"));
    }

    {
        const QMetaObject &mobj = SingleFunctionKeywordBeforeInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 7);
        QCOMPARE(mobj.method(5).methodSignature(), QByteArray("mySignal()"));
        QCOMPARE(mobj.method(6).methodSignature(), QByteArray("mySlot()"));
    }

    {
        const QMetaObject &mobj = SingleFunctionKeywordAfterInline::staticMetaObject;
        QCOMPARE(mobj.methodCount(), 7);
        QCOMPARE(mobj.method(5).methodSignature(), QByteArray("mySignal()"));
        QCOMPARE(mobj.method(6).methodSignature(), QByteArray("mySlot()"));
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
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub MEMBER mBlub)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub2 MEMBER mBlub READ blub)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub3 MEMBER mBlub WRITE setBlub)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub4 MEMBER mBlub NOTIFY blub4Changed)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub5 MEMBER mBlub NOTIFY blub5Changed)
    Q_PRIVATE_PROPERTY(PrivatePropertyTest::d, QString blub6 MEMBER mConst CONSTANT)
    class MyDPointer {
    public:
        MyDPointer() : mConst("const"), mBar(0), mPlop(0) {}
        int bar() { return mBar ; }
        void setBar(int value) { mBar = value; }
        int plop() { return mPlop ; }
        void setPlop(int value) { mPlop = value; }
        int baz() { return mBaz ; }
        void setBaz(int value) { mBaz = value; }
        QString blub() const { return mBlub; }
        void setBlub(const QString &value) { mBlub = value; }
        QString mBlub;
        const QString mConst;
    private:
        int mBar;
        int mPlop;
        int mBaz;
    };
public:
    PrivatePropertyTest(QObject *parent = 0) : QObject(parent), mFoo(0), d (new MyDPointer) {}
    int foo() { return mFoo ; }
    void setFoo(int value) { mFoo = value; }
    MyDPointer *d_func() {return d.data();}
signals:
    void blub4Changed();
    void blub5Changed(const QString &newBlub);
private:
    int mFoo;
    QScopedPointer<MyDPointer> d;
};


void tst_Moc::qprivateproperties()
{
    PrivatePropertyTest test;

    test.setProperty("foo", 1);
    QCOMPARE(test.property("foo"), QVariant::fromValue(1));

    test.setProperty("bar", 2);
    QCOMPARE(test.property("bar"), QVariant::fromValue(2));

    test.setProperty("plop", 3);
    QCOMPARE(test.property("plop"), QVariant::fromValue(3));

    test.setProperty("baz", 4);
    QCOMPARE(test.property("baz"), QVariant::fromValue(4));

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
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/warn-on-property-without-read.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header +
                QString(":36: Warning: Property declaration foo has no READ accessor function or associated MEMBER variable. The property will be invalid.\n"));
#else
    QSKIP("Only tested on linux/gcc");
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
        QCOMPARE(mm.methodSignature(), QByteArray("CtorTestClass(QObject*)"));
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
        QCOMPARE(mm.methodSignature(), QByteArray("CtorTestClass()"));
        QCOMPARE(mm.typeName(), "");
        QCOMPARE(mm.parameterNames().size(), 0);
        QCOMPARE(mm.parameterTypes().size(), 0);
    }
    {
        QMetaMethod mm = mo->constructor(2);
        QCOMPARE(mm.access(), QMetaMethod::Public);
        QCOMPARE(mm.methodType(), QMetaMethod::Constructor);
        QCOMPARE(mm.methodSignature(), QByteArray("CtorTestClass(QString)"));
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

    QScopedPointer<QObject> o1(mo->newInstance());
    QVERIFY(o1 != 0);
    QCOMPARE(o1->parent(), (QObject*)0);
    QVERIFY(qobject_cast<CtorTestClass*>(o1.data()) != 0);

    QObject *o2 = mo->newInstance(Q_ARG(QObject*, o1.data()));
    QVERIFY(o2 != 0);
    QCOMPARE(o2->parent(), o1.data());

    QString str = QString::fromLatin1("hello");
    QScopedPointer<QObject> o3(mo->newInstance(Q_ARG(QString, str)));
    QVERIFY(o3 != 0);
    QCOMPARE(qobject_cast<CtorTestClass*>(o3.data())->m_str, str);

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
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/pure-virtual-signals.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QString mocWarning = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocWarning, header + QString(":38: Warning: Signals cannot be declared virtual\n") +
                         header + QString(":40: Warning: Signals cannot be declared virtual\n"));
#else
    QSKIP("Only tested on linux/gcc");
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
    void doSomethingElse(QSharedPointer<QVarLengthArray<QString, (16 >> 2)> > val
            = QSharedPointer<QVarLengthArray<QString, (16 >> 2)> >() )
    { Q_UNUSED(val); }
#else
    // we want to test the previous function, but gcc < 4.4 seemed to have a bug similar to the one moc has.
    typedef QHash<QString, QVariant> WorkaroundGCCBug;
    void doSomething(QHash<QString, QVariant> values = WorkaroundGCCBug() ) { Q_UNUSED(values); }
    void doSomethingElse(QSharedPointer<QVarLengthArray<QString, (16 >> 2)> > val
            = (QSharedPointer<QVarLengthArray<QString, (16 >> 2)> >()) )
    { Q_UNUSED(val); }
#endif

    void doAnotherThing(bool a = (1 < 3), bool b = (1 > 4)) { Q_UNUSED(a); Q_UNUSED(b); }

#if defined(Q_MOC_RUN) || (defined(Q_COMPILER_AUTO_TYPE) && !(defined(Q_CC_CLANG) && Q_CC_CLANG < 304))
    // There is no Q_COMPILER_>>  but if compiler support auto, it should also support >>
    void performSomething(QVector<QList<QString>> e = QVector<QList<QString>>(8 < 1),
                          QHash<int, QVector<QString>> h = QHash<int, QVector<QString>>())
    { Q_UNUSED(e); Q_UNUSED(h); }
#else
    void performSomething(QVector<QList<QString> > e = QVector<QList<QString> >(),
                          QHash<int, QVector<QString> > h = (QHash<int, QVector<QString> >()))
    { Q_UNUSED(e); Q_UNUSED(h); }
#endif
};


void tst_Moc::QTBUG12260_defaultTemplate()
{
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doSomething(QHash<QString,QVariant>)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doAnotherThing(bool,bool)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("doSomethingElse(QSharedPointer<QVarLengthArray<QString,(16>>2)> >)") != -1);
    QVERIFY(QTBUG12260_defaultTemplate_Object::staticMetaObject.indexOfSlot("performSomething(QVector<QList<QString> >,QHash<int,QVector<QString> >)") != -1);
}

void tst_Moc::notifyError()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    const QString header = m_sourceDirectory + QStringLiteral("/error-on-wrong-notify.h");
    proc.start(m_moc, QStringList(header));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 1);
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(mocOut.isEmpty());
    QString mocError = QString::fromLocal8Bit(proc.readAllStandardError());
    QCOMPARE(mocError, header +
        QString(":42: Error: NOTIFY signal 'fooChanged' of property 'foo' does not exist in class ClassWithWrongNOTIFY.\n"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

class QTBUG_17635_InvokableAndProperty : public QObject
{
    Q_OBJECT
public:
    Q_PROPERTY(int numberOfEggs READ numberOfEggs)
    Q_PROPERTY(int numberOfChickens READ numberOfChickens)
    Q_INVOKABLE QString getEgg(int index) { Q_UNUSED(index); return QString::fromLatin1("Egg"); }
    Q_INVOKABLE QString getChicken(int index) { Q_UNUSED(index); return QString::fromLatin1("Chicken"); }
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

public:
    int foo() const { return 0; }

    Q_INVOKABLE void method1() {}
    Q_INVOKABLE Q_REVISION(4) void method2() {}

    enum TestEnum { One, Two };
    Q_ENUM(TestEnum);


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

public:
    int foo() const { return 0; }

    Q_INVOKABLE void method1() {}
    Q_INVOKABLE Q_REVISION(4) void method2() {}

    enum TestEnum { One, Two };
    Q_ENUM(TestEnum);

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
    QCOMPARE(T::staticMetaObject.property(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfProperty("prop2");
    QCOMPARE(T::staticMetaObject.property(idx).revision(), 2);

    idx = T::staticMetaObject.indexOfMethod("method1()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfMethod("method2()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 4);

    idx = T::staticMetaObject.indexOfSlot("slot1()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfSlot("slot2()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 3);

    idx = T::staticMetaObject.indexOfSlot("slot3()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 6);
    idx = T::staticMetaObject.indexOfSlot("slot4()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 6);

    idx = T::staticMetaObject.indexOfSignal("signal1()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 0);
    idx = T::staticMetaObject.indexOfSignal("signal2()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 5);

    idx = T::staticMetaObject.indexOfSignal("signal3()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 7);
    idx = T::staticMetaObject.indexOfSignal("signal4()");
    QCOMPARE(T::staticMetaObject.method(idx).revision(), 7);

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
        << QString("standard input:1: Warning: Property declaration x has no READ accessor function or associated MEMBER variable. The property will be invalid.");

    // This should output a warning
    QTest::newRow("Duplicate property warning")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x READ x) Q_PROPERTY(int x READ y) };")
        << QStringList()
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1: Warning: The property 'x' is defined multiple times in class X.");

    // Passing "-nn" should NOT suppress the warning
    QTest::newRow("Invalid property warning with -nn")
        << QByteArray("class X : public QObject { Q_OBJECT Q_PROPERTY(int x) };")
        << (QStringList() << "-nn")
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString("standard input:1: Warning: Property declaration x has no READ accessor function or associated MEMBER variable. The property will be invalid.");

    // Passing "-nw" should suppress the warning
    QTest::newRow("Invalid property warning with -nw")
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
    QTest::newRow("Does not inherit QObject with -nw")
        << QByteArray("class X { Q_OBJECT };")
        << (QStringList() << "-nw")
        << 1
        << QString()
        << QString("standard input:1: Error: Class contains Q_OBJECT macro but does not inherit from QObject");

    QTest::newRow("Warning on invalid macro")
        << QByteArray("#define Foo(a, b)\n class X : public QObject { Q_OBJECT  }; \n Foo(a) \n Foo(a,b,c) \n")
        << QStringList()
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QString();

    QTest::newRow("Class declaration lacks Q_OBJECT macro.")
        << QByteArray("class X : public QObject \n { \n public slots: \n void foo() {} \n };")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5: Error: Class declaration lacks Q_OBJECT macro.");

    QTest::newRow("Namespace declaration lacks Q_NAMESPACE macro.")
        << QByteArray("namespace X {\nQ_CLASSINFO(\"key\",\"value\")\nenum class MyEnum {Key1 = 1}\nQ_ENUMS(MyEnum)\n}\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:1: Error: Namespace declaration lacks Q_NAMESPACE macro.");

    QTest::newRow("Wrong Q_ENUM context.")
        << QByteArray("namespace X {\nQ_NAMESPACE\n\nenum class MyEnum {Key1 = 1}\nQ_ENUM(MyEnum)\n}\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5: Error: Q_ENUM can't be used in a Q_NAMESPACE, use Q_ENUM_NS instead");

    QTest::newRow("Wrong Q_FLAG context.")
        << QByteArray("namespace X {\nQ_NAMESPACE\n\nenum class MyEnum {Key1 = 1}\nQ_FLAG(MyEnum)\n}\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5: Error: Q_FLAG can't be used in a Q_NAMESPACE, use Q_FLAG_NS instead");

    QTest::newRow("Wrong Q_ENUM_NS context.")
        << QByteArray("class X {\nQ_GADGET\n\nenum class MyEnum {Key1 = 1}\nQ_ENUM_NS(MyEnum)\n};\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5: Error: Q_ENUM_NS can't be used in a Q_OBJECT/Q_GADGET, use Q_ENUM instead");

    QTest::newRow("Wrong Q_FLAG_NS context.")
        << QByteArray("class X {\nQ_GADGET\n\nenum class MyEnum {Key1 = 1}\nQ_FLAG_NS(MyEnum)\n};\n")
        << QStringList()
        << 1
        << QString()
        << QString("standard input:5: Error: Q_FLAG_NS can't be used in a Q_OBJECT/Q_GADGET, use Q_FLAG instead");

    QTest::newRow("Invalid macro definition")
        << QByteArray("#define Foo(a, b, c) a b c #a #b #c a##b##c #d\n Foo(45, 42, 39);")
        << QStringList()
        << 1
        << QString("IGNORE_ALL_STDOUT")
        << QString(":2: Error: '#' is not followed by a macro parameter");

    QTest::newRow("QTBUG-46210: crash on invalid macro invocation")
        << QByteArray("#define Foo(a, b, c) a b c #a #b #c a##b##c\n Foo(45);")
        << QStringList()
        << 1
        << QString("IGNORE_ALL_STDOUT")
        << QString(":2: Error: Macro invoked with too few parameters for a use of '#'");

    QTest::newRow("QTBUG-54609: crash on invalid input")
        << QByteArray::fromBase64("EAkJCQkJbGFzcyBjbGFzcyBiYWkcV2kgTUEKcGYjZGVmaW5lIE1BKFEs/4D/FoQ=")
        << QStringList()
        << 1
        << QString("IGNORE_ALL_STDOUT")
        << QString(":-1: Error: Unexpected character in macro argument list.");

    QTest::newRow("Missing header warning")
        << QByteArray("class X : public QObject { Q_OBJECT };")
        << (QStringList() << QStringLiteral("--include") << QStringLiteral("doesnotexist.h"))
        << 0
        << QString("IGNORE_ALL_STDOUT")
        << QStringLiteral("Warning: Failed to resolve include \"doesnotexist.h\" for moc file <standard input>");

    QTest::newRow("QTBUG-54815: Crash on invalid input")
        << QByteArray("class M{(})F<{}d000000000000000#0")
        << QStringList()
        << 0
        << QString()
        << QString("standard input:1: Note: No relevant classes found. No output generated.");
}

void tst_Moc::warnings()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
    QFETCH(QByteArray, input);
    QFETCH(QStringList, args);
    QFETCH(int, exitCode);
    QFETCH(QString, expectedStdOut);
    QFETCH(QString, expectedStdErr);

#ifdef Q_CC_MSVC
    // for some reasons, moc compiled with MSVC uses a different output format
    QRegExp lineNumberRe(":(-?\\d+):");
    lineNumberRe.setMinimal(true);
    expectedStdErr.replace(lineNumberRe, "(\\1):");
#endif

    QProcess proc;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_MESSAGE_PATTERN", "no qDebug or qWarning please");
    proc.setProcessEnvironment(env);

    proc.start(m_moc, args);
    QVERIFY(proc.waitForStarted());

    QCOMPARE(proc.write(input), qint64(input.size()));

    proc.closeWriteChannel();

    QVERIFY(proc.waitForFinished());

    QCOMPARE(proc.exitCode(), exitCode);
    QCOMPARE(proc.exitStatus(), QProcess::NormalExit);

    // magic value "IGNORE_ALL_STDOUT" ignores stdout
    if (expectedStdOut != "IGNORE_ALL_STDOUT")
        QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed(), expectedStdOut);
    QCOMPARE(QString::fromLocal8Bit(proc.readAllStandardError()).trimmed().remove('\r'), expectedStdErr);
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
    QCOMPARE(PrivateClass::staticMetaObject.indexOfConstructor("PrivateClass()"), 0);
    QVERIFY(PrivateClass::staticMetaObject.indexOfSignal("someSignal()") > 0);
}

void tst_Moc::cxx11Enums_data()
{
    QTest::addColumn<const QMetaObject *>("meta");
    QTest::addColumn<QByteArray>("enumName");
    QTest::addColumn<char>("prefix");
    QTest::addColumn<bool>("isScoped");

    const QMetaObject *meta1 = &CXX11Enums::staticMetaObject;
    const QMetaObject *meta2 = &CXX11Enums2::staticMetaObject;

    QTest::newRow("EnumClass") << meta1 << QByteArray("EnumClass") << 'A' << true;
    QTest::newRow("EnumClass 2") << meta2 << QByteArray("EnumClass") << 'A' << true;
    QTest::newRow("TypedEnum") << meta1 << QByteArray("TypedEnum") << 'B' << false;
    QTest::newRow("TypedEnum 2") << meta2 << QByteArray("TypedEnum") << 'B' << false;
    QTest::newRow("TypedEnumClass") << meta1 << QByteArray("TypedEnumClass") << 'C' << true;
    QTest::newRow("TypedEnumClass 2") << meta2 << QByteArray("TypedEnumClass") << 'C' << true;
    QTest::newRow("NormalEnum") << meta1 << QByteArray("NormalEnum") << 'D' << false;
    QTest::newRow("NormalEnum 2") << meta2 << QByteArray("NormalEnum") << 'D' << false;
}

void tst_Moc::cxx11Enums()
{
    QFETCH(const QMetaObject *,meta);
    QCOMPARE(meta->enumeratorOffset(), 0);

    QFETCH(QByteArray, enumName);
    QFETCH(char, prefix);
    QFETCH(bool, isScoped);

    int idx;
    idx = meta->indexOfEnumerator(enumName);
    QVERIFY(idx != -1);
    QCOMPARE(meta->enumerator(idx).enclosingMetaObject(), meta);
    QCOMPARE(meta->enumerator(idx).isValid(), true);
    QCOMPARE(meta->enumerator(idx).keyCount(), 4);
    QCOMPARE(meta->enumerator(idx).name(), enumName.constData());
    for (int i = 0; i < 4; i++) {
        QByteArray v = prefix + QByteArray::number(i);
        QCOMPARE(meta->enumerator(idx).keyToValue(v), i);
        QCOMPARE(meta->enumerator(idx).valueToKey(i), v.constData());
    }
    QCOMPARE(meta->enumerator(idx).isScoped(), isScoped);
}

void tst_Moc::returnRefs()
{
    TestClass tst;
    const QMetaObject *mobj = tst.metaObject();
    QVERIFY(mobj->indexOfMethod("myInvokableReturningRef()") != -1);
    QVERIFY(mobj->indexOfMethod("myInvokableReturningConstRef()") != -1);
    // Those two functions are copied from the qscriptextqobject test in qtscript
    // they used to cause miscompilation of the moc generated file.
}

void tst_Moc::memberProperties_data()
{
    QTest::addColumn<int>("object");
    QTest::addColumn<QString>("property");
    QTest::addColumn<QString>("signal");
    QTest::addColumn<QString>("writeValue");
    QTest::addColumn<bool>("expectedWriteResult");
    QTest::addColumn<QString>("expectedReadResult");

    pPPTest = new PrivatePropertyTest( this );

    QTest::newRow("MEMBER property")
            << 0 << "member1" << "" << "abc" << true << "abc";
    QTest::newRow("MEMBER property with READ function")
            << 0 << "member2" << "" << "def" << true << "def";
    QTest::newRow("MEMBER property with WRITE function")
            << 0 << "member3" << "" << "ghi" << true << "ghi";
    QTest::newRow("MEMBER property with NOTIFY")
            << 0 << "member4" << "member4Changed()" << "lmn" << true << "lmn";
    QTest::newRow("MEMBER property with NOTIFY(value)")
            << 0 << "member5" << "member5Changed(const QString&)" << "opq" << true << "opq";
    QTest::newRow("MEMBER property with CONSTANT")
            << 0 << "member6" << "" << "test" << false << "const";
    QTest::newRow("private MEMBER property")
            << 1 << "blub" << "" << "abc" << true << "abc";
    QTest::newRow("private MEMBER property with READ function")
            << 1 << "blub2" << "" << "def" << true << "def";
    QTest::newRow("private MEMBER property with WRITE function")
            << 1 << "blub3" << "" << "ghi" << true << "ghi";
    QTest::newRow("private MEMBER property with NOTIFY")
            << 1 << "blub4" << "blub4Changed()" << "jkl" << true << "jkl";
    QTest::newRow("private MEMBER property with NOTIFY(value)")
            << 1 << "blub5" << "blub5Changed(const QString&)" << "mno" << true << "mno";
    QTest::newRow("private MEMBER property with CONSTANT")
            << 1 << "blub6" << "" << "test" << false << "const";
    QTest::newRow("sub1")
            << 0 << "sub1" << "" << "helloSub1" << true << "helloSub1";
    QTest::newRow("sub2")
            << 0 << "sub2" << "" << "helloSub2" << true << "helloSub2";
}

void tst_Moc::memberProperties()
{
    QFETCH(int, object);
    QFETCH(QString, property);
    QFETCH(QString, signal);
    QFETCH(QString, writeValue);
    QFETCH(bool, expectedWriteResult);
    QFETCH(QString, expectedReadResult);

    QObject *pObj = (object == 0) ? this : static_cast<QObject*>(pPPTest);

    QString sSignalDeclaration;
    if (!signal.isEmpty())
        sSignalDeclaration = QString(SIGNAL(%1)).arg(signal);
    else
        QTest::ignoreMessage(QtWarningMsg, "QSignalSpy: Not a valid signal, use the SIGNAL macro");
    QSignalSpy notifySpy(pObj, sSignalDeclaration.toLatin1().constData());

    int index = pObj->metaObject()->indexOfProperty(property.toLatin1().constData());
    QVERIFY(index != -1);
    QMetaProperty prop = pObj->metaObject()->property(index);

    QCOMPARE(prop.write(pObj, writeValue), expectedWriteResult);

    QVariant readValue = prop.read(pObj);
    QCOMPARE(readValue.toString(), expectedReadResult);

    if (!signal.isEmpty())
    {
        QCOMPARE(notifySpy.count(), 1);
        if (prop.notifySignal().parameterNames().size() > 0) {
            QList<QVariant> arguments = notifySpy.takeFirst();
            QCOMPARE(arguments.size(), 1);
            QCOMPARE(arguments.at(0).toString(), expectedReadResult);
        }

        notifySpy.clear();
        // a second write with the same value should not cause the signal to be emitted again
        QCOMPARE(prop.write(pObj, writeValue), expectedWriteResult);
        QCOMPARE(notifySpy.count(), 0);
    }
}

//this used to fail to compile
class ClassWithOneMember  : public QObject {
    Q_PROPERTY(int member MEMBER member)
    Q_OBJECT
public:
    int member;
};

void tst_Moc::memberProperties2()
{
    ClassWithOneMember o;
    o.member = 442;
    QCOMPARE(o.property("member").toInt(), 442);
    QVERIFY(o.setProperty("member", 6666));
    QCOMPARE(o.member, 6666);
}

class SignalConnectionTester : public QObject
{
    Q_OBJECT
public:
    SignalConnectionTester(QObject *parent = 0)
      : QObject(parent), testPassed(false)
    {

    }

public Q_SLOTS:
    void testSlot()
    {
      testPassed = true;
    }
    void testSlotWith1Arg(int i)
    {
      testPassed = i == 42;
    }
    void testSlotWith2Args(int i, const QString &s)
    {
      testPassed = i == 42 && s == "Hello";
    }

public:
    bool testPassed;
};

class ClassWithPrivateSignals : public QObject
{
    Q_OBJECT
public:
    ClassWithPrivateSignals(QObject *parent = 0)
      : QObject(parent)
    {

    }

    void emitPrivateSignals()
    {
        emit privateSignal1(QPrivateSignal());
        emit privateSignalWith1Arg(42, QPrivateSignal());
        emit privateSignalWith2Args(42, "Hello", QPrivateSignal());

        emit privateOverloadedSignal(QPrivateSignal());
        emit privateOverloadedSignal(42, QPrivateSignal());

        emit overloadedMaybePrivate();
        emit overloadedMaybePrivate(42, QPrivateSignal());
    }

Q_SIGNALS:
    void privateSignal1(QPrivateSignal);
    void privateSignalWith1Arg(int arg1, QPrivateSignal);
    void privateSignalWith2Args(int arg1, const QString &arg2, QPrivateSignal);

    void privateOverloadedSignal(QPrivateSignal);
    void privateOverloadedSignal(int, QPrivateSignal);

    void overloadedMaybePrivate();
    void overloadedMaybePrivate(int, QPrivateSignal);

};

class SubClassFromPrivateSignals : public ClassWithPrivateSignals
{
    Q_OBJECT
public:
    SubClassFromPrivateSignals(QObject *parent = 0)
      : ClassWithPrivateSignals(parent)
    {

    }

    void emitProtectedSignals()
    {
      // Compile test: All of this intentionally does not compile:
//         emit privateSignal1();
//         emit privateSignalWith1Arg(42);
//         emit privateSignalWith2Args(42, "Hello");
//
//         emit privateSignal1(QPrivateSignal());
//         emit privateSignalWith1Arg(42, QPrivateSignal());
//         emit privateSignalWith2Args(42, "Hello", QPrivateSignal());
//
//         emit privateSignal1(ClassWithPrivateSignals::QPrivateSignal());
//         emit privateSignalWith1Arg(42, ClassWithPrivateSignals::QPrivateSignal());
//         emit privateSignalWith2Args(42, "Hello", ClassWithPrivateSignals::QPrivateSignal());

//         emit privateOverloadedSignal();
//         emit privateOverloadedSignal(42);

//         emit overloadedMaybePrivate();
//         emit overloadedMaybePrivate(42);


    }
};

void tst_Moc::privateSignalConnection()
{
    // Function pointer connects. Matching signals and slots
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignal1, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&classWithPrivateSignals, "privateSignal1");
        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignal1, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&subClassFromPrivateSignals, "privateSignal1");
        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&classWithPrivateSignals, "privateSignalWith1Arg", Q_ARG(int, 42));
        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&subClassFromPrivateSignals, "privateSignalWith1Arg", Q_ARG(int, 42));
        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith2Args, &tester, &SignalConnectionTester::testSlotWith2Args);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&classWithPrivateSignals, "privateSignalWith2Args", Q_ARG(int, 42), Q_ARG(QString, "Hello"));
        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith2Args, &tester, &SignalConnectionTester::testSlotWith2Args);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
        tester.testPassed = false;
        QMetaObject::invokeMethod(&subClassFromPrivateSignals, "privateSignalWith2Args", Q_ARG(int, 42), Q_ARG(QString, "Hello"));
        QVERIFY(tester.testPassed);
    }


    // String based connects. Matching signals and slots
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignal1()), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignal1()), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlotWith1Arg(int)));

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlotWith1Arg(int)));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith2Args(int,QString)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith2Args(int,QString)));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }

    // Function pointer connects. Decayed slot arguments
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlotWith1Arg);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, &ClassWithPrivateSignals::privateSignalWith1Arg, &tester, &SignalConnectionTester::testSlot);

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }

    // String based connects. Decayed slot arguments
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith1Arg(int)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith1Arg(int)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlotWith1Arg(int)));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlot()));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {
        SubClassFromPrivateSignals subClassFromPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&subClassFromPrivateSignals, SIGNAL(privateSignalWith2Args(int,QString)), &tester, SLOT(testSlot()));

        QVERIFY(!tester.testPassed);

        subClassFromPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }

    // Overloaded private signals
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateOverloadedSignal()), &tester, SLOT(testSlot()));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateOverloadedSignal(int)), &tester, SLOT(testSlotWith1Arg(int)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    // We can't use function pointer connections to private signals which are overloaded because we would have to cast in this case to:
    //   static_cast<void (ClassWithPrivateSignals::*)(int, ClassWithPrivateSignals::QPrivateSignal)>(&ClassWithPrivateSignals::privateOverloadedSignal)
    // Which doesn't work as ClassWithPrivateSignals::QPrivateSignal is private.

    // Overload with either private or not private signals
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(overloadedMaybePrivate()), &tester, SLOT(testSlot()));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals, SIGNAL(privateOverloadedSignal(int)), &tester, SLOT(testSlotWith1Arg(int)));
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    {

        ClassWithPrivateSignals classWithPrivateSignals;
        SignalConnectionTester tester;
        QObject::connect(&classWithPrivateSignals,
                         static_cast<void (ClassWithPrivateSignals::*)()>(&ClassWithPrivateSignals::overloadedMaybePrivate),
                         &tester, &SignalConnectionTester::testSlot);
        QVERIFY(!tester.testPassed);

        classWithPrivateSignals.emitPrivateSignals();

        QVERIFY(tester.testPassed);
    }
    // We can't use function pointer connections to private signals which are overloaded because we would have to cast in this case to:
    //   static_cast<void (ClassWithPrivateSignals::*)(int, ClassWithPrivateSignals::QPrivateSignal)>(&ClassWithPrivateSignals::overloadedMaybePrivate)
    // Which doesn't work as ClassWithPrivateSignals::QPrivateSignal is private.
}

void tst_Moc::finalClasses_data()
{
    QTest::addColumn<QString>("className");
    QTest::addColumn<QString>("expected");

    QTest::newRow("FinalTestClassQt") << FinalTestClassQt::staticMetaObject.className() << "FinalTestClassQt";
    QTest::newRow("ExportedFinalTestClassQt") << ExportedFinalTestClassQt::staticMetaObject.className() << "ExportedFinalTestClassQt";
    QTest::newRow("ExportedFinalTestClassQtX") << ExportedFinalTestClassQtX::staticMetaObject.className() << "ExportedFinalTestClassQtX";

    QTest::newRow("FinalTestClassCpp11") << FinalTestClassCpp11::staticMetaObject.className() << "FinalTestClassCpp11";
    QTest::newRow("ExportedFinalTestClassCpp11") << ExportedFinalTestClassCpp11::staticMetaObject.className() << "ExportedFinalTestClassCpp11";
    QTest::newRow("ExportedFinalTestClassCpp11X") << ExportedFinalTestClassCpp11X::staticMetaObject.className() << "ExportedFinalTestClassCpp11X";

    QTest::newRow("SealedTestClass") << SealedTestClass::staticMetaObject.className() << "SealedTestClass";
    QTest::newRow("ExportedSealedTestClass") << ExportedSealedTestClass::staticMetaObject.className() << "ExportedSealedTestClass";
    QTest::newRow("ExportedSealedTestClassX") << ExportedSealedTestClassX::staticMetaObject.className() << "ExportedSealedTestClassX";
}

void tst_Moc::finalClasses()
{
    QFETCH(QString, className);
    QFETCH(QString, expected);

    QCOMPARE(className, expected);
}

void tst_Moc::explicitOverrideControl_data()
{
    QTest::addColumn<const QMetaObject*>("mo");

#define ADD(x) QTest::newRow(#x) << &x::staticMetaObject
    ADD(ExplicitOverrideControlFinalQt);
    ADD(ExplicitOverrideControlFinalCxx11);
    ADD(ExplicitOverrideControlSealed);
    ADD(ExplicitOverrideControlOverrideQt);
    ADD(ExplicitOverrideControlOverrideCxx11);
    ADD(ExplicitOverrideControlFinalQtOverrideQt);
    ADD(ExplicitOverrideControlFinalCxx11OverrideCxx11);
    ADD(ExplicitOverrideControlSealedOverride);
#undef ADD
}

void tst_Moc::explicitOverrideControl()
{
    QFETCH(const QMetaObject*, mo);

    QVERIFY(mo);
    QCOMPARE(mo->indexOfMethod("pureSlot0()"), mo->methodOffset() + 0);
    QCOMPARE(mo->indexOfMethod("pureSlot1()"), mo->methodOffset() + 1);
    QCOMPARE(mo->indexOfMethod("pureSlot2()"), mo->methodOffset() + 2);
    QCOMPARE(mo->indexOfMethod("pureSlot3()"), mo->methodOffset() + 3);
#if 0 // moc doesn't support volatile slots
    QCOMPARE(mo->indexOfMethod("pureSlot4()"), mo->methodOffset() + 4);
    QCOMPARE(mo->indexOfMethod("pureSlot5()"), mo->methodOffset() + 5);
    QCOMPARE(mo->indexOfMethod("pureSlot6()"), mo->methodOffset() + 6);
    QCOMPARE(mo->indexOfMethod("pureSlot7()"), mo->methodOffset() + 7);
    QCOMPARE(mo->indexOfMethod("pureSlot8()"), mo->methodOffset() + 8);
    QCOMPARE(mo->indexOfMethod("pureSlot9()"), mo->methodOffset() + 9);
#endif
}

class CustomQObject : public QObject
{
    Q_OBJECT
    Q_ENUMS(Number)
public:
    enum Number {
      Zero,
      One,
      Two
    };
    explicit CustomQObject(QObject *parent = 0)
      : QObject(parent)
    {
    }
};

Q_DECLARE_METATYPE(CustomQObject::Number)

typedef CustomQObject* CustomQObjectStar;
Q_DECLARE_METATYPE(CustomQObjectStar);

namespace SomeNamespace {

class NamespacedQObject : public QObject
{
    Q_OBJECT
public:
    explicit NamespacedQObject(QObject *parent = 0)
      : QObject(parent)
    {

    }
};

struct NamespacedNonQObject {};
}
Q_DECLARE_METATYPE(SomeNamespace::NamespacedNonQObject)

// Need different types for the invokable method tests because otherwise the registration
// done in the property test would interfere.

class CustomQObject2 : public QObject
{
    Q_OBJECT
    Q_ENUMS(Number)
public:
    enum Number {
      Zero,
      One,
      Two
    };
    explicit CustomQObject2(QObject *parent = 0)
      : QObject(parent)
    {
    }
};

Q_DECLARE_METATYPE(CustomQObject2::Number)

typedef CustomQObject2* CustomQObject2Star;
Q_DECLARE_METATYPE(CustomQObject2Star);

namespace SomeNamespace2 {

class NamespacedQObject2 : public QObject
{
    Q_OBJECT
public:
    explicit NamespacedQObject2(QObject *parent = 0)
      : QObject(parent)
    {

    }
};

struct NamespacedNonQObject2 {};
}
Q_DECLARE_METATYPE(SomeNamespace2::NamespacedNonQObject2)


struct CustomObject3 {};
struct CustomObject4 {};
struct CustomObject5 {};
struct CustomObject6 {};
struct CustomObject7 {};
struct CustomObject8 {};
struct CustomObject9 {};
struct CustomObject10 {};
struct CustomObject11 {};
struct CustomObject12 {};

Q_DECLARE_METATYPE(CustomObject3)
Q_DECLARE_METATYPE(CustomObject4)
Q_DECLARE_METATYPE(CustomObject5)
Q_DECLARE_METATYPE(CustomObject6)
Q_DECLARE_METATYPE(CustomObject7)
Q_DECLARE_METATYPE(CustomObject8)
Q_DECLARE_METATYPE(CustomObject9)
Q_DECLARE_METATYPE(CustomObject10)
Q_DECLARE_METATYPE(CustomObject11)
Q_DECLARE_METATYPE(CustomObject12)

class AutoRegistrationObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* object READ object CONSTANT)
    Q_PROPERTY(CustomQObject* customObject READ customObject CONSTANT)
    Q_PROPERTY(QSharedPointer<CustomQObject> customObjectP READ customObjectP CONSTANT)
    Q_PROPERTY(QWeakPointer<CustomQObject> customObjectWP READ customObjectWP CONSTANT)
    Q_PROPERTY(QPointer<CustomQObject> customObjectTP READ customObjectTP CONSTANT)
    Q_PROPERTY(QList<int> listInt READ listInt CONSTANT)
    Q_PROPERTY(QVector<QVariant> vectorVariant READ vectorVariant CONSTANT)
    Q_PROPERTY(QList<CustomQObject*> listObject READ listObject CONSTANT)
    Q_PROPERTY(QVector<QList<int>> vectorListInt READ vectorListInt CONSTANT)
    Q_PROPERTY(QVector<QList<CustomQObject*>> vectorListObject READ vectorListObject CONSTANT)
    Q_PROPERTY(CustomQObject::Number enumValue READ enumValue CONSTANT)
    Q_PROPERTY(CustomQObjectStar customObjectTypedef READ customObjectTypedef CONSTANT)
    Q_PROPERTY(SomeNamespace::NamespacedQObject* customObjectNamespaced READ customObjectNamespaced CONSTANT)
    Q_PROPERTY(SomeNamespace::NamespacedNonQObject customNonQObjectNamespaced READ customNonQObjectNamespaced CONSTANT)
public:
    AutoRegistrationObject(QObject *parent = 0)
      : QObject(parent)
    {
    }

    QObject* object() const
    {
        return 0;
    }

    QSharedPointer<CustomQObject> customObjectP() const
    {
        return QSharedPointer<CustomQObject>();
    }

    QWeakPointer<CustomQObject> customObjectWP() const
    {
        return QWeakPointer<CustomQObject>();
    }

    QPointer<CustomQObject> customObjectTP() const
    {
        return QPointer<CustomQObject>();
    }

    CustomQObject* customObject() const
    {
        return 0;
    }

    QList<int> listInt() const
    {
        return QList<int>();
    }

    QVector<QVariant> vectorVariant() const
    {
        return QVector<QVariant>();
    }

    QList<CustomQObject*> listObject() const
    {
        return QList<CustomQObject*>();
    }

    QVector<QList<int> > vectorListInt() const
    {
        return QVector<QList<int> >();
    }

    QVector<QList<CustomQObject*> > vectorListObject() const
    {
        return QVector<QList<CustomQObject*> >();
    }

    CustomQObject::Number enumValue() const
    {
        return CustomQObject::Zero;
    }

    CustomQObjectStar customObjectTypedef() const
    {
        return 0;
    }

    SomeNamespace::NamespacedQObject* customObjectNamespaced() const
    {
        return 0;
    }

    SomeNamespace::NamespacedNonQObject customNonQObjectNamespaced() const
    {
        return SomeNamespace::NamespacedNonQObject();
    }

public slots:
    void objectSlot(QObject*) {}
    void customObjectSlot(CustomQObject2*) {}
    void sharedPointerSlot(QSharedPointer<CustomQObject2>) {}
    void weakPointerSlot(QWeakPointer<CustomQObject2>) {}
    void trackingPointerSlot(QPointer<CustomQObject2>) {}
    void listIntSlot(QList<int>) {}
    void vectorVariantSlot(QVector<QVariant>) {}
    void listCustomObjectSlot(QList<CustomQObject2*>) {}
    void vectorListIntSlot(QVector<QList<int> >) {}
    void vectorListCustomObjectSlot(QVector<QList<CustomQObject2*> >) {}
    void enumSlot(CustomQObject2::Number) {}
    void typedefSlot(CustomQObject2Star) {}
    void namespacedQObjectSlot(SomeNamespace2::NamespacedQObject2*) {}
    void namespacedNonQObjectSlot(SomeNamespace2::NamespacedNonQObject2) {}

    void bu1(int, CustomObject3) {}
    void bu2(CustomObject4, int) {}
    void bu3(CustomObject5, CustomObject6) {}
    void bu4(CustomObject7, int, CustomObject8) {}
    void bu5(int, CustomObject9, CustomObject10) {}
    void bu6(int, CustomObject11, int) {}

    // these can't be registered, but they should at least compile
    void ref1(int&) {}
    void ref2(QList<int>&) {}
    void ref3(CustomQObject2&) {}
    void ref4(QSharedPointer<CustomQObject2>&) {}

signals:
    void someSignal(CustomObject12);
};

void tst_Moc::autoPropertyMetaTypeRegistration()
{
    AutoRegistrationObject aro;

    static const int numPropertiesUnderTest = 15;
    QVector<int> propertyMetaTypeIds;
    propertyMetaTypeIds.reserve(numPropertiesUnderTest);

    const QMetaObject *metaObject = aro.metaObject();
    QCOMPARE(metaObject->propertyCount(), numPropertiesUnderTest);
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        const QMetaProperty prop = metaObject->property(i);
        propertyMetaTypeIds.append(prop.userType());
        QVariant var = prop.read(&aro);
        QVERIFY(var.isValid());
    }

    // Verify that QMetaProperty::userType gave us what we expected.
    QVector<int> expectedMetaTypeIds = QVector<int>()
        << QMetaType::QString            // QObject::userType
        << QMetaType::QObjectStar        // AutoRegistrationObject::object
        << qMetaTypeId<CustomQObject*>() // etc.
        << qMetaTypeId<QSharedPointer<CustomQObject> >()
        << qMetaTypeId<QWeakPointer<CustomQObject> >()
        << qMetaTypeId<QPointer<CustomQObject> >()
        << qMetaTypeId<QList<int> >()
        << qMetaTypeId<QVector<QVariant> >()
        << qMetaTypeId<QList<CustomQObject*> >()
        << qMetaTypeId<QVector<QList<int> > >()
        << qMetaTypeId<QVector<QList<CustomQObject*> > >()
        << qMetaTypeId<CustomQObject::Number>()
        << qMetaTypeId<CustomQObjectStar>()
        << qMetaTypeId<SomeNamespace::NamespacedQObject*>()
        << qMetaTypeId<SomeNamespace::NamespacedNonQObject>()
        ;

    QCOMPARE(propertyMetaTypeIds, expectedMetaTypeIds);
}

template<typename T>
struct DefaultConstructor
{
  static inline T construct() { return T(); }
};

template<typename T>
struct DefaultConstructor<T*>
{
  static inline T* construct() { return 0; }
};

void tst_Moc::autoMethodArgumentMetaTypeRegistration()
{
    AutoRegistrationObject aro;

    QVector<int> methodArgMetaTypeIds;

    const QMetaObject *metaObject = aro.metaObject();

    int i = metaObject->methodOffset(); // Start after QObject built-in slots;

    while (i < metaObject->methodCount()) {
        // Skip over signals so we start at the first slot.
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() == QMetaMethod::Signal)
            ++i;
        else
            break;

    }

#define TYPE_LOOP(TYPE) \
    { \
        const QMetaMethod method = metaObject->method(i); \
        for (int j = 0; j < method.parameterCount(); ++j) \
            methodArgMetaTypeIds.append(method.parameterType(j)); \
        QVERIFY(method.invoke(&aro, Q_ARG(TYPE, DefaultConstructor<TYPE>::construct()))); \
        ++i; \
    }

#define FOR_EACH_SLOT_ARG_TYPE(F) \
    F(QObject*) \
    F(CustomQObject2*) \
    F(QSharedPointer<CustomQObject2>) \
    F(QWeakPointer<CustomQObject2>) \
    F(QPointer<CustomQObject2>) \
    F(QList<int>) \
    F(QVector<QVariant>) \
    F(QList<CustomQObject2*>) \
    F(QVector<QList<int> >) \
    F(QVector<QList<CustomQObject2*> >) \
    F(CustomQObject2::Number) \
    F(CustomQObject2Star) \
    F(SomeNamespace2::NamespacedQObject2*) \
    F(SomeNamespace2::NamespacedNonQObject2)

    // Note: mulit-arg slots are tested below.

    FOR_EACH_SLOT_ARG_TYPE(TYPE_LOOP)

#undef TYPE_LOOP
#undef FOR_EACH_SLOT_ARG_TYPE

    QVector<int> expectedMetaTypeIds = QVector<int>()
        << QMetaType::QObjectStar
        << qMetaTypeId<CustomQObject2*>()
        << qMetaTypeId<QSharedPointer<CustomQObject2> >()
        << qMetaTypeId<QWeakPointer<CustomQObject2> >()
        << qMetaTypeId<QPointer<CustomQObject2> >()
        << qMetaTypeId<QList<int> >()
        << qMetaTypeId<QVector<QVariant> >()
        << qMetaTypeId<QList<CustomQObject2*> >()
        << qMetaTypeId<QVector<QList<int> > >()
        << qMetaTypeId<QVector<QList<CustomQObject2*> > >()
        << qMetaTypeId<CustomQObject2::Number>()
        << qMetaTypeId<CustomQObject2Star>()
        << qMetaTypeId<SomeNamespace2::NamespacedQObject2*>()
        << qMetaTypeId<SomeNamespace2::NamespacedNonQObject2>()
        ;

    QCOMPARE(methodArgMetaTypeIds, expectedMetaTypeIds);


    QVector<int> methodMultiArgMetaTypeIds;

    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu1"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(int, 42), Q_ARG(CustomObject3, CustomObject3())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu2"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(CustomObject4, CustomObject4()), Q_ARG(int, 42)));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu3"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(CustomObject5, CustomObject5()), Q_ARG(CustomObject6, CustomObject6())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu4"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(CustomObject7, CustomObject7()), Q_ARG(int, 42), Q_ARG(CustomObject8, CustomObject8())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu5"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(int, 42), Q_ARG(CustomObject9, CustomObject9()), Q_ARG(CustomObject10, CustomObject10())));
        ++i;
    }
    {
        const QMetaMethod method = metaObject->method(i);
        QCOMPARE(method.name(), QByteArray("bu6"));
        for (int j = 0; j < method.parameterCount(); ++j)
            methodMultiArgMetaTypeIds.append(method.parameterType(j));
        QVERIFY(method.invoke(&aro, Q_ARG(int, 42), Q_ARG(CustomObject11, CustomObject11()), Q_ARG(int, 42)));
        ++i;
    }

    QVector<int> expectedMultiMetaTypeIds = QVector<int>()
        << QMetaType::Int
        << qMetaTypeId<CustomObject3>()
        << qMetaTypeId<CustomObject4>()
        << QMetaType::Int
        << qMetaTypeId<CustomObject5>()
        << qMetaTypeId<CustomObject6>()
        << qMetaTypeId<CustomObject7>()
        << QMetaType::Int
        << qMetaTypeId<CustomObject8>()
        << QMetaType::Int
        << qMetaTypeId<CustomObject9>()
        << qMetaTypeId<CustomObject10>()
        << QMetaType::Int
        << qMetaTypeId<CustomObject11>()
        << QMetaType::Int
        ;

    QCOMPARE(methodMultiArgMetaTypeIds, expectedMultiMetaTypeIds);


}

void tst_Moc::autoSignalSpyMetaTypeRegistration()
{
    AutoRegistrationObject aro;

    QVector<int> methodArgMetaTypeIds;

    const QMetaObject *metaObject = aro.metaObject();

    int i = metaObject->indexOfSignal(QMetaObject::normalizedSignature("someSignal(CustomObject12)"));

    QVERIFY(i > 0);

    QCOMPARE(QMetaType::type("CustomObject12"), (int)QMetaType::UnknownType);

    QSignalSpy spy(&aro, SIGNAL(someSignal(CustomObject12)));

    QVERIFY(QMetaType::type("CustomObject12") != QMetaType::UnknownType);
    QCOMPARE(QMetaType::type("CustomObject12"), qMetaTypeId<CustomObject12>());
}

void tst_Moc::parseDefines()
{
    const QMetaObject *mo = &PD_NAMESPACE::PD_CLASSNAME::staticMetaObject;
    QCOMPARE(mo->className(), PD_SCOPED_STRING(PD_NAMESPACE, PD_CLASSNAME));
    QVERIFY(mo->indexOfSlot("voidFunction()") != -1);

    int index = mo->indexOfSlot("stringMethod()");
    QVERIFY(index != -1);
    QCOMPARE(mo->method(index).returnType(), int(QMetaType::QString));

    index = mo->indexOfSlot("combined1()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined2()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined3()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined4(int,int)");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined5()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("combined6()");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("vararg1()");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg2(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg3(int,int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg4()");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg5(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("vararg6(int,int)");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("INNERFUNCTION(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("inner_expanded(int)");
    QVERIFY(index != -1);
    index = mo->indexOfSlot("expanded_method(int)");
    QVERIFY(index != -1);

    index = mo->indexOfSlot("conditionSlot()");
    QVERIFY(index != -1);

    int count = 0;
    for (int i = 0; i < mo->classInfoCount(); ++i) {
        QMetaClassInfo mci = mo->classInfo(i);
        if (!qstrcmp(mci.name(), "TestString")) {
            ++count;
            QVERIFY(!qstrcmp(mci.value(), "PD_CLASSNAME"));
        }
        if (!qstrcmp(mci.name(), "TestString2")) {
            ++count;
            QVERIFY(!qstrcmp(mci.value(), "ParseDefine"));
        }
        if (!qstrcmp(mci.name(), "TestString3")) {
            ++count;
            QVERIFY(!qstrcmp(mci.value(), "TestValue"));
        }
    }
    QCOMPARE(count, 3);

    index = mo->indexOfSlot("PD_DEFINE_ITSELF_SUFFIX(int)");
    QVERIFY(index != -1);

    index = mo->indexOfSignal("cmdlineSignal(QMap<int,int>)");
    QVERIFY(index != -1);

    index = mo->indexOfSignal("signalQTBUG55853()");
    QVERIFY(index != -1);
}

void tst_Moc::preprocessorOnly()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start(m_moc, QStringList() << "-E" << m_sourceDirectory + QStringLiteral("/pp-dollar-signs.h"));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 0);
    QByteArray mocOut = proc.readAllStandardOutput();
    QVERIFY(!mocOut.isEmpty());
    QCOMPARE(proc.readAllStandardError(), QByteArray());

    QVERIFY(mocOut.contains("$$ = parser->createFoo()"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}


void tst_Moc::unterminatedFunctionMacro()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if defined(Q_OS_LINUX) && defined(Q_CC_GNU) && !defined(QT_NO_PROCESS)
    QProcess proc;
    proc.start(m_moc, QStringList() << "-E" << m_sourceDirectory + QStringLiteral("/unterminated-function-macro.h"));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), 1);
    QCOMPARE(proc.readAllStandardOutput(), QByteArray());
    QByteArray errorOutput = proc.readAllStandardError();
    QVERIFY(!errorOutput.isEmpty());
    QVERIFY(errorOutput.contains("missing ')' in macro usage"));
#else
    QSKIP("Only tested on linux/gcc");
#endif
}

namespace QTBUG32933_relatedObjectsDontIncludeItself {
    namespace NS {
        class Obj : QObject {
            Q_OBJECT
            Q_PROPERTY(MyEnum p1 MEMBER member)
            Q_PROPERTY(Obj::MyEnum p2 MEMBER member)
            Q_PROPERTY(NS::Obj::MyEnum p3 MEMBER member)
            Q_PROPERTY(QTBUG32933_relatedObjectsDontIncludeItself::NS::Obj::MyEnum p4 MEMBER member)
            Q_ENUMS(MyEnum);
        public:
            enum MyEnum { Something, SomethingElse };
            MyEnum member;
        };
    }
}

void tst_Moc::QTBUG32933_relatedObjectsDontIncludeItself()
{
    const QMetaObject *mo = &QTBUG32933_relatedObjectsDontIncludeItself::NS::Obj::staticMetaObject;
    const QMetaObject * const *objects = mo->d.relatedMetaObjects;
    // the related objects should be empty because the enums is in the same object.
    QVERIFY(!objects);

}

class UnrelatedClass : public QObject
{
    Q_OBJECT
    Q_ENUMS(UnrelatedEnum)
public:
    enum UnrelatedEnum {
        UnrelatedInvalidValue = -1,
        UnrelatedValue = 42
    };
};

// The presence of this macro used to confuse moc and prevent
// UnrelatedClass from being listed in the related meta objects.
Q_DECLARE_METATYPE(UnrelatedClass::UnrelatedEnum)

class TestClassReferencingUnrelatedEnum : public QObject
{
    Q_OBJECT
    Q_PROPERTY(UnrelatedClass::UnrelatedEnum enumProperty READ enumProperty WRITE setEnumProperty)
public:
    TestClassReferencingUnrelatedEnum()
        : m_enumProperty(UnrelatedClass::UnrelatedInvalidValue)
    {}

    UnrelatedClass::UnrelatedEnum enumProperty() const {
        return m_enumProperty;
    }

    void setEnumProperty(UnrelatedClass::UnrelatedEnum arg) {
        m_enumProperty = arg;
    }

private:
    UnrelatedClass::UnrelatedEnum m_enumProperty;
};

void tst_Moc::writeEnumFromUnrelatedClass()
{
    TestClassReferencingUnrelatedEnum obj;
    QString enumValueAsString("UnrelatedValue");
    obj.setProperty("enumProperty", enumValueAsString);
    QCOMPARE(int(obj.enumProperty()), int(UnrelatedClass::UnrelatedValue));
}



void tst_Moc::relatedMetaObjectsWithinNamespaces()
{
    const QMetaObject *relatedMo = &QTBUG_2151::A::staticMetaObject;

    const QMetaObject *testMo = &QTBUG_2151::B::staticMetaObject;
    QVERIFY(testMo->d.relatedMetaObjects);
    QCOMPARE(testMo->d.relatedMetaObjects[0], relatedMo);
}

void tst_Moc::relatedMetaObjectsInGadget()
{
    const QMetaObject *relatedMo = &QTBUG_35657::A::staticMetaObject;

    const QMetaObject *testMo = &QTBUG_35657::B::staticMetaObject;
    QVERIFY(testMo->d.relatedMetaObjects);
    QCOMPARE(testMo->d.relatedMetaObjects[0], relatedMo);
}

void tst_Moc::relatedMetaObjectsNameConflict_data()
{
    typedef QVector<const QMetaObject*> QMetaObjects;
    QTest::addColumn<const QMetaObject*>("dependingObject");
    QTest::addColumn<QMetaObjects>("relatedMetaObjects");

    //NS1
    const QMetaObject *n1gadget = &NS1::Gadget::staticMetaObject;
    const QMetaObject *n1object = &NS1::Object::staticMetaObject;
    const QMetaObject *n1nestedGadget = &NS1::Nested::Gadget::staticMetaObject;
    const QMetaObject *n1nestedObject = &NS1::Nested::Object::staticMetaObject;
    //N2
    const QMetaObject *n2gadget = &NS2::Gadget::staticMetaObject;
    const QMetaObject *n2object = &NS2::Object::staticMetaObject;
    const QMetaObject *n2nestedGadget = &NS2::Nested::Gadget::staticMetaObject;
    const QMetaObject *n2nestedObject = &NS2::Nested::Object::staticMetaObject;

    QTest::newRow("N1::dependingObject") << &NS1::DependingObject::staticMetaObject
                                        <<  (QMetaObjects() << n1gadget << n1object);
    QTest::newRow("N2::dependingObject") << &NS2::DependingObject::staticMetaObject
                                        <<  (QMetaObjects() << n2gadget << n2object);
    QTest::newRow("N1::dependingNestedObject") << &NS1::DependingNestedObject::staticMetaObject
                                        <<  (QMetaObjects() << n1nestedObject);
    QTest::newRow("N2::dependingNestedObject") << &NS2::DependingNestedObject::staticMetaObject
                                        <<  (QMetaObjects() << n2nestedObject);
    QTest::newRow("N1::dependingNestedGadget") << &NS1::DependingNestedGadget::staticMetaObject
                                        <<  (QMetaObjects() << n1nestedGadget);
    QTest::newRow("N2::dependingNestedGadget") << &NS2::DependingNestedGadget::staticMetaObject
                                        <<  (QMetaObjects() << n2nestedGadget);
}

void tst_Moc::relatedMetaObjectsNameConflict()
{
    typedef QVector<const QMetaObject*> QMetaObjects;
    QFETCH(const QMetaObject*, dependingObject);
    QFETCH(QMetaObjects, relatedMetaObjects);

    // load all specified metaobjects int a set
    QSet<const QMetaObject*> dependency;
    const QMetaObject *const *i = dependingObject->d.relatedMetaObjects;
    while (*i) {
        dependency.insert(*i);
        ++i;
    }

    // check if all required metaobjects are specified
    foreach (const QMetaObject *mo, relatedMetaObjects)
        QVERIFY(dependency.contains(mo));

    // check if no additional metaobjects ara specified
    QCOMPARE(dependency.size(), relatedMetaObjects.size());
}

class StringLiteralsInMacroExtension: public QObject
{
    Q_OBJECT
#define Macro(F) F " " F
    Q_CLASSINFO(Macro("String"), Macro("Literal"))
#undef Macro

#define Macro(F) F
    Q_CLASSINFO("String" Macro("!"), "Literal" Macro("!"))
    Q_CLASSINFO(Macro("!") "String", Macro("!") "Literal")
#undef Macro

#define Macro "foo"
    Q_CLASSINFO("String" Macro, "Literal" Macro)
    Q_CLASSINFO(Macro "String", Macro "Literal")
#undef Macro
};

void tst_Moc::strignLiteralsInMacroExtension()
{
    const QMetaObject *mobj = &StringLiteralsInMacroExtension::staticMetaObject;
    QCOMPARE(mobj->classInfoCount(), 5);

    QCOMPARE(mobj->classInfo(0).name(), "String String");
    QCOMPARE(mobj->classInfo(0).value(), "Literal Literal");

    QCOMPARE(mobj->classInfo(1).name(), "String!");
    QCOMPARE(mobj->classInfo(1).value(), "Literal!");

    QCOMPARE(mobj->classInfo(2).name(), "!String");
    QCOMPARE(mobj->classInfo(2).value(), "!Literal");

    QCOMPARE(mobj->classInfo(3).name(), "Stringfoo");
    QCOMPARE(mobj->classInfo(3).value(), "Literalfoo");

    QCOMPARE(mobj->classInfo(4).name(), "fooString");
    QCOMPARE(mobj->classInfo(4).value(), "fooLiteral");
}

class VeryLongStringData : public QObject
{
    Q_OBJECT

    #define repeat2(V) V V
    #define repeat4(V) repeat2(V) repeat2(V)
    #define repeat8(V) repeat4(V) repeat4(V)
    #define repeat16(V) repeat8(V) repeat8(V)
    #define repeat32(V) repeat16(V) repeat16(V)
    #define repeat64(V) repeat32(V) repeat32(V)
    #define repeat128(V) repeat64(V) repeat64(V)
    #define repeat256(V) repeat128(V) repeat128(V)
    #define repeat512(V) repeat256(V) repeat256(V)
    #define repeat1024(V) repeat512(V) repeat512(V)
    #define repeat2048(V) repeat1024(V) repeat1024(V)
    #define repeat4096(V) repeat2048(V) repeat2048(V)
    #define repeat8192(V) repeat4096(V) repeat4096(V)
    #define repeat16384(V) repeat8192(V) repeat8192(V)
    #define repeat32768(V) repeat16384(V) repeat16384(V)
    #define repeat65534(V) repeat32768(V) repeat16384(V) repeat8192(V) repeat4096(V) repeat2048(V) repeat1024(V) repeat512(V) repeat256(V) repeat128(V) repeat64(V) repeat32(V) repeat16(V) repeat8(V) repeat4(V) repeat2(V)

    Q_CLASSINFO(repeat65534("n"), repeat65534("i"))
    Q_CLASSINFO(repeat65534("e"), repeat65534("r"))
    Q_CLASSINFO(repeat32768("o"), repeat32768("b"))
    Q_CLASSINFO(":", ")")

    #undef repeat2
    #undef repeat4
    #undef repeat8
    #undef repeat16
    #undef repeat32
    #undef repeat64
    #undef repeat128
    #undef repeat256
    #undef repeat512
    #undef repeat1024
    #undef repeat2048
    #undef repeat4096
    #undef repeat8192
    #undef repeat16384
    #undef repeat32768
    #undef repeat65534
};

void tst_Moc::unnamedNamespaceObjectsAndGadgets()
{
    // these just test very basic functionality of gadgets and objects
    // defined in unnamed namespaces.
    {
        GadgetInUnnamedNS gadget(21, 42);
        QCOMPARE(gadget.x(), 21);
        QCOMPARE(gadget.y(), 42);
        gadget.staticMetaObject.property(0).writeOnGadget(&gadget, 12);
        gadget.staticMetaObject.property(1).writeOnGadget(&gadget, 24);
        QCOMPARE(gadget.x(), 12);
        QCOMPARE(gadget.y(), 24);
    }

    {
        ObjectInUnnamedNS object;
        QObject *qObject = &object;
        QCOMPARE(static_cast<ObjectInUnnamedNS *>(qObject),
                 qobject_cast<ObjectInUnnamedNS *>(qObject));
    }
}

void tst_Moc::veryLongStringData()
{
    const QMetaObject *mobj = &VeryLongStringData::staticMetaObject;
    QCOMPARE(mobj->classInfoCount(), 4);

    QCOMPARE(mobj->classInfo(0).name()[0], 'n');
    QCOMPARE(mobj->classInfo(0).value()[0], 'i');
    QCOMPARE(mobj->classInfo(1).name()[0], 'e');
    QCOMPARE(mobj->classInfo(1).value()[0], 'r');
    QCOMPARE(mobj->classInfo(2).name()[0], 'o');
    QCOMPARE(mobj->classInfo(2).value()[0], 'b');
    QCOMPARE(mobj->classInfo(3).name()[0], ':');
    QCOMPARE(mobj->classInfo(3).value()[0], ')');

    QCOMPARE(strlen(mobj->classInfo(0).name()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(0).value()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(1).name()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(1).value()), static_cast<size_t>(65534));
    QCOMPARE(strlen(mobj->classInfo(2).name()), static_cast<size_t>(32768));
    QCOMPARE(strlen(mobj->classInfo(2).value()), static_cast<size_t>(32768));
    QCOMPARE(strlen(mobj->classInfo(3).name()), static_cast<size_t>(1));
    QCOMPARE(strlen(mobj->classInfo(3).value()), static_cast<size_t>(1));
}

void tst_Moc::gadgetHierarchy()
{
    QCOMPARE(NonGadgetParent::Derived::staticMetaObject.superClass(), static_cast<const QMetaObject*>(Q_NULLPTR));
    QCOMPARE(GrandParentGadget::DerivedGadget::staticMetaObject.superClass(), &GrandParentGadget::BaseGadget::staticMetaObject);
}

void tst_Moc::optionsFileError_data()
{
    QTest::addColumn<QString>("optionsArgument");
    QTest::newRow("no filename") << QStringLiteral("@");
    QTest::newRow("nonexistent file") << QStringLiteral("@letshuntasnark");
}

void tst_Moc::optionsFileError()
{
#ifdef MOC_CROSS_COMPILED
    QSKIP("Not tested when cross-compiled");
#endif
#if !defined(QT_NO_PROCESS)
    QFETCH(QString, optionsArgument);
    QProcess p;
    p.start(m_moc, QStringList(optionsArgument));
    QVERIFY(p.waitForFinished());
    QCOMPARE(p.exitCode(), 1);
    QVERIFY(p.readAllStandardOutput().isEmpty());
    const QByteArray err = p.readAllStandardError();
    QVERIFY(err.contains("moc: "));
    QVERIFY(!err.contains("QCommandLineParser"));
#endif
}

static void checkEnum(const QMetaEnum &enumerator, const QByteArray &name, const QVector<QPair<QByteArray, int >> &keys)
{
    QCOMPARE(name, QByteArray{enumerator.name()});
    QCOMPARE(keys.size(), enumerator.keyCount());
    for (int i = 0; i < enumerator.keyCount(); ++i) {
        QCOMPARE(keys[i].first, QByteArray{enumerator.key(i)});
        QCOMPARE(keys[i].second, enumerator.value(i));
    }
}

void tst_Moc::testQNamespace()
{
    QCOMPARE(TestQNamespace::staticMetaObject.enumeratorCount(), 4);
    checkEnum(TestQNamespace::staticMetaObject.enumerator(0), "TestEnum1",
                {{"Key1", 11}, {"Key2", 12}});
    checkEnum(TestQNamespace::staticMetaObject.enumerator(1), "TestEnum2",
                {{"Key1", 17}, {"Key2", 18}});
    checkEnum(TestQNamespace::staticMetaObject.enumerator(2), "TestFlag1",
                {{"None", 0}, {"Flag1", 1}, {"Flag2", 2}, {"Any", 1 | 2}});
    checkEnum(TestQNamespace::staticMetaObject.enumerator(3), "TestFlag2",
                {{"None", 0}, {"Flag1", 4}, {"Flag2", 8}, {"Any", 4 | 8}});

    QCOMPARE(TestQNamespace::TestGadget::staticMetaObject.enumeratorCount(), 2);
    checkEnum(TestQNamespace::TestGadget::staticMetaObject.enumerator(0), "TestGEnum1",
                {{"Key1", 13}, {"Key2", 14}});
    checkEnum(TestQNamespace::TestGadget::staticMetaObject.enumerator(1), "TestGEnum2",
                {{"Key1", 23}, {"Key2", 24}});

    QMetaEnum meta = QMetaEnum::fromType<TestQNamespace::TestEnum1>();
    QVERIFY(meta.isValid());
    QCOMPARE(meta.name(), "TestEnum1");
    QCOMPARE(meta.enclosingMetaObject(), &TestQNamespace::staticMetaObject);
    QCOMPARE(meta.keyCount(), 2);

    QCOMPARE(FooNamespace::staticMetaObject.enumeratorCount(), 1);
    QCOMPARE(FooNamespace::FooNestedNamespace::staticMetaObject.enumeratorCount(), 2);
    QCOMPARE(FooNamespace::FooNestedNamespace::FooMoreNestedNamespace::staticMetaObject.enumeratorCount(), 1);
}

QTEST_MAIN(tst_Moc)

// the generated code must compile with QT_NO_KEYWORDS
#undef signals
#undef slots
#undef emit

#include "tst_moc.moc"

