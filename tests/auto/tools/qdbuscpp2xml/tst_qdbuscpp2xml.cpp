/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Stephen Kelly <stephen.kelly@kdab.com>
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

#include "test1.h"

#include <QtDBus/QDBusConnection>

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

    QBitArray doneFlags(QDBusConnection::ExportAllContents + 1);
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
    const QString binpath = QLibraryInfo::location(QLibraryInfo::BinariesPath);
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

QTEST_APPLESS_MAIN(tst_qdbuscpp2xml)

#include "tst_qdbuscpp2xml.moc"
