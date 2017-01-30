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
#include <qcoreapplication.h>
#include <qmetatype.h>
#include <QtTest/QtTest>

#include <QtDBus/QtDBus>
#include <private/qdbusmetaobject_p.h>

class tst_QDBusMetaObject: public QObject
{
    Q_OBJECT

    QHash<QString, QDBusMetaObject *> map;
public slots:
    void init();

private slots:
    void initTestCase();
    void types_data();
    void types();
    void methods_data();
    void methods();
    void _signals_data();
    void _signals();
    void properties_data();
    void properties();
};

typedef QPair<QString,QString> StringPair;

struct Struct1 { };             // (s)
struct Struct4                  // (ssa(ss)sayasx)
{
    QString m1;
    QString m2;
    QList<StringPair> m3;
    QString m4;
    QByteArray m5;
    QStringList m6;
    qlonglong m7;
};

Q_DECLARE_METATYPE(Struct1)
Q_DECLARE_METATYPE(Struct4)
Q_DECLARE_METATYPE(StringPair)

Q_DECLARE_METATYPE(const QMetaObject*)

QT_BEGIN_NAMESPACE
QDBusArgument &operator<<(QDBusArgument &arg, const Struct1 &)
{
    arg.beginStructure();
    arg << QString();
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const StringPair &s)
{
    arg.beginStructure();
    arg << s.first << s.second;
    arg.endStructure();
    return arg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const Struct4 &s)
{
    arg.beginStructure();
    arg << s.m1 << s.m2 << s.m3 << s.m4 << s.m5 << s.m6 << s.m7;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, Struct1 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, Struct4 &)
{ return arg; }
const QDBusArgument &operator>>(const QDBusArgument &arg, StringPair &)
{ return arg; }
QT_END_NAMESPACE

void tst_QDBusMetaObject::initTestCase()
{
    qDBusRegisterMetaType<Struct1>();
    qDBusRegisterMetaType<Struct4>();
    qDBusRegisterMetaType<StringPair>();

    qDBusRegisterMetaType<QList<Struct1> >();
    qDBusRegisterMetaType<QList<Struct4> >();
}

void tst_QDBusMetaObject::init()
{
    qDeleteAll(map);
    map.clear();
}

// test classes
class TypesTest1: public QObject
{
    Q_OBJECT

signals:
    void signal(uchar);
};
const char TypesTest1_xml[] =
    "<signal name=\"signal\"><arg type=\"y\"/></signal>";

class TypesTest2: public QObject
{
    Q_OBJECT

signals:
    void signal(bool);
};
const char TypesTest2_xml[] =
    "<signal name=\"signal\"><arg type=\"b\"/></signal>";

class TypesTest3: public QObject
{
    Q_OBJECT

signals:
    void signal(short);
};
const char TypesTest3_xml[] =
    "<signal name=\"signal\"><arg type=\"n\"/></signal>";

class TypesTest4: public QObject
{
    Q_OBJECT

signals:
    void signal(ushort);
};
const char TypesTest4_xml[] =
    "<signal name=\"signal\"><arg type=\"q\"/></signal>";

class TypesTest5: public QObject
{
    Q_OBJECT

signals:
    void signal(int);
};
const char TypesTest5_xml[] =
    "<signal name=\"signal\"><arg type=\"i\"/></signal>";

class TypesTest6: public QObject
{
    Q_OBJECT

signals:
    void signal(uint);
};
const char TypesTest6_xml[] =
    "<signal name=\"signal\"><arg type=\"u\"/></signal>";

class TypesTest7: public QObject
{
    Q_OBJECT

signals:
    void signal(qlonglong);
};
const char TypesTest7_xml[] =
    "<signal name=\"signal\"><arg type=\"x\"/></signal>";

class TypesTest8: public QObject
{
    Q_OBJECT

signals:
    void signal(qulonglong);
};
const char TypesTest8_xml[] =
    "<signal name=\"signal\"><arg type=\"t\"/></signal>";

class TypesTest9: public QObject
{
    Q_OBJECT

signals:
    void signal(double);
};
const char TypesTest9_xml[] =
    "<signal name=\"signal\"><arg type=\"d\"/></signal>";

class TypesTest10: public QObject
{
    Q_OBJECT

signals:
    void signal(QString);
};
const char TypesTest10_xml[] =
    "<signal name=\"signal\"><arg type=\"s\"/></signal>";

class TypesTest11: public QObject
{
    Q_OBJECT

signals:
    void signal(QDBusObjectPath);
};
const char TypesTest11_xml[] =
    "<signal name=\"signal\"><arg type=\"o\"/></signal>";

class TypesTest12: public QObject
{
    Q_OBJECT

signals:
    void signal(QDBusSignature);
};
const char TypesTest12_xml[] =
    "<signal name=\"signal\"><arg type=\"g\"/></signal>";

class TypesTest13: public QObject
{
    Q_OBJECT

signals:
    void signal(QDBusVariant);
};
const char TypesTest13_xml[] =
    "<signal name=\"signal\"><arg type=\"v\"/></signal>";

class TypesTest14: public QObject
{
    Q_OBJECT

signals:
    void signal(QStringList);
};
const char TypesTest14_xml[] =
    "<signal name=\"signal\"><arg type=\"as\"/></signal>";

class TypesTest15: public QObject
{
    Q_OBJECT

signals:
    void signal(QByteArray);
};
const char TypesTest15_xml[] =
    "<signal name=\"signal\"><arg type=\"ay\"/></signal>";

class TypesTest16: public QObject
{
    Q_OBJECT

signals:
    void signal(StringPair);
};
const char TypesTest16_xml[] =
    "<signal name=\"signal\"><arg type=\"(ss)\"/>"
    "<annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"StringPair\"></signal>";

class TypesTest17: public QObject
{
    Q_OBJECT

signals:
    void signal(Struct1);
};
const char TypesTest17_xml[] =
    "<signal name=\"signal\"><arg type=\"(s)\"/>"
    "<annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"Struct1\"></signal>";

class TypesTest18: public QObject
{
    Q_OBJECT

signals:
    void signal(Struct4);
};
const char TypesTest18_xml[] =
    "<signal name=\"signal\"><arg type=\"(ssa(ss)sayasx)\"/>"
    "<annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"Struct4\"></signal>";

class TypesTest19: public QObject
{
    Q_OBJECT

signals:
    void signal(QVariantList);
};
const char TypesTest19_xml[] =
    "<signal name=\"signal\"><arg type=\"av\"/>"
    "<annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QVariantList\"></signal>";

class TypesTest20: public QObject
{
    Q_OBJECT

signals:
    void signal(QVariantMap);
};
const char TypesTest20_xml[] =
    "<signal name=\"signal\"><arg type=\"a{sv}\"/>"
    "<annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"QVariantMap\"></signal>";
const char TypesTest20_oldxml[] =
    "<signal name=\"signal\"><arg type=\"a{sv}\"/>"
    "<annotation name=\"com.trolltech.QtDBus.QtTypeName.Out0\" value=\"QVariantMap\"></signal>";


void tst_QDBusMetaObject::types_data()
{
    QTest::addColumn<const QMetaObject *>("metaobject");
    QTest::addColumn<QString>("xml");

    QTest::newRow("byte") << &TypesTest1::staticMetaObject << QString(TypesTest1_xml);
    QTest::newRow("bool") << &TypesTest2::staticMetaObject << QString(TypesTest2_xml);
    QTest::newRow("short") << &TypesTest3::staticMetaObject << QString(TypesTest3_xml);
    QTest::newRow("ushort") << &TypesTest4::staticMetaObject << QString(TypesTest4_xml);
    QTest::newRow("int") << &TypesTest5::staticMetaObject << QString(TypesTest5_xml);
    QTest::newRow("uint") << &TypesTest6::staticMetaObject << QString(TypesTest6_xml);
    QTest::newRow("qlonglong") << &TypesTest7::staticMetaObject << QString(TypesTest7_xml);
    QTest::newRow("qulonglong") << &TypesTest8::staticMetaObject << QString(TypesTest8_xml);
    QTest::newRow("double") << &TypesTest9::staticMetaObject << QString(TypesTest9_xml);
    QTest::newRow("QString") << &TypesTest10::staticMetaObject << QString(TypesTest10_xml);
    QTest::newRow("QDBusObjectPath") << &TypesTest11::staticMetaObject << QString(TypesTest11_xml);
    QTest::newRow("QDBusSignature") << &TypesTest12::staticMetaObject << QString(TypesTest12_xml);
    QTest::newRow("QDBusVariant") << &TypesTest13::staticMetaObject << QString(TypesTest13_xml);
    QTest::newRow("QStringList") << &TypesTest14::staticMetaObject << QString(TypesTest14_xml);
    QTest::newRow("QByteArray") << &TypesTest15::staticMetaObject << QString(TypesTest15_xml);
    QTest::newRow("StringPair") << &TypesTest16::staticMetaObject << QString(TypesTest16_xml);
    QTest::newRow("Struct1") << &TypesTest17::staticMetaObject << QString(TypesTest17_xml);
    QTest::newRow("Struct4") << &TypesTest18::staticMetaObject << QString(TypesTest18_xml);
    QTest::newRow("QVariantList") << &TypesTest19::staticMetaObject << QString(TypesTest19_xml);
    QTest::newRow("QVariantMap") << &TypesTest20::staticMetaObject << QString(TypesTest20_xml);
    QTest::newRow("QVariantMap-oldannotation") << &TypesTest20::staticMetaObject << QString(TypesTest20_oldxml);
}

void tst_QDBusMetaObject::types()
{
    QFETCH(const QMetaObject*, metaobject);
    QFETCH(QString, xml);

    // add the rest of the XML tags
    xml = QString("<node><interface name=\"local.Interface\">%1</interface></node>")
          .arg(xml);

    QDBusError error;

    const QScopedPointer<QDBusMetaObject> result(QDBusMetaObject::createMetaObject("local.Interface", xml, map, error));
    QVERIFY2(result, qPrintable(error.message()));

    QCOMPARE(result->enumeratorCount(), 0);
    QCOMPARE(result->classInfoCount(), 0);

    // compare the meta objects
    QCOMPARE(result->methodCount() - result->methodOffset(),
             metaobject->methodCount() - metaobject->methodOffset());
    QCOMPARE(result->propertyCount() - result->propertyOffset(),
             metaobject->propertyCount() - metaobject->propertyOffset());

    for (int i = metaobject->methodOffset(); i < metaobject->methodCount(); ++i) {
        QMetaMethod expected = metaobject->method(i);

        int methodIdx = result->indexOfMethod(expected.methodSignature().constData());
        QVERIFY(methodIdx != -1);
        QMetaMethod constructed = result->method(methodIdx);

        QCOMPARE(int(constructed.access()), int(expected.access()));
        QCOMPARE(int(constructed.methodType()), int(expected.methodType()));
        QCOMPARE(constructed.name(), expected.name());
        QCOMPARE(constructed.parameterCount(), expected.parameterCount());
        QCOMPARE(constructed.parameterNames(), expected.parameterNames());
        QCOMPARE(constructed.parameterTypes(), expected.parameterTypes());
        for (int j = 0; j < constructed.parameterCount(); ++j)
            QCOMPARE(constructed.parameterType(j), expected.parameterType(j));
        QCOMPARE(constructed.tag(), expected.tag());
        QCOMPARE(constructed.typeName(), expected.typeName());
        QCOMPARE(constructed.returnType(), expected.returnType());
    }

    for (int i = metaobject->propertyOffset(); i < metaobject->propertyCount(); ++i) {
        QMetaProperty expected = metaobject->property(i);

        int propIdx = result->indexOfProperty(expected.name());
        QVERIFY(propIdx != -1);
        QMetaProperty constructed = result->property(propIdx);

        QCOMPARE(constructed.isDesignable(), expected.isDesignable());
        QCOMPARE(constructed.isEditable(), expected.isEditable());
        QCOMPARE(constructed.isEnumType(), expected.isEnumType());
        QCOMPARE(constructed.isFlagType(), expected.isFlagType());
        QCOMPARE(constructed.isReadable(), expected.isReadable());
        QCOMPARE(constructed.isResettable(), expected.isResettable());
        QCOMPARE(constructed.isScriptable(), expected.isScriptable());
        QCOMPARE(constructed.isStored(), expected.isStored());
        QCOMPARE(constructed.isUser(), expected.isUser());
        QCOMPARE(constructed.isWritable(), expected.isWritable());
        QCOMPARE(constructed.typeName(), expected.typeName());
        QCOMPARE(constructed.type(), expected.type());
        QCOMPARE(constructed.userType(), expected.userType());
    }
}

class MethodTest1: public QObject
{
    Q_OBJECT

public slots:
    void method() { }
};
const char MethodTest1_xml[] =
    "<method name=\"method\" />";

class MethodTest2: public QObject
{
    Q_OBJECT

public slots:
    void method(int) { }
};
const char MethodTest2_xml[] =
    "<method name=\"method\"><arg direction=\"in\" type=\"i\"/></method>";

class MethodTest3: public QObject
{
    Q_OBJECT

public slots:
    void method(int input0) { Q_UNUSED(input0); }
};
const char MethodTest3_xml[] =
    "<method name=\"method\"><arg direction=\"in\" type=\"i\" name=\"input0\"/></method>";

class MethodTest4: public QObject
{
    Q_OBJECT

public slots:
    int method() { return 0; }
};
const char MethodTest4_xml[] =
    "<method name=\"method\"><arg direction=\"out\" type=\"i\"/></method>";
const char MethodTest4_xml2[] =
    "<method name=\"method\"><arg direction=\"out\" type=\"i\" name=\"thisShouldNeverBeSeen\"/></method>";

class MethodTest5: public QObject
{
    Q_OBJECT

public slots:
    int method(int input0) { return input0; }
};
const char MethodTest5_xml[] =
    "<method name=\"method\">"
    "<arg direction=\"in\" type=\"i\" name=\"input0\"/>"
    "<arg direction=\"out\" type=\"i\"/>"
    "</method>";

class MethodTest6: public QObject
{
    Q_OBJECT

public slots:
    int method(int input0, int input1) { Q_UNUSED(input0); return input1; }
};
const char MethodTest6_xml[] =
    "<method name=\"method\">"
    "<arg direction=\"in\" type=\"i\" name=\"input0\"/>"
    "<arg direction=\"out\" type=\"i\"/>"
    "<arg direction=\"in\" type=\"i\" name=\"input1\"/>"
    "</method>";

class MethodTest7: public QObject
{
    Q_OBJECT

public slots:
    int method(int input0, int input1, int &output1) { output1 = input1; return input0; }
};
const char MethodTest7_xml[] =
    "<method name=\"method\">"
    "<arg direction=\"in\" type=\"i\" name=\"input0\"/>"
    "<arg direction=\"in\" type=\"i\" name=\"input1\"/>"
    "<arg direction=\"out\" type=\"i\"/>"
    "<arg direction=\"out\" type=\"i\" name=\"output1\"/>"
    "</method>";

class MethodTest8: public QObject
{
    Q_OBJECT

public slots:
    int method(int input0, int input1, int &output1, int &output2) { output1 = output2 = input1; return input0; }
};
const char MethodTest8_xml[] =
    "<method name=\"method\">"
    "<arg direction=\"in\" type=\"i\" name=\"input0\"/>"
    "<arg direction=\"in\" type=\"i\" name=\"input1\"/>"
    "<arg direction=\"out\" type=\"i\"/>"
    "<arg direction=\"out\" type=\"i\" name=\"output1\"/>"
    "<arg direction=\"out\" type=\"i\" name=\"output2\"/>"
    "</method>";

class MethodTest9: public QObject
{
    Q_OBJECT

public slots:
    Q_NOREPLY void method(int) { }
};
const char MethodTest9_xml[] =
    "<method name=\"method\">"
    "<arg direction=\"in\" type=\"i\"/>"
    "<annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>"
    "</method>";

void tst_QDBusMetaObject::methods_data()
{
    QTest::addColumn<const QMetaObject *>("metaobject");
    QTest::addColumn<QString>("xml");

    QTest::newRow("void-void") << &MethodTest1::staticMetaObject << QString(MethodTest1_xml);
    QTest::newRow("void-int") << &MethodTest2::staticMetaObject << QString(MethodTest2_xml);
    QTest::newRow("void-int-with-name") << &MethodTest3::staticMetaObject << QString(MethodTest3_xml);
    QTest::newRow("int-void") << &MethodTest4::staticMetaObject << QString(MethodTest4_xml);
    QTest::newRow("int-void2") << &MethodTest4::staticMetaObject << QString(MethodTest4_xml2);
    QTest::newRow("int-int") << &MethodTest5::staticMetaObject << QString(MethodTest5_xml);
    QTest::newRow("int-int,int") << &MethodTest6::staticMetaObject << QString(MethodTest6_xml);
    QTest::newRow("int,int-int,int") << &MethodTest7::staticMetaObject << QString(MethodTest7_xml);
    QTest::newRow("int,int,int-int,int") << &MethodTest8::staticMetaObject << QString(MethodTest8_xml);
    QTest::newRow("Q_ASYNC") << &MethodTest9::staticMetaObject << QString(MethodTest9_xml);
}

void tst_QDBusMetaObject::methods()
{
    types();
}

class SignalTest1: public QObject
{
    Q_OBJECT

signals:
    void signal();
};
const char SignalTest1_xml[] =
    "<signal name=\"signal\" />";

class SignalTest2: public QObject
{
    Q_OBJECT

signals:
    void signal(int);
};
const char SignalTest2_xml[] =
    "<signal name=\"signal\"><arg type=\"i\"/></signal>";

class SignalTest3: public QObject
{
    Q_OBJECT

signals:
    void signal(int output0);
};
const char SignalTest3_xml[] =
    "<signal name=\"signal\"><arg type=\"i\" name=\"output0\"/></signal>";

class SignalTest4: public QObject
{
    Q_OBJECT

signals:
    void signal(int output0, int);
};
const char SignalTest4_xml[] =
    "<signal name=\"signal\"><arg type=\"i\" name=\"output0\"/><arg type=\"i\"/></signal>";

void tst_QDBusMetaObject::_signals_data()
{
    QTest::addColumn<const QMetaObject *>("metaobject");
    QTest::addColumn<QString>("xml");

    QTest::newRow("empty") << &SignalTest1::staticMetaObject << QString(SignalTest1_xml);
    QTest::newRow("int") << &SignalTest2::staticMetaObject << QString(SignalTest2_xml);
    QTest::newRow("int output0") << &SignalTest3::staticMetaObject << QString(SignalTest3_xml);
    QTest::newRow("int output0,int") << &SignalTest4::staticMetaObject << QString(SignalTest4_xml);
}

void tst_QDBusMetaObject::_signals()
{
    types();
}

class PropertyTest1: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int property READ property)
public:
    int property() { return 0; }
    void setProperty(int) { }
};
const char PropertyTest1_xml[] =
    "<property name=\"property\" type=\"i\" access=\"read\"/>";

class PropertyTest2: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int property READ property WRITE setProperty)
public:
    int property() { return 0; }
    void setProperty(int) { }
};
const char PropertyTest2_xml[] =
    "<property name=\"property\" type=\"i\" access=\"readwrite\"/>";

class PropertyTest3: public QObject
{
    Q_OBJECT
    Q_PROPERTY(int property WRITE setProperty)
public:
    int property() { return 0; }
    void setProperty(int) { }
};
const char PropertyTest3_xml[] =
    "<property name=\"property\" type=\"i\" access=\"write\"/>";

class PropertyTest4: public QObject
{
    Q_OBJECT
    Q_PROPERTY(Struct1 property WRITE setProperty)
public:
    Struct1 property() { return Struct1(); }
    void setProperty(Struct1) { }
};
const char PropertyTest4_xml[] =
    "<property name=\"property\" type=\"(s)\" access=\"write\">"
    "<annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"Struct1\"/>"
    "</property>";

class PropertyTest_b: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool property READ property WRITE setProperty)
public:
    bool property() { return false; }
    void setProperty(bool) { }
};
const char PropertyTest_b_xml[] =
    "<property name=\"property\" type=\"b\" access=\"readwrite\"/>";

class PropertyTest_y: public QObject
{
    Q_OBJECT
    Q_PROPERTY(uchar property READ property WRITE setProperty)
public:
    uchar property() { return 0; }
    void setProperty(uchar) { }
};
const char PropertyTest_y_xml[] =
    "<property name=\"property\" type=\"y\" access=\"readwrite\"/>";

class PropertyTest_n: public QObject
{
    Q_OBJECT
    Q_PROPERTY(short property READ property WRITE setProperty)
public:
    short property() { return 0; }
    void setProperty(short) { }
};
const char PropertyTest_n_xml[] =
    "<property name=\"property\" type=\"n\" access=\"readwrite\"/>";

class PropertyTest_q: public QObject
{
    Q_OBJECT
    Q_PROPERTY(ushort property READ property WRITE setProperty)
public:
    ushort property() { return 0; }
    void setProperty(ushort) { }
};
const char PropertyTest_q_xml[] =
    "<property name=\"property\" type=\"q\" access=\"readwrite\"/>";

class PropertyTest_u: public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint property READ property WRITE setProperty)
public:
    uint property() { return 0; }
    void setProperty(uint) { }
};
const char PropertyTest_u_xml[] =
    "<property name=\"property\" type=\"u\" access=\"readwrite\"/>";

class PropertyTest_x: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qlonglong property READ property WRITE setProperty)
public:
    qlonglong property() { return 0; }
    void setProperty(qlonglong) { }
};
const char PropertyTest_x_xml[] =
    "<property name=\"property\" type=\"x\" access=\"readwrite\"/>";

class PropertyTest_t: public QObject
{
    Q_OBJECT
    Q_PROPERTY(qulonglong property READ property WRITE setProperty)
public:
    qulonglong property() { return 0; }
    void setProperty(qulonglong) { }
};
const char PropertyTest_t_xml[] =
    "<property name=\"property\" type=\"t\" access=\"readwrite\"/>";

class PropertyTest_d: public QObject
{
    Q_OBJECT
    Q_PROPERTY(double property READ property WRITE setProperty)
public:
    double property() { return 0; }
    void setProperty(double) { }
};
const char PropertyTest_d_xml[] =
    "<property name=\"property\" type=\"d\" access=\"readwrite\"/>";

class PropertyTest_s: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString property READ property WRITE setProperty)
public:
    QString property() { return QString(); }
    void setProperty(QString) { }
};
const char PropertyTest_s_xml[] =
    "<property name=\"property\" type=\"s\" access=\"readwrite\"/>";

class PropertyTest_v: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDBusVariant property READ property WRITE setProperty)
public:
    QDBusVariant property() { return QDBusVariant(); }
    void setProperty(QDBusVariant) { }
};
const char PropertyTest_v_xml[] =
    "<property name=\"property\" type=\"v\" access=\"readwrite\"/>";

class PropertyTest_o: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDBusObjectPath property READ property WRITE setProperty)
public:
    QDBusObjectPath property() { return QDBusObjectPath(); }
    void setProperty(QDBusObjectPath) { }
};
const char PropertyTest_o_xml[] =
    "<property name=\"property\" type=\"o\" access=\"readwrite\"/>";

class PropertyTest_g: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDBusSignature property READ property WRITE setProperty)
public:
    QDBusSignature property() { return QDBusSignature(); }
    void setProperty(QDBusSignature) { }
};
const char PropertyTest_g_xml[] =
    "<property name=\"property\" type=\"g\" access=\"readwrite\"/>";

class PropertyTest_h: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QDBusUnixFileDescriptor property READ property WRITE setProperty)
public:
    QDBusUnixFileDescriptor property() { return QDBusUnixFileDescriptor(); }
    void setProperty(QDBusUnixFileDescriptor) { }
};
const char PropertyTest_h_xml[] =
    "<property name=\"property\" type=\"h\" access=\"readwrite\"/>";

class PropertyTest_ay: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QByteArray property READ property WRITE setProperty)
public:
    QByteArray property() { return QByteArray(); }
    void setProperty(QByteArray) { }
};
const char PropertyTest_ay_xml[] =
    "<property name=\"property\" type=\"ay\" access=\"readwrite\"/>";

class PropertyTest_as: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList property READ property WRITE setProperty)
public:
    QStringList property() { return QStringList(); }
    void setProperty(QStringList) { }
};
const char PropertyTest_as_xml[] =
    "<property name=\"property\" type=\"as\" access=\"readwrite\"/>";

class PropertyTest_av: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList property READ property WRITE setProperty)
public:
    QVariantList property() { return QVariantList(); }
    void setProperty(QVariantList) { }
};
const char PropertyTest_av_xml[] =
    "<property name=\"property\" type=\"av\" access=\"readwrite\"/>";

class PropertyTest_ao: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QDBusObjectPath> property READ property WRITE setProperty)
public:
    QList<QDBusObjectPath> property() { return QList<QDBusObjectPath>(); }
    void setProperty(QList<QDBusObjectPath>) { }
};
const char PropertyTest_ao_xml[] =
    "<property name=\"property\" type=\"ao\" access=\"readwrite\"/>";

class PropertyTest_ag: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QDBusSignature> property READ property WRITE setProperty)
public:
    QList<QDBusSignature> property() { return QList<QDBusSignature>(); }
    void setProperty(QList<QDBusSignature>) { }
};
const char PropertyTest_ag_xml[] =
    "<property name=\"property\" type=\"ag\" access=\"readwrite\"/>";

void tst_QDBusMetaObject::properties_data()
{
    QTest::addColumn<const QMetaObject *>("metaobject");
    QTest::addColumn<QString>("xml");

    QTest::newRow("read") << &PropertyTest1::staticMetaObject << QString(PropertyTest1_xml);
    QTest::newRow("readwrite") << &PropertyTest2::staticMetaObject << QString(PropertyTest2_xml);
    QTest::newRow("write") << &PropertyTest3::staticMetaObject << QString(PropertyTest3_xml);
    QTest::newRow("customtype") << &PropertyTest4::staticMetaObject << QString(PropertyTest4_xml);

    QTest::newRow("bool") << &PropertyTest_b::staticMetaObject << QString(PropertyTest_b_xml);
    QTest::newRow("byte") << &PropertyTest_y::staticMetaObject << QString(PropertyTest_y_xml);
    QTest::newRow("short") << &PropertyTest_n::staticMetaObject << QString(PropertyTest_n_xml);
    QTest::newRow("ushort") << &PropertyTest_q::staticMetaObject << QString(PropertyTest_q_xml);
    QTest::newRow("uint") << &PropertyTest_u::staticMetaObject << QString(PropertyTest_u_xml);
    QTest::newRow("qlonglong") << &PropertyTest_x::staticMetaObject << QString(PropertyTest_x_xml);
    QTest::newRow("qulonglong") << &PropertyTest_t::staticMetaObject << QString(PropertyTest_t_xml);
    QTest::newRow("double") << &PropertyTest_d::staticMetaObject << QString(PropertyTest_d_xml);
    QTest::newRow("QString") << &PropertyTest_s::staticMetaObject << QString(PropertyTest_s_xml);
    QTest::newRow("QDBusVariant") << &PropertyTest_v::staticMetaObject << QString(PropertyTest_v_xml);
    QTest::newRow("QDBusObjectPath") << &PropertyTest_o::staticMetaObject << QString(PropertyTest_o_xml);
    QTest::newRow("QDBusSignature") << &PropertyTest_g::staticMetaObject << QString(PropertyTest_g_xml);
    QTest::newRow("QDBusUnixFileDescriptor") << &PropertyTest_h::staticMetaObject << QString(PropertyTest_h_xml);
    QTest::newRow("QByteArray") << &PropertyTest_ay::staticMetaObject << QString(PropertyTest_ay_xml);
    QTest::newRow("QStringList") << &PropertyTest_as::staticMetaObject << QString(PropertyTest_as_xml);
    QTest::newRow("QVariantList") << &PropertyTest_av::staticMetaObject << QString(PropertyTest_av_xml);
    QTest::newRow("QList<QDBusObjectPath>") << &PropertyTest_ao::staticMetaObject << QString(PropertyTest_ao_xml);
    QTest::newRow("QList<QDBusSignature>") << &PropertyTest_ag::staticMetaObject << QString(PropertyTest_ag_xml);
}

void tst_QDBusMetaObject::properties()
{
    types();
}


QTEST_MAIN(tst_QDBusMetaObject)
#include "tst_qdbusmetaobject.moc"
