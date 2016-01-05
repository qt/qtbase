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


#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtTest/QtTest>
#include <QtCore/QProcess>
#include <QtCore/QByteArray>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTemporaryDir>
#include <QtCore/QStandardPaths>

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
    QRegExp m_versionRegexp;
};

tst_uic::tst_uic()
    : m_command(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/uic"))
    , m_versionRegexp(QLatin1String("Created by: Qt User Interface Compiler version [.\\d]{5,5}"))
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
    QVERIFY2(m_generated.isValid(), qPrintable(m_generated.errorString()));
    QVERIFY(m_versionRegexp.isValid());
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
    QString msg = QString::fromLatin1("uic test running in '%1' using: ").
                  arg(QDir::currentPath());
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
    QFETCH(QStringList, options);

    QProcess process;
    process.start(m_command, QStringList(originalFile)
        << QString(QLatin1String("-o")) << generatedFile << options);
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
    QTest::addColumn<QStringList>("options");

    QDir generated(m_generated.path());
    QDir baseline(m_baseline);
    const QFileInfoList baselineFiles = baseline.entryInfoList(QStringList("*.ui"), QDir::Files);
    foreach (const QFileInfo &baselineFile, baselineFiles) {
        const QString generatedFile = generated.absolutePath()
            + QLatin1Char('/') + baselineFile.fileName()
            + QLatin1String(".h");

        QStringList options;
        if (baselineFile.fileName() == QLatin1String("qttrid.ui"))
            options << QStringList(QLatin1String("-idbased"));

        QTest::newRow(qPrintable(baselineFile.baseName()))
            << baselineFile.absoluteFilePath()
            << generatedFile
            << options;
    }
}

// Helpers to generate a diff using the standard diff tool if present for failures.
static inline QString diffBinary()
{
    QString binary = QLatin1String("diff");
#ifdef Q_OS_WIN
    binary += QLatin1String(".exe");
#endif
    return QStandardPaths::findExecutable(binary);
}

static QString generateDiff(const QString &originalFile, const QString &generatedFile)
{
    static const QString diff = diffBinary();
    if (diff.isEmpty())
        return QString();
    const QStringList args = QStringList() << QLatin1String("-u")
        << QDir::toNativeSeparators(originalFile)
        << QDir::toNativeSeparators(generatedFile);
    QProcess diffProcess;
    diffProcess.start(diff, args);
    return diffProcess.waitForStarted() && diffProcess.waitForFinished()
        ? QString::fromLocal8Bit(diffProcess.readAllStandardOutput()) : QString();
}

static QByteArray msgCannotReadFile(const QFile &file)
{
    const QString result = QLatin1String("Could not read file: ")
        + QDir::toNativeSeparators(file.fileName())
        + QLatin1String(": ") + file.errorString();
    return result.toLocal8Bit();
}

void tst_uic::compare()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);

    QFile orgFile(originalFile);
    QFile genFile(generatedFile);

    QVERIFY2(orgFile.open(QIODevice::ReadOnly | QIODevice::Text), msgCannotReadFile(orgFile));

    QVERIFY2(genFile.open(QIODevice::ReadOnly | QIODevice::Text), msgCannotReadFile(genFile));

    QString originalFileContents = orgFile.readAll();
    originalFileContents.replace(m_versionRegexp, QString());

    QString generatedFileContents = genFile.readAll();
    generatedFileContents.replace(m_versionRegexp, QString());

    if (generatedFileContents != originalFileContents) {
        const QString diff = generateDiff(originalFile, generatedFile);
        if (!diff.isEmpty())
             qWarning().noquote().nospace() << "Difference:\n" << diff;
    }

    QCOMPARE(generatedFileContents, originalFileContents);
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
    const QString dialogFile = QLatin1String("/translation/Dialog_without_Buttons_tr.h");
    const QString originalFile = m_baseline + dialogFile;
    QFile orgFile(originalFile);

    QDir generated(m_generated.path());
    const QString generatedFile = generated.absolutePath() + dialogFile;
    QFile genFile(generatedFile);

    QVERIFY2(orgFile.open(QIODevice::ReadOnly | QIODevice::Text), msgCannotReadFile(orgFile));

    QVERIFY2(genFile.open(QIODevice::ReadOnly | QIODevice::Text), msgCannotReadFile(genFile));

    QString originalFileContents = orgFile.readAll();
    originalFileContents.replace(m_versionRegexp, QString());

    QString generatedFileContents = genFile.readAll();
    generatedFileContents.replace(m_versionRegexp, QString());

    if (generatedFileContents != originalFileContents) {
        const QString diff = generateDiff(originalFile, generatedFile);
        if (!diff.isEmpty())
             qWarning().noquote().nospace() << "Difference:\n" << diff;
    }

    QCOMPARE(generatedFileContents, originalFileContents);
}

QTEST_MAIN(tst_uic)
#include "tst_uic.moc"
