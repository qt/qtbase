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


#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QByteArray>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTemporaryDir>

class tst_uic : public QObject
{
    Q_OBJECT

public:
    tst_uic();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void stdOut();

    void run();
    void run_data() const;

    void runTranslation();

    void compare();
    void compare_data() const;

    void runCompare();

private:
    const QString m_command;
    QString m_baseline;
    QTemporaryDir m_generated;
};

tst_uic::tst_uic()
    : m_command(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/uic"))
{
}

static QByteArray msgProcessStartFailed(const QString &command, const QString &why)
{
    const QString result = QString::fromLatin1("Could not start %1: %2")
            .arg(command, why);
    return result.toLocal8Bit();
}

void tst_uic::initTestCase()
{
    m_baseline = QFINDTESTDATA("baseline");
    QVERIFY2(!m_baseline.isEmpty(), "Could not find 'baseline'.");
    QProcess process;
    process.start(m_command, QStringList(QLatin1String("-help")));
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    // Print version
    const QString out = QString::fromLocal8Bit(process.readAllStandardError()).remove(QLatin1Char('\r'));
    const QStringList outLines = out.split(QLatin1Char('\n'));
    // Print version
    QString msg = QString::fromLatin1("uic test built %1 running in '%2' using: ").
                  arg(QString::fromLatin1(__DATE__), QDir::currentPath());
    if (!outLines.empty())
        msg += outLines.front();
    qDebug("%s", qPrintable(msg));
}

void tst_uic::cleanupTestCase()
{
    static const char envVar[] = "UIC_KEEP_GENERATED_FILES";
    if (qgetenv(envVar).isEmpty()) {
        qDebug("Note: The environment variable '%s' can be set to keep the temporary files for error analysis.", envVar);
    } else {
        m_generated.setAutoRemove(false);
        qDebug("Keeping generated files in '%s'", qPrintable(QDir::toNativeSeparators(m_generated.path())));
    }
}

void tst_uic::stdOut()
{
    // Checks of everything works when using stdout and whether
    // the OS file format conventions regarding newlines are met.
    QDir baseline(m_baseline);
    const QFileInfoList baselineFiles = baseline.entryInfoList(QStringList(QLatin1String("*.ui")), QDir::Files);
    QVERIFY(!baselineFiles.isEmpty());
    QProcess process;
    process.start(m_command, QStringList(baselineFiles.front().absoluteFilePath()));
    process.closeWriteChannel();
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    const QByteArray output = process.readAllStandardOutput();
    QByteArray expected = "/********************************************************************************";
#ifdef Q_OS_WIN
    expected += "\r\n";
#else
    expected += '\n';
#endif
    expected += "** ";
    QVERIFY2(output.startsWith(expected), (QByteArray("Got: ") + output.toHex()).constData());
}

void tst_uic::run()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);

    QProcess process;
    process.start(m_command, QStringList(originalFile)
        << QString(QLatin1String("-o")) << generatedFile);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(QFileInfo(generatedFile).exists(), true);
}

void tst_uic::run_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    QDir generated(m_generated.path());
    QDir baseline(m_baseline);
    const QFileInfoList baselineFiles = baseline.entryInfoList(QStringList("*.ui"), QDir::Files);
    foreach (const QFileInfo &baselineFile, baselineFiles) {
        const QString generatedFile = generated.absolutePath()
            + QLatin1Char('/') + baselineFile.fileName()
            + QLatin1String(".h");
        QTest::newRow(qPrintable(baselineFile.baseName()))
            << baselineFile.absoluteFilePath()
            << generatedFile;
    }
}


void tst_uic::compare()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);

    QFile orgFile(originalFile);
    QFile genFile(generatedFile);

    if (!orgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err(QLatin1String("Could not read file: %1..."));
        QFAIL(err.arg(orgFile.fileName()).toUtf8());
    }

    if (!genFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err(QLatin1String("Could not read file: %1..."));
        QFAIL(err.arg(genFile.fileName()).toUtf8());
    }

    originalFile = orgFile.readAll();
    originalFile.replace(QRegExp(QLatin1String("Created by: Qt User Interface Compiler version [.\\d]{5,5}")), "");

    generatedFile = genFile.readAll();
    generatedFile.replace(QRegExp(QLatin1String("Created by: Qt User Interface Compiler version [.\\d]{5,5}")), "");

    QCOMPARE(generatedFile, originalFile);
}

void tst_uic::compare_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    QDir generated(m_generated.path());
    QDir baseline(m_baseline);
    const QFileInfoList baselineFiles = baseline.entryInfoList(QStringList("*.h"), QDir::Files);
    foreach (const QFileInfo &baselineFile, baselineFiles) {
        const QString generatedFile = generated.absolutePath()
                + QLatin1Char('/') + baselineFile.fileName();
        QTest::newRow(qPrintable(baselineFile.baseName()))
            << baselineFile.absoluteFilePath()
            << generatedFile;
    }
}

void tst_uic::runTranslation()
{
    QProcess process;

    QDir baseline(m_baseline);

    QDir generated(m_generated.path());
    generated.mkdir(QLatin1String("translation"));
    QString generatedFile = generated.absolutePath() + QLatin1String("/translation/Dialog_without_Buttons_tr.h");

    process.start(m_command, QStringList(baseline.filePath("Dialog_without_Buttons.ui"))
        << QString(QLatin1String("-tr")) << "i18n"
        << QString(QLatin1String("-include")) << "ki18n.h"
        << QString(QLatin1String("-o")) << generatedFile);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QCOMPARE(QFileInfo(generatedFile).exists(), true);
}


void tst_uic::runCompare()
{
    QFile orgFile(m_baseline + QLatin1String("/translation/Dialog_without_Buttons_tr.h"));

    QDir generated(m_generated.path());
    QFile genFile(generated.absolutePath() + QLatin1String("/translation/Dialog_without_Buttons_tr.h"));

    if (!orgFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err(QLatin1String("Could not read file: %1..."));
        QFAIL(err.arg(orgFile.fileName()).toUtf8());
    }

    if (!genFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString err(QLatin1String("Could not read file: %1..."));
        QFAIL(err.arg(genFile.fileName()).toUtf8());
    }

    QString originalFile = orgFile.readAll();
    originalFile.replace(QRegExp(QLatin1String("Created by: Qt User Interface Compiler version [.\\d]{5,5}")), "");

    QString generatedFile = genFile.readAll();
    generatedFile.replace(QRegExp(QLatin1String("Created by: Qt User Interface Compiler version [.\\d]{5,5}")), "");

    QCOMPARE(generatedFile, originalFile);
}

QTEST_MAIN(tst_uic)
#include "tst_uic.moc"
