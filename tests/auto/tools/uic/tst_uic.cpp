// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


#include <QtCore/QDir>
#include <QtCore/QString>
#include <QTest>
#include <QtCore/QProcess>
#include <QtCore/QByteArray>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTemporaryDir>
#include <QtCore/QRegularExpression>
#include <QtCore/QStandardPaths>
#include <QtCore/QList>

#include <cstdio>

using namespace Qt::StringLiterals;

static const char keepEnvVar[] = "UIC_KEEP_GENERATED_FILES";
static const char diffToStderrEnvVar[] = "UIC_STDERR_DIFF";

struct TestEntry
{
    enum Flag
    {
        IdBasedTranslation = 0x1,
        Python = 0x2, // Python baseline is present
        DontTestPythonCompile = 0x4 // Do not test Python compilation
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QByteArray name;
    QString uiFileName;
    QString baseLineFileName;
    QString generatedFileName;
    Flags flags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TestEntry::Flags)

class tst_uic : public QObject
{
    Q_OBJECT

public:
    using TestEntries = QList<TestEntry>;

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

    void pythonCompile();
    void pythonCompile_data() const;

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

static const char versionRegexp[] =
    R"([*#][*#] Created by: Qt User Interface Compiler version \d{1,2}\.\d{1,2}\.\d{1,2})";

tst_uic::tst_uic()
    : m_command(QLibraryInfo::path(QLibraryInfo::LibraryExecutablesPath) + "/uic"_L1)
    , m_versionRegexp(QLatin1String(versionRegexp))
{
}

static QByteArray msgProcessStartFailed(const QString &command, const QString &why)
{
    const QString result = QString::fromLatin1("Could not start %1: %2")
            .arg(command, why);
    return result.toLocal8Bit();
}

// Locate Python and check whether Qt for Python is installed
static QString locatePython(QTemporaryDir &generatedDir)
{
    const QString python = QStandardPaths::findExecutable("python"_L1);
    if (python.isEmpty()) {
        qWarning("Cannot locate python, skipping tests");
        return {};
    }
    QFile importTestFile(generatedDir.filePath("import_test.py"_L1));
    if (!importTestFile.open(QIODevice::WriteOnly| QIODevice::Text))
        return {};
    importTestFile.write("import PySide");
    importTestFile.write(QByteArray::number(QT_VERSION_MAJOR));
    importTestFile.write(".QtCore\n");
    importTestFile.close();
    QProcess process;
    process.start(python, {importTestFile.fileName()});
    if (!process.waitForStarted() || !process.waitForFinished())
        return {};
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        const QString stdErr = QString::fromLocal8Bit(process.readAllStandardError()).simplified();
        qWarning("PySide6 is not installed (%s)", qPrintable(stdErr));
        return {};
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
    process.start(m_command, {"-help"_L1});
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
    const QString generatedPrefix = m_generated.path() + u'/';
    const QDir baseline(m_baseline);
    const QString baseLinePrefix = baseline.path() + u'/';
    const QFileInfoList baselineFiles =
        baseline.entryInfoList(QStringList(QString::fromLatin1("*.ui")), QDir::Files);
    m_testEntries.reserve(baselineFiles.size());
    for (const QFileInfo &baselineFile : baselineFiles) {
        const QString baseName = baselineFile.baseName();
        TestEntry entry;
        // qprintsettingsoutput: variable named 'from' clashes with Python
        if (baseName == "qprintsettingsoutput"_L1)
            entry.flags.setFlag(TestEntry::DontTestPythonCompile);
        else if (baseName  == "qttrid"_L1)
            entry.flags.setFlag(TestEntry::IdBasedTranslation);
        entry.name = baseName.toLocal8Bit();
        entry.uiFileName = baselineFile.absoluteFilePath();
        entry.baseLineFileName = entry.uiFileName + ".h"_L1;
        const QString generatedFilePrefix = generatedPrefix + baselineFile.fileName();
        entry.generatedFileName = generatedFilePrefix + ".h"_L1;
        m_testEntries.append(entry);
        // Check for a Python baseline
        entry.baseLineFileName = entry.uiFileName + ".py"_L1;
        if (QFileInfo::exists(entry.baseLineFileName)) {
            entry.name.append(QByteArrayLiteral("-python"));
            entry.flags.setFlag(TestEntry::DontTestPythonCompile);
            entry.flags.setFlag(TestEntry::Python);
            entry.generatedFileName = generatedFilePrefix + ".py"_L1;
            m_testEntries.append(entry);
        }
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
    const QFileInfoList baselineFiles = baseline.entryInfoList({"*.ui"_L1}, QDir::Files);
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
    process.start(m_command, QStringList{originalFile, "-o"_L1, generatedFile} << options);
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
        if (te.flags.testFlag(TestEntry::IdBasedTranslation))
            options.append("-idbased"_L1);
        if (te.flags.testFlag(TestEntry::Python))
            options << "-g"_L1 << "python"_L1;
        QTest::newRow(te.name.constData()) << te.uiFileName
            << te.generatedFileName << options;
    }
}

// Helpers to generate a diff using the standard diff tool if present for failures.
static inline QString diffBinary()
{
    QString binary = "diff"_L1;
#ifdef Q_OS_WIN
    binary += ".exe"_L1;
#endif
    return QStandardPaths::findExecutable(binary);
}

static QString generateDiff(const QString &originalFile, const QString &generatedFile)
{
    static const QString diff = diffBinary();
    if (diff.isEmpty())
        return {};
    const QStringList args = {"-u"_L1, QDir::toNativeSeparators(originalFile),
                              QDir::toNativeSeparators(generatedFile)};
    QProcess diffProcess;
    diffProcess.start(diff, args);
    return diffProcess.waitForStarted() && diffProcess.waitForFinished()
        ? QString::fromLocal8Bit(diffProcess.readAllStandardOutput()) : QString();
}

static QByteArray msgCannotReadFile(const QFile &file)
{
    const QString result = "Could not read file: "_L1
        + QDir::toNativeSeparators(file.fileName()) + ": "_L1 + file.errorString();
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
        QTest::newRow(te.name.constData()) << te.baseLineFileName
            << te.generatedFileName;
    }
}

void tst_uic::runTranslation()
{
    QProcess process;

    const QDir baseline(m_baseline);

    QDir generated(m_generated.path());
    generated.mkdir("translation"_L1);
    QString generatedFile = generated.absolutePath() + "/translation/Dialog_without_Buttons_tr.h"_L1;

    process.start(m_command, {baseline.filePath("Dialog_without_Buttons.ui"),
                              "-tr"_L1, "i18n"_L1, "-include"_L1, "ki18n.h"_L1,
                              "-o"_L1, generatedFile});
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(QFileInfo::exists(generatedFile));
}


void tst_uic::runCompare()
{
    const QString dialogFile = "/translation/Dialog_without_Buttons_tr.h"_L1;
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
    for (auto i = lines.size() -  1; i > 0; --i) {
        const auto &line = lines.at(i);
        const auto caret = line.indexOf(u'^');
        if (caret == 0 || (caret > 0 && line.at(caret - 1) == u' ')) {
            lines.removeAt(i);
            lines[i - 1].insert(caret, u'|');
            break;
        }
    }
    return lines.join(u'\n');
}

// Test Python code generation by compiling the file
void tst_uic::pythonCompile_data() const
{
    QTest::addColumn<QString>("originalFile");
    QTest::addColumn<QString>("generatedFile");

    const auto size = m_python.isEmpty()
        ? qMin(qsizetype(1), m_testEntries.size()) : m_testEntries.size();
    for (qsizetype i = 0; i < size; ++i) {
        const TestEntry &te = m_testEntries.at(i);
        if (!te.flags.testFlag(TestEntry::DontTestPythonCompile)) {
            QTest::newRow(te.name.constData())
                << te.uiFileName
                << te.generatedFileName;
        }
    }
}

void tst_uic::pythonCompile()
{
    QFETCH(QString, originalFile);
    QFETCH(QString, generatedFile);
    if (m_python.isEmpty())
        QSKIP("Python was not found");

    const QStringList uicArguments{"-g"_L1, "python"_L1,
                                   originalFile, "-o"_L1, generatedFile};
    QProcess process;
    process.setWorkingDirectory(m_generated.path());
    process.start(m_command, uicArguments);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), 0);
    QVERIFY(QFileInfo::exists(generatedFile));

    // Test Python code generation by compiling the file
    const QStringList compileArguments{"-m"_L1, "py_compile"_L1, generatedFile};
    process.start(m_python, compileArguments);
    QVERIFY2(process.waitForStarted(), msgProcessStartFailed(m_command, process.errorString()));
    QVERIFY(process.waitForFinished());
    const bool compiled = process.exitStatus() == QProcess::NormalExit
        && process.exitCode() == 0;
    QVERIFY2(compiled, msgCompilePythonFailed(process.readAllStandardError()).constData());
}

QTEST_MAIN(tst_uic)
#include "tst_uic.moc"
