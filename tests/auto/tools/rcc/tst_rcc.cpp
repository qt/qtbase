/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QCoreApplication>
#include <QtCore/QByteArray>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTimer>

#include <QtTest/QtTest>


class tst_rcc : public QObject
{
    Q_OBJECT
public:
    tst_rcc() {}

private slots:
    void rcc_data();
    void rcc();
};


QString findExpectedFile(const QString &base)
{
    QString expectedrccfile = base;

    // Must be updated with each minor release.
    if (QFileInfo(expectedrccfile + QLatin1String(".450")).exists())
        expectedrccfile += QLatin1String(".450");

    return expectedrccfile;
}

static QString doCompare(const QStringList &actual, const QStringList &expected)
{
    if (actual.size() != expected.size()) {
        return QString("Length count different: actual: %1, expected: %2")
            .arg(actual.size()).arg(expected.size());
    }

    QByteArray ba;
    for (int i = 0, n = expected.size(); i != n; ++i) {
        if (expected.at(i).startsWith("IGNORE:"))
            continue;
        if (expected.at(i) != actual.at(i)) {
            qDebug() << "LINES" << i << "DIFFER";
            ba.append(
             "\n<<<<<< actual\n" + actual.at(i) + "\n======\n" + expected.at(i)
                + "\n>>>>>> expected\n"
            );
        }
    }
    return ba;
}


void tst_rcc::rcc_data()
{
    QTest::addColumn<QString>("directory");
    QTest::addColumn<QString>("qrcfile");
    QTest::addColumn<QString>("expected");

    QTest::newRow("images") << SRCDIR "data" << "images.qrc" << "images.expected";
}

void tst_rcc::rcc()
{
    QFETCH(QString, directory);
    QFETCH(QString, qrcfile);
    QFETCH(QString, expected);

    if (!QDir::setCurrent(directory)) {
        QString message = QString::fromLatin1("Unable to cd from '%1' to '%2'").arg(QDir::currentPath(), directory);
        QFAIL(qPrintable(message));
    }

    // If the file expectedoutput.txt exists, compare the
    // console output with the content of that file
    const QString expected2 = findExpectedFile(expected);
    QFile expectedFile(expected2);
    if (!expectedFile.exists()) {
        qDebug() << "NO EXPECTATIONS? " << expected2;
        return;
    }

    // Launch
    const QString command = QLatin1String("rcc");
    QProcess process;
    process.start(command, QStringList(qrcfile));
    if (!process.waitForFinished()) {
        const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
        QString message = QString::fromLatin1("'%1' could not be found when run from '%2'. Path: '%3' ").
                          arg(command, QDir::currentPath(), path);
        QFAIL(qPrintable(message));
    }
    const QChar cr = QLatin1Char('\r');
    const QString err = QString::fromLocal8Bit(process.readAllStandardError()).remove(cr);
    const QString out = QString::fromAscii(process.readAllStandardOutput()).remove(cr);

    if (!err.isEmpty()) {
        qDebug() << "UNEXPECTED STDERR CONTENTS: " << err;
        QFAIL("UNEXPECTED STDERR CONTENTS");
    }

    const QChar nl = QLatin1Char('\n');
    const QStringList actualLines = out.split(nl);

    QVERIFY(expectedFile.open(QIODevice::ReadOnly|QIODevice::Text));
    const QStringList expectedLines = QString::fromAscii(expectedFile.readAll()).split(nl);

    const QString diff = doCompare(actualLines, expectedLines);
    if (diff.size())
        QFAIL(qPrintable(diff));
}



QTEST_APPLESS_MAIN(tst_rcc)

#include "tst_rcc.moc"
