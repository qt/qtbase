// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QLibraryInfo>
#include <QProcess>

#include <QtDBus/QDBusConnection>
#include <QtDBus/private/dbus_minimal_p.h>

#include "test1.h"


// in qdbusxmlgenerator.cpp
QT_BEGIN_NAMESPACE
extern Q_DBUS_EXPORT QString qDBusGenerateMetaObjectXml(QString interface,
                                                        const QMetaObject *mo,
                                                        const QMetaObject *base,
                                                        int flags);
QT_END_NAMESPACE

static QString addXmlHeader(const QString &input)
{
    return
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\" \"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n<node>"
      + (input.isEmpty() ? QString() : QString("\n  " + input.trimmed()))
    + "\n</node>\n";
}

class tst_qdbuscpp2xml : public QObject
{
    Q_OBJECT

private slots:
    void qdbuscpp2xml_data();
    void qdbuscpp2xml();
    void qtdbusTypes_data();
    void qtdbusTypes();

    void initTestCase();
    void cleanupTestCase();

private:
    QHash<QString, QObject*> m_tests;
};

void tst_qdbuscpp2xml::initTestCase()
{
    m_tests.insert("test1", new Test1);
}

void tst_qdbuscpp2xml::cleanupTestCase()
{
    qDeleteAll(m_tests);
}

void tst_qdbuscpp2xml::qdbuscpp2xml_data()
{
    QTest::addColumn<QString>("inputfile");
    QTest::addColumn<int>("flags");

    QBitArray doneFlags(int(QDBusConnection::ExportAllContents) + 1);
    for (int flag = 0x10; flag < QDBusConnection::ExportScriptableContents; flag += 0x10) {
        QTest::newRow("xmlgenerator-" + QByteArray::number(flag)) << "test1" << flag;
        doneFlags.setBit(flag);
        for (int mask = QDBusConnection::ExportAllSlots; mask <= QDBusConnection::ExportAllContents; mask += 0x110) {
            int flags = flag | mask;
            if (doneFlags.testBit(flags))
                continue;
            QTest::newRow("xmlgenerator-" + QByteArray::number(flags)) << "test1" << flags;
            doneFlags.setBit(flags);
        }
    }
}

void tst_qdbuscpp2xml::qdbuscpp2xml()
{
    QFETCH(QString, inputfile);
    QFETCH(int, flags);

    // qdbuscpp2xml considers these equivalent
    if (flags & QDBusConnection::ExportScriptableSlots)
        flags |= QDBusConnection::ExportScriptableInvokables;
    if (flags & QDBusConnection::ExportNonScriptableSlots)
        flags |= QDBusConnection::ExportNonScriptableInvokables;

    if (flags & QDBusConnection::ExportScriptableInvokables)
        flags |= QDBusConnection::ExportScriptableSlots;
    if (flags & QDBusConnection::ExportNonScriptableInvokables)
        flags |= QDBusConnection::ExportNonScriptableSlots;

    QStringList options;
    if (flags & QDBusConnection::ExportScriptableProperties) {
        if (flags & QDBusConnection::ExportNonScriptableProperties)
            options << "-P";
        else
            options << "-p";
    }
    if (flags & QDBusConnection::ExportScriptableSignals) {
        if (flags & QDBusConnection::ExportNonScriptableSignals)
            options << "-S";
        else
            options << "-s";
    }
    if (flags & QDBusConnection::ExportScriptableSlots) {
        if (flags & QDBusConnection::ExportNonScriptableSlots)
            options << "-M";
        else
            options << "-m";
    }

    // Launch
    const QString binpath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    const QString command = binpath + QLatin1String("/qdbuscpp2xml");
    QProcess process;
    process.start(command, QStringList() << options << (QFINDTESTDATA(inputfile + QStringLiteral(".h"))));
    if (!process.waitForFinished()) {
        const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
        QString message = QString::fromLatin1("'%1' could not be found when run from '%2'. Path: '%3' ").
                          arg(command, QDir::currentPath(), path);
        QFAIL(qPrintable(message));
    }
    const QChar cr = QLatin1Char('\r');
    const QString err = QString::fromLocal8Bit(process.readAllStandardError()).remove(cr);
    const QString out = QString::fromLatin1(process.readAllStandardOutput()).remove(cr);

    if (!err.isEmpty()) {
        qDebug() << "UNEXPECTED STDERR CONTENTS: " << err;
        QFAIL("UNEXPECTED STDERR CONTENTS");
    }

    const QChar nl = QLatin1Char('\n');
    const QStringList actualLines = out.split(nl);

    QObject *testObject = m_tests.value(inputfile);

    if (flags == 0)
        flags = QDBusConnection::ExportScriptableContents
              | QDBusConnection::ExportNonScriptableContents;

    QString expected = qDBusGenerateMetaObjectXml(QString(), testObject->metaObject(), &QObject::staticMetaObject, flags);

    expected = addXmlHeader(expected);

    QCOMPARE(out, expected);
}

void tst_qdbuscpp2xml::qtdbusTypes_data()
{
    QTest::addColumn<QByteArray>("type");
    QTest::addColumn<QByteArray>("expectedSignature");
    auto addRow = [](QByteArray type, QByteArray signature) {
        QTest::addRow("%s", type.constData()) << type << signature;

        // lists and vectors
        QTest::addRow("QList-%s", type.constData())
                << "QList<" + type + '>' << DBUS_TYPE_ARRAY + signature;
        QTest::addRow("QVector-%s", type.constData())
                << "QVector<" + type + '>' << DBUS_TYPE_ARRAY + signature;
    };
    addRow("QDBusVariant", DBUS_TYPE_VARIANT_AS_STRING);
    addRow("QDBusObjectPath", DBUS_TYPE_OBJECT_PATH_AS_STRING);
    addRow("QDBusSignature", DBUS_TYPE_SIGNATURE_AS_STRING);
    addRow("QDBusUnixFileDescriptor", DBUS_TYPE_UNIX_FD_AS_STRING);

    // QDBusMessage is not a type, but must be recognized
    QTest::newRow("QDBusMessage") << QByteArray("QDBusMessage") << QByteArray();
}

void tst_qdbuscpp2xml::qtdbusTypes()
{
    static const char cppSkeleton[] = R"(
class QDBusVariantBugRepro : public QDBusAbstractAdaptor
{
   Q_OBJECT
   Q_CLASSINFO("D-Bus Interface", "org.qtproject.test")
public Q_SLOTS:
   void method(const @TYPE@ &);
};)";
    static const char methodXml[] = R"(<method name="method")";
    static const char expectedSkeleton[] = R"(<arg type="@S@" direction="in"/>)";

    QFETCH(QByteArray, type);
    QFETCH(QByteArray, expectedSignature);

    const QString binpath = QLibraryInfo::path(QLibraryInfo::BinariesPath);
    const QString command = binpath + QLatin1String("/qdbuscpp2xml");
    QProcess process;
    process.start(command);

    process.write(QByteArray(cppSkeleton).replace("@TYPE@", type));
    process.closeWriteChannel();

    if (!process.waitForStarted()) {
        const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
        QString message = QString::fromLatin1("'%1' could not be found when run from '%2'. Path: '%3' ").
                          arg(command, QDir::currentPath(), path);
        QFAIL(qPrintable(message));
    }
    QVERIFY2(process.waitForFinished(), qPrintable(process.errorString()));

    // verify nothing was printed on stderr
    QCOMPARE(process.readAllStandardError(), QString());

    // we don't do a full XML parsing here...
    QByteArray output = process.readAll().simplified();
    QVERIFY2(output.contains("<node>") && output.contains("</node>"), "Output was: " + output);
    output = output.mid(output.indexOf("<node>") + strlen("<node>"));
    output = output.left(output.indexOf("</node>"));

    QVERIFY2(output.contains(methodXml), "Output was: " + output);

    if (!expectedSignature.isEmpty()) {
        QByteArray expected = QByteArray(expectedSkeleton).replace("@S@", expectedSignature);
        QVERIFY2(output.contains(expected), "Expected: '" + expected + "'; got: '" + output + '\'');
    }
}

QTEST_APPLESS_MAIN(tst_qdbuscpp2xml)

#include "tst_qdbuscpp2xml.moc"
