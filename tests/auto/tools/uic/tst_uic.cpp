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
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QVector>

#include <cstdio>

static const char keepEnvVar[] = "UIC_KEEP_GENERATED_FILES";
static const char diffToStderrEnvVar[] = "UIC_STDERR_DIFF";

struct TestEntry
{
    QByteArray name;
    QString baselineBaseName;
    QString generatedFileName;
};

class tst_uic : public QObject
{
    Q_OBJECT

public:
    using TestEntries = QVector<TestEntry>;

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

    void python();
    void python_data() const;

    void runCompare();

private:
    void populateTestEntries();

    const QString m_command;
    QString m_baseline;
    QTemporaryDir m_generated;
    TestEntries m_testEntries;
    QRegularExpression m_versionRegexp;
    QString m_python;
};

tst_uic::tst_uic()
    : m_command(QLibraryInfo::location(QLibraryInfo::BinariesPath) + QLatin1String("/uic"))
    , m_versionRegexp(QLatin1String(R"(\*\* Created by: Qt User Interface Compiler version \d{1,2}\.\d{1,2}\.\d{1,2})"))
{
}

static QByteArray msgProcessStartFailed(const QString &command, const QString &why)
{
    const QString result = QString::fromLatin1("Could not start %1: %2")
            .arg(command, why);
    return result.toLocal8Bit();
}

// Locate Python and check whether PySide2 is installed
static QString locatePython(QTemporaryDir &generatedDir)
{
    QString python = QStandardPaths::findExecutable(QLatin1String("python"));
    if (python.isEmpty()) {
        qWarning("Cannot locate python, skipping tests");
        return QString();
    }
    QFile importTestFile(generatedDir.filePath(QLatin1String("import_test.py")));
    if (!importTestFile.open(QIODevice::WriteOnly| QIODevice::Text))
        return QString();
    importTestFile.write("import PySide2.QtCore\n");
    importTestFile.close();
    QProcess process;
    process.start(python, {importTestFile.fileName()});
    if (!process.waitForStarted() || !process.waitForFinished())
        return QString();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError()).simplified();
        qWarning("PySide2 is not installed (%s)", qPrintable(stdErr));
        return QString();
    }
    importTestFile.remove();
    return python;
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
    populateTestEntries();
    QVERIFY(!m_testEntries.isEmpty());
    qDebug("%s", qPrintable(msg));

    m_python = locatePython(m_generated);
}

void tst_uic::populateTestEntries()
{
    const QString generatedPrefix = m_generated.path() + QLatin1Char('/');
    QDir baseline(m_baseline);
    const QString baseLinePrefix = baseline.path() + QLatin1Char('/');
    const QFileInfoList baselineFiles =
        baseline.entryInfoList(QStringList(QString::fromLatin1("*.ui")), QDir::Files);
    m_testEntries.reserve(baselineFiles.size());
    for (const QFileInfo &baselineFile : baselineFiles) {
        const QString baseName = baselineFile.baseName();
        const QString baselineBaseName = baseLinePrefix + baseName;
        const QString generatedFile = generatedPrefix + baselineFile.fileName()
            + QLatin1String(".h");
        m_testEntries.append(TestEntry{baseName.toLocal8Bit(), baselineBaseName, generatedFile});
    }
}

static const char helpFormat[] = R"(
Note: The environment variable '%s' can be set to keep the temporary files
for error analysis.
The environment variable '%s' can be set to redirect the diff output to
stderr.)";

void tst_uic::cleanupTestCase()
{
    if (qEnvironmentVariableIsSet(keepEnvVar)) {
        m_generated.setAutoRemove(false);
        qDebug("Keeping generated files in '%s'", qPrintable(QDir::toNativeSeparators(m_generated.path())));
    } else {
        qDebug(helpFormat, keepEnvVar, diffToStderrEnvVar);
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
    QVERIFY(QFileInfo::exists(generatedFile));
}

void tst_uic::run_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");
    QTest::addColumn<QStringList>("options");

    for (const TestEntry &te : m_testEntries) {
        QStringList options;
        if (te.name == QByteArrayLiteral("qttrid"))
            options << QStringList(QLatin1String("-idbased"));
        QTest::newRow(te.name.constData()) << (te.baselineBaseName + QLatin1String(".ui"))
            << te.generatedFileName << options;
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

static void outputDiff(const QString &diff)
{
    // Use patch -p3 < diff to apply the obtained diff output in the baseline directory.
    static const bool diffToStderr = qEnvironmentVariableIsSet(diffToStderrEnvVar);
    if (diffToStderr)
        std::fputs(qPrintable(diff), stderr);
    else
        qWarning("Difference:\n%s", qPrintable(diff));
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
            outputDiff(diff);
    }

    QCOMPARE(generatedFileContents, originalFileContents);
}

void tst_uic::compare_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    for (const TestEntry &te : m_testEntries) {
        QTest::newRow(te.name.constData()) << (te.baselineBaseName + QLatin1String(".ui.h"))
            << te.generatedFileName;
    }
}

void tst_uic::runTranslation()
{
    QProcess process;

    const QDir baseline(m_baseline);

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
    QVERIFY(QFileInfo::exists(generatedFile));
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
            outputDiff(diff);
    }

    QCOMPARE(generatedFileContents, originalFileContents);
}

// Let uic generate Python code and verify that it is syntactically
// correct by compiling it into .pyc. This test is executed only
// when python with an installed Qt for Python is detected (see locatePython()).

static inline QByteArray msgCompilePythonFailed(const QByteArray &error)
{
    // If there is a line with blanks and caret indicating an error in the line
    // above, insert the cursor into the offending line and remove the caret.
    QByteArrayList lines = error.trimmed().split('\n');
    for (int i = lines.size() -  1; i > 0; --i) {
        const auto &line = lines.at(i);
        const int caret = line.indexOf('^');
        if (caret == 0 || (caret > 0 && line.at(caret - 1) == ' ')) {
            lines.removeAt(i);
            lines[i - 1].insert(caret, '|');
            break;
        }
    }
    return lines.join('\n');
}

void tst_uic::python_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    const int size = m_python.isEmpty()
        ? qMin(1, m_testEntries.size()) : m_testEntries.size();
    for (int i = 0; i < size; ++i) {
        const TestEntry &te = m_testEntries.at(i);
        // qprintsettingsoutput: variable named 'from' clashes with Python
        if (!te.baselineBaseName.endsWith(QLatin1String("/qprintsettingsoutput"))) {
            QString generatedFile = te.generatedFileName;
            generatedFile.chop(1); // foo.h -> foo.py
            generatedFile.append(QLatin1String("py"));
            QTest::newRow(te.name.constData())
                << (te.baselineBaseName + QLatin1String(".ui"))
                << generatedFile;
        }
    }
}

void tst_uic::python()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);
    if (m_python.isEmpty())
        QSKIP("Python was not found");

    QStringList uicArguments{QLatin1String("-g"), QLatin1String("python"),
        originalFile, QLatin1String("-o"), generatedFile};
    QProcess process;
    process.setWorkingDirectory(m_generated.path());
    process.start(m_command, uicArguments);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(QFileInfo::exists(generatedFile));

    // Test Python code generation by compiling the file
    QStringList compileArguments{QLatin1String("-m"), QLatin1String("py_compile"), generatedFile};
    process.start(m_python, compileArguments);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    const bool compiled = process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0;
    QVERIFY2(compiled, msgCompilePythonFailed(process.readAllStandardError()).constData());
}

QTEST_MAIN(tst_uic)
#include "tst_uic.moc"
