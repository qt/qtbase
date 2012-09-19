/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation.
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

#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <dbus/dbus.h>

class tst_qdbusxml2cpp : public QObject
{
    Q_OBJECT

    enum { Interface, Adaptor };

private slots:
    void initTestCase_data();
    void process_data();
    void process();
};

struct BasicTypeList {
    char dbusType[3];
    char cppType[24];
};
static const BasicTypeList basicTypeList[] =
{
    { DBUS_TYPE_BOOLEAN_AS_STRING, "bool" },
    { DBUS_TYPE_BYTE_AS_STRING, "uchar" },
    { DBUS_TYPE_INT16_AS_STRING, "short" },
    { DBUS_TYPE_UINT16_AS_STRING, "ushort" },
    { DBUS_TYPE_INT32_AS_STRING, "int" },
    { DBUS_TYPE_UINT32_AS_STRING, "uint" },
    { DBUS_TYPE_INT64_AS_STRING, "qlonglong" },
    { DBUS_TYPE_UINT64_AS_STRING, "qulonglong" },
    { DBUS_TYPE_DOUBLE_AS_STRING, "double" },
    { DBUS_TYPE_STRING_AS_STRING, "QString" },
    { DBUS_TYPE_OBJECT_PATH_AS_STRING, "QDBusObjectPath" },
    { DBUS_TYPE_SIGNATURE_AS_STRING, "QDBusSignature" },
#ifdef DBUS_TYPE_UNIX_FD_AS_STRING
    { DBUS_TYPE_UNIX_FD_AS_STRING, "QDBusUnixFileDescriptor" },
#endif
    { DBUS_TYPE_VARIANT_AS_STRING, "QDBusVariant" },
    { DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_BYTE_AS_STRING, "QByteArray" },
    { DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING, "QStringList" },
    { DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_VARIANT_AS_STRING, "QVariantList" }
};
static const int basicTypeCount = sizeof(basicTypeList) / sizeof(basicTypeList[0]);

static QString stripHeader(QString output)
{
    static QRegularExpression header("^.*?(?=\\Rclass)", QRegularExpression::DotMatchesEverythingOption);
    return output.remove(header);
}

void tst_qdbusxml2cpp::initTestCase_data()
{
    QTest::addColumn<int>("outputMode");
    QTest::addColumn<QString>("commandLineArg");
    QTest::newRow("interface") << int(Interface) << "-p";
    QTest::newRow("adaptor") << int(Adaptor) << "-a";
}

void tst_qdbusxml2cpp::process_data()
{
    QTest::addColumn<QString>("xmlSnippet");
    QTest::addColumn<QRegularExpression>("interfaceSearch");
    QTest::addColumn<QRegularExpression>("adaptorSearch");

    // -- class info --
    QTest::newRow("classinfo")
            << ""
            << QRegularExpression("staticInterfaceName\\(\\)\\s+"
                                  "{ return \"local\\.name\\.is\\.not\\.important\"\\; }")
            << QRegularExpression("Q_CLASSINFO\\(\"D-Bus Interface\", \"local\\.name\\.is\\.not\\.important\"\\)");

    // -- properties --
    for (int i = 0; i < basicTypeCount; ++i) {
        QRegularExpression rx(QString("\\bQ_PROPERTY\\(%1 PropertyIsPresent "
                                                      "READ propertyIsPresent WRITE setPropertyIsPresent\\b")
                                              .arg(basicTypeList[i].cppType));
        QTest::newRow(QByteArray("property-") + basicTypeList[i].dbusType)
                << QString("<property type=\"%1\" name=\"PropertyIsPresent\" access=\"readwrite\" />")
                   .arg(basicTypeList[i].dbusType)
                << rx << rx;
    }

    QTest::newRow("property-readonly-multi")
            << "<property type=\"i\" name=\"Value\" access=\"read\"></property>"
            << QRegularExpression("\\bQ_PROPERTY\\(int Value READ value(?! WRITE)")
            << QRegularExpression("\\bQ_PROPERTY\\(int Value READ value(?! WRITE)");
    QTest::newRow("property-readonly")
            << "<property type=\"i\" name=\"Value\" access=\"read\" />"
            << QRegularExpression("\\bQ_PROPERTY\\(int Value READ value(?! WRITE)")
            << QRegularExpression("\\bQ_PROPERTY\\(int Value READ value(?! WRITE)");
    QTest::newRow("property-writeonly")
            << "<property type=\"i\" name=\"Value\" access=\"write\" />"
            << QRegularExpression("\\bQ_PROPERTY\\(int Value WRITE setValue\\b")
            << QRegularExpression("\\bQ_PROPERTY\\(int Value WRITE setValue\\b");

    QTest::newRow("property-getter-setter")
            << "<property type=\"b\" name=\"Enabled\" access=\"readwrite\">"
               "<annotation name=\"org.qtproject.QtDBus.PropertyGetter\" value=\"wasEnabled\" />"
               "<annotation name=\"org.qtproject.QtDBus.PropertySetter\" value=\"setEnabledFlag\" />"
               "</property>"
            << QRegularExpression("\\bQ_PROPERTY\\(bool Enabled READ wasEnabled WRITE setEnabledFlag\\b.*"
                                  "\\bbool wasEnabled\\(\\) const.*" // no semi-colon
                                  "\\bvoid setEnabledFlag\\(bool", QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("\\bQ_PROPERTY\\(bool Enabled READ wasEnabled WRITE setEnabledFlag\\b.*"
                                  "\\bbool wasEnabled\\(\\) const;.*" // has semi-colon
                                  "\\bvoid setEnabledFlag\\(bool", QRegularExpression::DotMatchesEverythingOption);

    QTest::newRow("property-complex")
            << "<property type=\"(ii)\" name=\"Position\" access=\"readwrite\">"
               "<annotation name=\"org.qtproject.QtDBus.QtTypeName\" value=\"Point\"/>"
               "</property>"
            << QRegularExpression("\\bQ_PROPERTY\\(Point Position READ position WRITE setPosition\\b")
            << QRegularExpression("\\bQ_PROPERTY\\(Point Position READ position WRITE setPosition\\b");

    // -- methods --
    for (int i = 0; i < basicTypeCount; ++i) {
        QTest::newRow(QByteArray("method-") + basicTypeList[i].dbusType)
                << QString("<method name=\"Method\">"
                           "<arg type=\"%1\" direction=\"out\"/>"
                           "<arg type=\"%1\" direction=\"in\"/>"
                           "</method>")
                   .arg(basicTypeList[i].dbusType)
                << QRegularExpression(QString("Q_SLOTS:.*\\bQDBusPendingReply<%1> Method\\((const )?%1 ")
                                      .arg(basicTypeList[i].cppType), QRegularExpression::DotMatchesEverythingOption)
                << QRegularExpression(QString("Q_SLOTS:.*\\b%1 Method\\((const )?%1 ")
                                      .arg(basicTypeList[i].cppType), QRegularExpression::DotMatchesEverythingOption);
    }

    QTest::newRow("method-complex")
            << "<method name=\"Method\">"
               "<arg type=\"(dd)\" direction=\"in\"/>"
               "<arg type=\"(ii)\" direction=\"out\"/>"
               "<annotation name=\"org.qtproject.QtDBus.QtTypeName.Out0\" value=\"Point\"/>"
               "<annotation name=\"org.qtproject.QtDBus.QtTypeName.In0\" value=\"PointF\"/>"
               "</method>"
            << QRegularExpression("Q_SLOTS:.*\\bQDBusPendingReply<Point> Method\\(PointF ",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*\\bPoint Method\\(PointF ",
                                  QRegularExpression::DotMatchesEverythingOption);

    QTest::newRow("method-ss")
            << "<method name=\"Method\">"
               "<arg type=\"s\" direction=\"in\"/>"
               "<arg type=\"s\" direction=\"in\"/>"
               "<arg type=\"s\" direction=\"out\"/>"
               "<arg type=\"s\" direction=\"out\"/>"
               "</method>"
            << QRegularExpression("Q_SLOTS:.*QDBusPendingReply<QString, QString> Method\\(const QString &\\w*, const QString &",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*QString Method\\(const QString &\\w*, const QString &\\w*, QString &",
                                  QRegularExpression::DotMatchesEverythingOption);

    // -- signals --
    for (int i = 0; i < basicTypeCount; ++i) {
        QRegularExpression rx(QString("Q_SIGNALS:.*\\bvoid Signal\\((const )?%1\\b")
                              .arg(basicTypeList[i].cppType),
                              QRegularExpression::DotMatchesEverythingOption);
        QTest::newRow(QByteArray("signal-") + basicTypeList[i].dbusType)
                << QString("<signal name=\"Signal\">"
                           "<arg type=\"%1\"/>"
                           "</signal>")
                   .arg(basicTypeList[i].dbusType)
                << rx << rx;
    }
}

void tst_qdbusxml2cpp::process()
{
    QFETCH(QString, xmlSnippet);
    QFETCH(QRegularExpression, interfaceSearch);
    QFETCH(QRegularExpression, adaptorSearch);
    QVERIFY2(interfaceSearch.isValid(), qPrintable(interfaceSearch.errorString()));
    QVERIFY2(adaptorSearch.isValid(), qPrintable(adaptorSearch.errorString()));

    // test both interface and adaptor generation
    QFETCH_GLOBAL(int, outputMode);
    QFETCH_GLOBAL(QString, commandLineArg);

    // Run the tool
    QProcess process;
    process.start("qdbusxml2cpp", QStringList() << commandLineArg << "-" << "-N");
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));

    // feed it our XML data
    static const char xmlHeader[] =
            "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
            DBUS_INTROSPECT_1_0_XML_DOCTYPE_DECL_NODE // \n is included
            "<node>\n"
            "  <interface name=\"local.name.is.not.important\">\n"
            "    <!-- begin data -->\n";
    static const char xmlFooter[] = "\n"
            "    <!-- end data -->\n"
            "  </interface>\n"
            "</node>\n";

    process.write(xmlHeader, int(sizeof xmlHeader) - 1);
    process.write(xmlSnippet.toLatin1());
    process.write(xmlFooter, int(sizeof xmlFooter) - 1);
    while (process.bytesToWrite())
        QVERIFY2(process.waitForBytesWritten(), qPrintable(process.errorString()));
    //    fprintf(stderr, "%s%s%s", xmlHeader, xmlSnippet.toLatin1().constData(), xmlFooter);

    process.closeWriteChannel();
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));

    QByteArray errOutput = process.readAllStandardError();
    QVERIFY2(errOutput.isEmpty(), errOutput);
    QCOMPARE(process.exitCode(), 0);

    QByteArray fullOutput = process.readAll();
    QString output = stripHeader(QString::fromLatin1(fullOutput));
    QVERIFY2(!output.isEmpty(), fullOutput);
    if (outputMode == Interface)
        QVERIFY2(output.count(interfaceSearch) == 1, qPrintable(interfaceSearch.pattern() + "\nin\n" + output));
    else
        QVERIFY2(output.count(adaptorSearch) == 1, qPrintable(adaptorSearch.pattern() + "\nin\n" + output));
}

QTEST_MAIN(tst_qdbusxml2cpp)

#include "tst_qdbusxml2cpp.moc"
