// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QLibraryInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>

// We just need the DBUS_TYPE_* constants, so use our own copy
#include "../../../../src/dbus/dbus_minimal_p.h"

class tst_qdbusxml2cpp : public QObject
{
    Q_OBJECT

    enum { Interface, Adaptor };

private slots:
    void initTestCase_data();
    void process_data();
    void process();
    void includeStyle_data();
    void includeStyle();
    void missingAnnotation_data();
    void missingAnnotation();
    void includeMoc_data();
    void includeMoc();
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

static void runTool(QProcess &process, const QByteArray &data,
                    const QStringList &flags)
{
    // test both interface and adaptor generation
    QFETCH_GLOBAL(QString, commandLineArg);

    // Run the tool
    const QString binpath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    QStringList arguments = { commandLineArg };
    arguments += flags;
    process.setArguments(arguments);
    process.setProgram(binpath + QLatin1String("/qdbusxml2cpp"));
    process.start(QIODevice::Text | QIODevice::ReadWrite);
    QVERIFY2(process.waitForStarted(), qPrintable(process.errorString()));

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

    process.write(xmlHeader, sizeof(xmlHeader) - 1);
    process.write(data);
    process.write(xmlFooter, sizeof(xmlFooter) - 1);

    while (process.bytesToWrite())
        QVERIFY2(process.waitForBytesWritten(), qPrintable(process.errorString()));
    //    fprintf(stderr, "%s%s%s", xmlHeader, xmlSnippet.toLatin1().constData(), xmlFooter);

    process.closeWriteChannel();
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
}

static void checkOneFile(const QString &fileName, const QByteArray &expected)
{
    QFile file(fileName);
    QVERIFY(file.exists());
    const auto guard = QScopeGuard([&](){ QFile::remove(fileName); });

    QVERIFY(file.open(QFile::Text | QFile::ReadOnly));
    QByteArray text = file.readAll();
    QVERIFY(text.contains(expected));
}

static void checkTwoFiles(const QString &headerName, const QString &sourceName, const QByteArray &expected)
{
    QFile headerFile(headerName);
    QFile sourceFile(sourceName);

    QVERIFY(headerFile.exists());
    const auto headerGuard = QScopeGuard([&](){ QFile::remove(headerName); });

    QVERIFY(sourceFile.exists());
    const auto sourceGuard = QScopeGuard([&](){ QFile::remove(sourceName); });

    QVERIFY(sourceFile.open(QFile::Text | QFile::ReadOnly));
    QByteArray text = sourceFile.readAll();
    QVERIFY(text.contains(expected));
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

    QTest::newRow("method-name")
        << "<method name=\"Method\">"
            "<arg type=\"s\" direction=\"in\"/>"
            "<annotation name=\"org.qtproject.QtDBus.MethodName\" value=\"MethodRenamed\" />"
            "</method>"
        << QRegularExpression("Q_SLOTS:.*QDBusPendingReply<> MethodRenamed\\(const QString &\\w*",
                                QRegularExpression::DotMatchesEverythingOption)
        << QRegularExpression("Q_SLOTS:.*void MethodRenamed\\(const QString &\\w*",
                                QRegularExpression::DotMatchesEverythingOption);

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
            << QRegularExpression("Q_SLOTS:.*QDBusPendingReply<QString, QString> Method\\(const QString &\\w*, const QString &\\w*\\)"
                                  ".*inline QDBusReply<QString> Method\\(const QString &\\w*, const QString &\\w*, QString &\\w*\\)",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*QString Method\\(const QString &\\w*, const QString &\\w*, QString &",
                                  QRegularExpression::DotMatchesEverythingOption);

    QTest::newRow("method-deprecated-0out")
            << "<method name=\"Method\">"
               "<annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>"
               "</method>"
            << QRegularExpression("Q_SLOTS:.*Q_DECL_DEPRECATED inline QDBusPendingReply<> Method\\(\\)",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*\n\\s*void Method\\(\\)",  // no Q_DECL_DEPRECATED
                                  QRegularExpression::DotMatchesEverythingOption);

    QTest::newRow("method-deprecated-2out")
            << "<method name=\"Method\">"
               "<annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>"
               "<arg type=\"s\" direction=\"out\"/>"
               "<arg type=\"s\" direction=\"out\"/>"
               "</method>"
            << QRegularExpression("Q_SLOTS:.*Q_DECL_DEPRECATED inline QDBusPendingReply<QString, QString> Method\\(\\)"
                                  ".*Q_DECL_DEPRECATED inline QDBusReply<QString> Method\\(QString &\\w*\\)",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*\n\\s*QString Method\\(QString &", // no Q_DECL_DEPRECATED
                                  QRegularExpression::DotMatchesEverythingOption);

    QTest::newRow("method-noreply")
            << "<method name=\"Method\">"
               "<annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>"
               "</method>"
            << QRegularExpression("Q_SLOTS:.*Q_NOREPLY inline void Method\\(\\).*\\bQDBus::NoBlock\\b",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*Q_NOREPLY void Method\\(",
                                  QRegularExpression::DotMatchesEverythingOption);

    QTest::newRow("method-deprecated-noreply")
            << "<method name=\"Method\">"
               "<annotation name=\"org.freedesktop.DBus.Method.NoReply\" value=\"true\"/>"
               "<annotation name=\"org.freedesktop.DBus.Deprecated\" value=\"true\"/>"
               "</method>"
            << QRegularExpression("Q_SLOTS:.*Q_DECL_DEPRECATED Q_NOREPLY inline void Method\\(\\).*\\bQDBus::NoBlock\\b",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression("Q_SLOTS:.*Q_NOREPLY void Method\\(",
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

    QRegularExpression rx(R"(Q_SIGNALS:.*\b\Qvoid Signal(const QVariantMap &map);\E)",
                          QRegularExpression::DotMatchesEverythingOption);
    QTest::newRow("signal-complex")
            << R"(<signal name="Signal">
                    <arg type="a{sv}" name="map"/>
                    <annotation name="org.qtproject.QtDBus.QtTypeName.Out0" value="QVariantMap"/>"
                  </signal>)"
            << rx << rx;

    QTest::newRow("signal-deprecated")
            << R"(<signal name="Signal">
                    <annotation name="org.freedesktop.DBus.Deprecated" value="true"/>
                  </signal>)"
            << QRegularExpression(R"(Q_SIGNALS:.*\bQ_DECL_DEPRECATED void Signal\(\))",
                                  QRegularExpression::DotMatchesEverythingOption)
            << QRegularExpression(R"(Q_SIGNALS:.*\n\s*void Signal\(\))",    // no Q_DECL_DEPRECATED
                                  QRegularExpression::DotMatchesEverythingOption);
}

void tst_qdbusxml2cpp::process()
{
    QFETCH(QString, xmlSnippet);
    QFETCH(QRegularExpression, interfaceSearch);
    QFETCH(QRegularExpression, adaptorSearch);
    QVERIFY2(interfaceSearch.isValid(), qPrintable(interfaceSearch.errorString()));
    QVERIFY2(adaptorSearch.isValid(), qPrintable(adaptorSearch.errorString()));

    QFETCH_GLOBAL(int, outputMode);
    QProcess process;
    QStringList flags = {"-", "-N"};
    runTool(process, xmlSnippet.toLatin1(), flags);
    if (QTest::currentTestFailed()) return;

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

void tst_qdbusxml2cpp::includeStyle_data()
{
    QTest::addColumn<bool>("isGlobal");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("localInclude") <<  false << QByteArray("#include \"test.hpp\"");
    QTest::newRow("globalInclude") <<  true << QByteArray("#include <test.hpp>");
}

void tst_qdbusxml2cpp::includeStyle()
{
    QFETCH(bool, isGlobal);
    QFETCH(QByteArray, expected);

    QProcess process;
    QStringList flags = {"-", "-N", (isGlobal ? "-I" : "-i"), "test.hpp"};

    runTool(process,QByteArray{},flags);
    QCOMPARE(process.exitCode(), 0);

    QByteArray errOutput = process.readAllStandardError();
    QVERIFY2(errOutput.isEmpty(), errOutput);

    QByteArray fullOutput = process.readAll();
    QVERIFY(!fullOutput.isEmpty());
    QVERIFY(fullOutput.contains(expected));
}

void tst_qdbusxml2cpp::missingAnnotation_data()
{
    QTest::addColumn<QString>("xmlSnippet");
    QTest::addColumn<QString>("annotationName");

    QTest::newRow("property")
            << R"(<property type="%1" name="name" access="readwrite"/>)"
            << "org.qtproject.QtDBus.QtTypeName";
    QTest::newRow("method-in")
            << R"(<method name="Method">
                    <arg type="%1" name="name" direction="in"/>
                  </method>)"
            << "org.qtproject.QtDBus.QtTypeName.In0";
    QTest::newRow("method-out")
            << R"(<method name="Method">
                    <arg type="%1" name="name" direction="out"/>
                  </method>)"
            << "org.qtproject.QtDBus.QtTypeName.Out0";
    QTest::newRow("signal")
            << R"(<signal name="Signal">
                    <arg type="%1" name="name"/>
                  </signal>)"
            << "org.qtproject.QtDBus.QtTypeName.Out0";
    QTest::newRow("signal-out")
            << R"(<signal name="Signal">
                    <arg type="%1" name="name" direction="out"/>
                  </signal>)"
            << "org.qtproject.QtDBus.QtTypeName.Out0";
}

void tst_qdbusxml2cpp::missingAnnotation()
{
    QFETCH(QString, xmlSnippet);
    QFETCH(QString, annotationName);

    QString type = "(ii)";
    QProcess process;
    QStringList flags = {"-", "-N"};
    runTool(process, xmlSnippet.arg(type).toLatin1(),flags);
    if (QTest::currentTestFailed()) return;

    // it must have failed
    QString errOutput = QString::fromLatin1(process.readAllStandardError().trimmed());
    QCOMPARE(process.exitCode(), 1);
    QCOMPARE(process.readAllStandardOutput(), QByteArray());
    QVERIFY(!errOutput.isEmpty());

    // check it did suggest the right annotation
    QString expected = R"(qdbusxml2cpp: Got unknown type `%1' processing ''
You should add <annotation name="%2" value="<type>"/> to the XML description for 'name')";
    expected = expected.arg(type, annotationName);
    QCOMPARE(errOutput, expected);
}

void tst_qdbusxml2cpp::includeMoc_data()
{
    QTest::addColumn<QString>("filenames");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<QByteArray>("warning");

    QTest::newRow("combined-h") << "foo.h" << QByteArray("#include \"foo.moc\"") << QByteArray("");
    QTest::newRow("combined-cpp") << "foo.cpp" << QByteArray("#include \"foo.moc\"") << QByteArray("");
    QTest::newRow("combined-cc") << "foo.cc" << QByteArray("#include \"foo.moc\"") << QByteArray("");
    QTest::newRow("without extension") << "foo" << QByteArray("#include \"moc_foo.cpp\"") << QByteArray("");
    QTest::newRow("cpp-only") << ":foo.cpp" << QByteArray("#include \"moc_foo.cpp\"")
                              << QByteArray("warning: no header name is provided, assuming it to be \"foo.h\"");
    QTest::newRow("header-and-cpp") << "foo_h.h:foo.cpp" << QByteArray("#include \"moc_foo_h.cpp\"") << QByteArray("");

    QTest::newRow("combined-cpp with dots") << "foo.bar.cpp" << QByteArray("#include \"foo.bar.moc\"") << QByteArray("");
    QTest::newRow("without extension with dots") << "foo.bar" << QByteArray("#include \"moc_foo.bar.cpp\"") << QByteArray("");
    QTest::newRow("header-and-cpp with dots") << "foo.bar_h.h:foo.bar.cpp" << QByteArray("#include \"moc_foo.bar_h.cpp\"") << QByteArray("");
}

void tst_qdbusxml2cpp::includeMoc()
{
    QFETCH(QString, filenames);
    QFETCH(QByteArray, expected);
    QFETCH(QByteArray, warning);

    QProcess process;
    QStringList flags = {filenames, "--moc"};
    runTool(process,QByteArray{},flags);
    QByteArray errOutput = process.readAllStandardError();
    QVERIFY(errOutput.startsWith(warning));
    QCOMPARE(process.exitCode(), 0);

    QStringList parts = filenames.split(u':');
    QFileInfo first{parts.first()};

    const bool firstHasSuffix = QStringList({"h", "cpp", "cc"}).contains(first.suffix());

    if ((parts.size() == 1) && firstHasSuffix) {
        checkOneFile(parts.first(), expected);
    } else if ((parts.size() == 1) && (!firstHasSuffix)) {
        QString headerName{parts.first()};
        headerName += ".h";
        QString sourceName{parts.first()};
        sourceName += ".cpp";

        checkTwoFiles(headerName, sourceName, expected);
    } else if ((parts.size() == 2) && (parts.first().isEmpty())) {
        checkOneFile(parts.last(), expected);
    }
    else if ((parts.size() == 2) && !parts.first().isEmpty() && !parts.last().isEmpty()) {
        checkTwoFiles(parts.first(), parts.last(), expected);
    }
}

QTEST_MAIN(tst_qdbusxml2cpp)

#include "tst_qdbusxml2cpp.moc"
