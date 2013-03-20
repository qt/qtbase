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
/* -*- C++ -*-
 */
#include <qcoreapplication.h>
#include <qmetatype.h>
#include <QtTest/QtTest>
#include <QtDBus/QtDBus>
#include <QtXml/QDomDocument>

#define USE_PRIVATE_CODE
#include "../qdbusmarshall/common.h"

class tst_QDBusXmlParser: public QObject
{
    Q_OBJECT

private:
    void parsing_common(const QString&);
    QString clean_xml(const QString&);

private slots:
    void initTestCase();
    void parsing_data();
    void parsing();
    void parsingWithDoctype_data();
    void parsingWithDoctype();

    void methods_data();
    void methods();
    void signals__data();
    void signals_();
    void properties_data();
    void properties();
};

QT_BEGIN_NAMESPACE
// Avoid QHash randomization so that the order of the XML attributes is stable
extern Q_CORE_EXPORT QBasicAtomicInt qt_qhash_seed; // from qhash.cpp
QT_END_NAMESPACE

void tst_QDBusXmlParser::initTestCase()
{
    // If the seed not initialized yet (-1), set it to 0
    // otherwise abort, so we don't get unexplained test failures later.
    QVERIFY(qt_qhash_seed.testAndSetRelaxed(-1, 0));
}

void tst_QDBusXmlParser::parsing_data()
{
    QTest::addColumn<QString>("xmlData");
    QTest::addColumn<int>("interfaceCount");
    QTest::addColumn<int>("objectCount");
    QTest::addColumn<int>("annotationCount");
    QTest::addColumn<QStringList>("introspection");

    QStringList introspection;

    QTest::newRow("null") << QString() << 0 << 0 << 0 << introspection;
    QTest::newRow("empty") << QString("") << 0 << 0 << 0 << introspection;

    QTest::newRow("junk") << "<junk/>" << 0 << 0 << 0 << introspection;
    QTest::newRow("interface-inside-junk") << "<junk><interface name=\"iface.iface1\" /></junk>"
                                           << 0 << 0 << 0 << introspection;
    QTest::newRow("object-inside-junk") << "<junk><node name=\"obj1\" /></junk>"
                                        << 0 << 0 << 0 << introspection;

    QTest::newRow("zero-interfaces") << "<node/>" << 0 << 0 << 0 << introspection;

    introspection << "<interface name=\"iface.iface1\"/>";
    QTest::newRow("one-interface") << "<node><interface name=\"iface.iface1\" /></node>"
                                   << 1 << 0 << 0 << introspection;
    introspection.clear();

    introspection << "<interface name=\"iface.iface1\"/>"
                  << "<interface name=\"iface.iface2\"/>";
    QTest::newRow("two-interfaces") << "<node><interface name=\"iface.iface1\" />"
                                       "<interface name=\"iface.iface2\" /></node>"
                                    << 2 << 0 << 0 << introspection;
    introspection.clear();


    QTest::newRow("one-object") << "<node><node name=\"obj1\"/></node>"
                                << 0 << 1 << 0 << introspection;
    QTest::newRow("two-objects") << "<node><node name=\"obj1\"/><node name=\"obj2\"/></node>"
                                 << 0 << 2 << 0 << introspection;

    introspection << "<interface name=\"iface.iface1\"/>";
    QTest::newRow("i1o1") << "<node><interface name=\"iface.iface1\"/><node name=\"obj1\"/></node>"
                          << 1 << 1 << 0 << introspection;
    introspection.clear();

    introspection << "<interface name=\"iface.iface1\">"
                     "  <annotation name=\"foo.testing\" value=\"nothing to see here\"/>"
                     "</interface>";
    QTest::newRow("one-interface-annotated") << "<node><interface name=\"iface.iface1\">"
                                                "<annotation name=\"foo.testing\" value=\"nothing to see here\" />"
                                                "</interface></node>" << 1 << 0 << 1 << introspection;
    introspection.clear();


    introspection << "<interface name=\"iface.iface1\"/>";
    QTest::newRow("one-interface-docnamespace") << "<?xml version=\"1.0\" xmlns:doc=\"foo\" ?><node>"
                                                   "<interface name=\"iface.iface1\"><doc:something />"
                                                   "</interface></node>" << 1 << 0 << 0 << introspection;
    introspection.clear();
}

void tst_QDBusXmlParser::parsing_common(const QString &xmlData)
{
    QDBusIntrospection::Object obj =
        QDBusIntrospection::parseObject(xmlData, "local.testing", "/");
    QFETCH(int, interfaceCount);
    QFETCH(int, objectCount);
    QFETCH(int, annotationCount);
    QFETCH(QStringList, introspection);
    QCOMPARE(obj.interfaces.count(), interfaceCount);
    QCOMPARE(obj.childObjects.count(), objectCount);
    QCOMPARE(QDBusIntrospection::parseInterface(xmlData).annotations.count(), annotationCount);

    QDBusIntrospection::Interfaces ifaces = QDBusIntrospection::parseInterfaces(xmlData);

    // also verify the naming
    int i = 0;
    foreach (QString name, obj.interfaces) {
        const QString expectedName = QString("iface.iface%1").arg(i+1);
        QCOMPARE(name, expectedName);

        const QString expectedIntrospection = clean_xml(introspection.at(i++));
        const QString resultIntrospection = clean_xml(ifaces.value(expectedName)->introspection);
        QCOMPARE(resultIntrospection, expectedIntrospection);
    }

    i = 0;
    foreach (QString name, obj.childObjects)
        QCOMPARE(name, QString("obj%1").arg(++i));
}

QString tst_QDBusXmlParser::clean_xml(const QString &xmlData)
{
    QDomDocument dom;
    dom.setContent(xmlData);
    return dom.toString();
}

void tst_QDBusXmlParser::parsing()
{
    QFETCH(QString, xmlData);

    parsing_common(xmlData);
}

void tst_QDBusXmlParser::parsingWithDoctype_data()
{
    parsing_data();
}

void tst_QDBusXmlParser::parsingWithDoctype()
{
    QString docType = "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
                      "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n";
    QFETCH(QString, xmlData);

    QString toParse;
    if (xmlData.startsWith(QLatin1String("<?xml"))) {
        int split = xmlData.indexOf(QLatin1Char('>')) + 1;
        toParse = xmlData.left(split) + docType + xmlData.mid(split);
    } else {
        toParse = docType + xmlData;
    }
    parsing_common(toParse);
}

void tst_QDBusXmlParser::methods_data()
{
    QTest::addColumn<QString>("xmlDataFragment");
    QTest::addColumn<MethodMap>("methodMap");

    MethodMap map;
    QTest::newRow("no-methods") << QString() << map;

    // one method without arguments
    QDBusIntrospection::Method method;
    method.name = "Foo";
    map << method;
    QTest::newRow("one-method") << "<method name=\"Foo\"/>" << map;

    // add another method without arguments
    method.name = "Bar";
    map << method;
    QTest::newRow("two-methods") << "<method name=\"Foo\"/>"
                                    "<method name=\"Bar\"/>"
                                 << map;

    // invert the order of the XML declaration
    QTest::newRow("two-methods-inverse") << "<method name=\"Bar\"/>"
                                            "<method name=\"Foo\"/>"
                                         << map;

    // add a third, with annotations
    method.name = "Baz";
    method.annotations.insert("foo.testing", "nothing to see here");
    map << method;
    QTest::newRow("method-with-annotation") <<
        "<method name=\"Foo\"/>"
        "<method name=\"Bar\"/>"
        "<method name=\"Baz\"><annotation name=\"foo.testing\" value=\"nothing to see here\" /></method>"
                                            << map;

    // arguments
    map.clear();
    method.annotations.clear();

    method.name = "Method";
    method.inputArgs << arg("s");
    map << method;
    QTest::newRow("one-in") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" direction=\"in\"/>"
        "</method>" << map;

    // two arguments
    method.inputArgs << arg("v");
    map.clear();
    map << method;
    QTest::newRow("two-in") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" direction=\"in\"/>"
        "<arg type=\"v\" direction=\"in\"/>"
        "</method>" << map;

    // one invalid arg
    method.inputArgs << arg("~", "invalid");
    map.clear();
    map << method;
    QTest::newRow("two-in-one-invalid") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" direction=\"in\"/>"
        "<arg type=\"v\" direction=\"in\"/>"
        "<arg type=\"~\" name=\"invalid\" direction=\"in\"/>"
        "</method>" << map;

    // one out argument
    method.inputArgs.clear();
    method.outputArgs << arg("s");
    map.clear();
    map << method;
    QTest::newRow("one-out") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" direction=\"out\"/>"
        "</method>" << map;

    // two in and one out
    method.inputArgs << arg("s") << arg("v");
    map.clear();
    map << method;
    QTest::newRow("two-in-one-out") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" direction=\"in\"/>"
        "<arg type=\"v\" direction=\"in\"/>"
        "<arg type=\"s\" direction=\"out\"/>"
        "</method>" << map;

    // let's try an arg with name
    method.outputArgs.clear();
    method.inputArgs.clear();
    method.inputArgs << arg("s", "foo");
    map.clear();
    map << method;
    QTest::newRow("one-in-with-name") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" name=\"foo\" direction=\"in\"/>"
        "</method>" << map;

    // two args with name
    method.inputArgs << arg("i", "bar");
    map.clear();
    map << method;
    QTest::newRow("two-in-with-name") <<
        "<method name=\"Method\">"
        "<arg type=\"s\" name=\"foo\" direction=\"in\"/>"
        "<arg type=\"i\" name=\"bar\" direction=\"in\"/>"
        "</method>" << map;

    // one complex
    map.clear();
    method = QDBusIntrospection::Method();

    // Method1(in STRING arg1, in BYTE arg2, out ARRAY of STRING)
    method.inputArgs << arg("s", "arg1") << arg("y", "arg2");
    method.outputArgs << arg("as");
    method.name = "Method1";
    map << method;

    // Method2(in ARRAY of DICT_ENTRY of (STRING,VARIANT) variantMap, in UINT32 index,
    //         out STRING key, out VARIANT value)
    // with annotation "foo.equivalent":"QVariantMap"
    method = QDBusIntrospection::Method();
    method.inputArgs << arg("a{sv}", "variantMap") << arg("u", "index");
    method.outputArgs << arg("s", "key") << arg("v", "value");
    method.annotations.insert("foo.equivalent", "QVariantMap");
    method.name = "Method2";
    map << method;

    QTest::newRow("complex") <<
        "<method name=\"Method1\">"
        "<arg name=\"arg1\" type=\"s\" direction=\"in\"/>"
        "<arg name=\"arg2\" type=\"y\" direction=\"in\"/>"
        "<arg type=\"as\" direction=\"out\"/>"
        "</method>"
        "<method name=\"Method2\">"
        "<arg name=\"variantMap\" type=\"a{sv}\" direction=\"in\"/>"
        "<arg name=\"index\" type=\"u\" direction=\"in\"/>"
        "<arg name=\"key\" type=\"s\" direction=\"out\"/>"
        "<arg name=\"value\" type=\"v\" direction=\"out\"/>"
        "<annotation name=\"foo.equivalent\" value=\"QVariantMap\"/>"
        "</method>" << map;
}

void tst_QDBusXmlParser::methods()
{
    QString intHeader = "<interface name=\"iface.iface1\">",
            intFooter = "</interface>",
            xmlHeader = "<node>" + intHeader,
            xmlFooter = intFooter + "</node>";

    QFETCH(QString, xmlDataFragment);

    QDBusIntrospection::Interface iface =
        QDBusIntrospection::parseInterface(xmlHeader + xmlDataFragment + xmlFooter);

    QCOMPARE(iface.name, QString("iface.iface1"));
    QCOMPARE(clean_xml(iface.introspection), clean_xml(intHeader + xmlDataFragment + intFooter));

    QFETCH(MethodMap, methodMap);
    MethodMap parsedMap = iface.methods;

    QCOMPARE(parsedMap.count(), methodMap.count());
    QCOMPARE(parsedMap, methodMap);
}

void tst_QDBusXmlParser::signals__data()
{
    QTest::addColumn<QString>("xmlDataFragment");
    QTest::addColumn<SignalMap>("signalMap");

    SignalMap map;
    QTest::newRow("no-signals") << QString() << map;

    // one signal without arguments
    QDBusIntrospection::Signal signal;
    signal.name = "Foo";
    map << signal;
    QTest::newRow("one-signal") << "<signal name=\"Foo\"/>" << map;

    // add another signal without arguments
    signal.name = "Bar";
    map << signal;
    QTest::newRow("two-signals") << "<signal name=\"Foo\"/>"
                                    "<signal name=\"Bar\"/>"
                                 << map;

    // invert the order of the XML declaration
    QTest::newRow("two-signals-inverse") << "<signal name=\"Bar\"/>"
                                            "<signal name=\"Foo\"/>"
                                         << map;

    // add a third, with annotations
    signal.name = "Baz";
    signal.annotations.insert("foo.testing", "nothing to see here");
    map << signal;
    QTest::newRow("signal-with-annotation") <<
        "<signal name=\"Foo\"/>"
        "<signal name=\"Bar\"/>"
        "<signal name=\"Baz\"><annotation name=\"foo.testing\" value=\"nothing to see here\" /></signal>"
                                            << map;

    // one out argument
    map.clear();
    signal.annotations.clear();
    signal.outputArgs << arg("s");
    signal.name = "Signal";
    map.clear();
    map << signal;
    QTest::newRow("one-out") <<
        "<signal name=\"Signal\">"
        "<arg type=\"s\" direction=\"out\"/>"
        "</signal>" << map;

    // without saying which direction it is
    QTest::newRow("one-out-no-direction") <<
        "<signal name=\"Signal\">"
        "<arg type=\"s\"/>"
        "</signal>" << map;

    // two args with name
    signal.outputArgs << arg("i", "bar");
    map.clear();
    map << signal;
    QTest::newRow("two-out-with-name") <<
        "<signal name=\"Signal\">"
        "<arg type=\"s\" direction=\"out\"/>"
        "<arg type=\"i\" name=\"bar\"/>"
        "</signal>" << map;

    // one complex
    map.clear();
    signal = QDBusIntrospection::Signal();

    // Signal1(out ARRAY of STRING)
    signal.outputArgs << arg("as");
    signal.name = "Signal1";
    map << signal;

    // Signal2(out STRING key, out VARIANT value)
    // with annotation "foo.equivalent":"QVariantMap"
    signal = QDBusIntrospection::Signal();
    signal.outputArgs << arg("s", "key") << arg("v", "value");
    signal.annotations.insert("foo.equivalent", "QVariantMap");
    signal.name = "Signal2";
    map << signal;

    QTest::newRow("complex") <<
        "<signal name=\"Signal1\">"
        "<arg type=\"as\" direction=\"out\"/>"
        "</signal>"
        "<signal name=\"Signal2\">"
        "<arg name=\"key\" type=\"s\" direction=\"out\"/>"
        "<arg name=\"value\" type=\"v\" direction=\"out\"/>"
        "<annotation name=\"foo.equivalent\" value=\"QVariantMap\"/>"
        "</signal>" << map;
}

void tst_QDBusXmlParser::signals_()
{
    QString intHeader = "<interface name=\"iface.iface1\">",
            intFooter = "</interface>",
            xmlHeader = "<node>" + intHeader,
            xmlFooter = intFooter + "</node>";

    QFETCH(QString, xmlDataFragment);

    QDBusIntrospection::Interface iface =
        QDBusIntrospection::parseInterface(xmlHeader + xmlDataFragment + xmlFooter);

    QCOMPARE(iface.name, QString("iface.iface1"));
    QCOMPARE(clean_xml(iface.introspection), clean_xml(intHeader + xmlDataFragment + intFooter));

    QFETCH(SignalMap, signalMap);
    SignalMap parsedMap = iface.signals_;

    QCOMPARE(signalMap.count(), parsedMap.count());
    QCOMPARE(signalMap, parsedMap);
}

void tst_QDBusXmlParser::properties_data()
{
    QTest::addColumn<QString>("xmlDataFragment");
    QTest::addColumn<PropertyMap>("propertyMap");

    PropertyMap map;
    QTest::newRow("no-signals") << QString() << map;

    // one readable signal
    QDBusIntrospection::Property prop;
    prop.name = "foo";
    prop.type = "s";
    prop.access = QDBusIntrospection::Property::Read;
    map << prop;
    QTest::newRow("one-readable") << "<property name=\"foo\" type=\"s\" access=\"read\"/>" << map;

    // one writable signal
    prop.access = QDBusIntrospection::Property::Write;
    map.clear();
    map << prop;
    QTest::newRow("one-writable") << "<property name=\"foo\" type=\"s\" access=\"write\"/>" << map;

    // one read- & writable signal
    prop.access = QDBusIntrospection::Property::ReadWrite;
    map.clear();
    map << prop;
    QTest::newRow("one-read-writable") << "<property name=\"foo\" type=\"s\" access=\"readwrite\"/>"
                                       << map;

    // two, mixed properties
    prop.name = "bar";
    prop.type = "i";
    prop.access = QDBusIntrospection::Property::Read;
    map << prop;
    QTest::newRow("two-1") <<
        "<property name=\"foo\" type=\"s\" access=\"readwrite\"/>"
        "<property name=\"bar\" type=\"i\" access=\"read\"/>" << map;

    // invert the order of the declaration
    QTest::newRow("two-2") <<
        "<property name=\"bar\" type=\"i\" access=\"read\"/>"
        "<property name=\"foo\" type=\"s\" access=\"readwrite\"/>" << map;

    // add a third with annotations
    prop.name = "baz";
    prop.type = "as";
    prop.access = QDBusIntrospection::Property::Write;
    prop.annotations.insert("foo.annotation", "Hello, World");
    prop.annotations.insert("foo.annotation2", "Goodbye, World");
    map << prop;
    QTest::newRow("complex") <<
        "<property name=\"bar\" type=\"i\" access=\"read\"/>"
        "<property name=\"baz\" type=\"as\" access=\"write\">"
        "<annotation name=\"foo.annotation\" value=\"Hello, World\" />"
        "<annotation name=\"foo.annotation2\" value=\"Goodbye, World\" />"
        "</property>"
        "<property name=\"foo\" type=\"s\" access=\"readwrite\"/>" << map;

    // and now change the order
    QTest::newRow("complex2") <<
        "<property name=\"baz\" type=\"as\" access=\"write\">"
        "<annotation name=\"foo.annotation2\" value=\"Goodbye, World\" />"
        "<annotation name=\"foo.annotation\" value=\"Hello, World\" />"
        "</property>"
        "<property name=\"bar\" type=\"i\" access=\"read\"/>"
        "<property name=\"foo\" type=\"s\" access=\"readwrite\"/>" << map;
}

void tst_QDBusXmlParser::properties()
{
    QString intHeader = "<interface name=\"iface.iface1\">",
            intFooter = "</interface>",
            xmlHeader = "<node>" + intHeader,
            xmlFooter = intFooter + "</node>";

    QFETCH(QString, xmlDataFragment);

    QDBusIntrospection::Interface iface =
        QDBusIntrospection::parseInterface(xmlHeader + xmlDataFragment + xmlFooter);

    QCOMPARE(iface.name, QString("iface.iface1"));
    QCOMPARE(clean_xml(iface.introspection), clean_xml(intHeader + xmlDataFragment + intFooter));

    QFETCH(PropertyMap, propertyMap);
    PropertyMap parsedMap = iface.properties;

    QCOMPARE(propertyMap.count(), parsedMap.count());
    QCOMPARE(propertyMap, parsedMap);
}

QTEST_MAIN(tst_QDBusXmlParser)

#include "tst_qdbusxmlparser.moc"
