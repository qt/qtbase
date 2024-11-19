// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstandardpaths.h>

Q_LOGGING_CATEGORY(lcTests, "qt.tools.tests")

QTemporaryDir *g_temporaryDirectory;
QString g_macdeployqtBinary;
QString g_qmakeBinary;
QString g_makeBinary;
QString g_installNameToolBinary;

#if QT_CONFIG(process)

static const QString msgProcessError(const QProcess &process, const QString &what)
{
    QString result;
    QTextStream(&result) << what << ": \"" << process.program() << ' '
        << process.arguments().join(QLatin1Char(' ')) << "\": " << process.errorString();
    return result;
}

static bool runProcess(const QString &binary,
                       const QStringList &arguments,
                       QString *errorMessage,
                       const QString &workingDir = QString(),
                       const QProcessEnvironment &env = QProcessEnvironment(),
                       int timeOut = 10000,
                       QByteArray *stdOut = nullptr, QByteArray *stdErr = nullptr)
{
    QProcess process;
    if (!env.isEmpty())
        process.setProcessEnvironment(env);
    if (!workingDir.isEmpty())
        process.setWorkingDirectory(workingDir);

    const auto outputReader = qScopeGuard([&] {
        QByteArray standardOutput = process.readAllStandardOutput();
        if (!standardOutput.trimmed().isEmpty())
            qCDebug(lcTests).nospace() << "Standard output:\n" << qUtf8Printable(standardOutput.trimmed());
        if (stdOut)
            *stdOut = standardOutput;
        QByteArray standardError = process.readAllStandardError();
        if (!standardError.trimmed().isEmpty())
            qCDebug(lcTests).nospace() << "Standard error:\n" << qUtf8Printable(standardError.trimmed());
        if (stdErr)
            *stdErr = standardError;
    });

    qCDebug(lcTests).noquote() << "Running" << binary
        << "with arguments" << arguments
        << "in" << workingDir;

    process.start(binary, arguments, QIODevice::ReadOnly);
    if (!process.waitForStarted()) {
        *errorMessage = msgProcessError(process, "Failed to start");
        return false;
    }
    if (!process.waitForFinished(timeOut)) {
        *errorMessage = msgProcessError(process, "Timed out");
        process.terminate();
        if (!process.waitForFinished(300))
            process.kill();
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit) {
        *errorMessage = msgProcessError(process, "Crashed");
        return false;
    }
    if (process.exitCode() != QProcess::NormalExit) {
        *errorMessage = msgProcessError(process, "Exit code " + QString::number(process.exitCode()));
        return false;
    }
    return true;
}

#else

static bool runProcess(const QString &binary,
                       const QStringList &arguments,
                       QString *arguments,
                       const QString &workingDir = QString(),
                       const QProcessEnvironment &env = QProcessEnvironment(),
                       int timeOut = 5000,
                       QByteArray *stdOut = Q_NULLPTR, QByteArray *stdErr = Q_NULLPTR)
{
    Q_UNUSED(binary);
    Q_UNUSED(arguments);
    Q_UNUSED(arguments);
    Q_UNUSED(workingDir);
    Q_UNUSED(env);
    Q_UNUSED(timeOut);
    Q_UNUSED(stdOut);
    Q_UNUSED(stdErr);
    return false;
}

#endif

QString sourcePath(const QString &name)
{
    return "source_" + name;
}

QString buildPath(const QString &name)
{
    return g_temporaryDirectory->path() + "/build_" + name;
}

bool qmake(const QString &source, const QString &destination, QString *errorMessage)
{
    QStringList args = QStringList() << source;
    return runProcess(g_qmakeBinary, args, errorMessage, destination);
}

bool make(const QString &destination, QString *errorMessage)
{
    QStringList args;
    return runProcess(g_makeBinary, args, errorMessage, destination,
                      {}, 60000);
}

void build(const QString &name)
{
    // Build the app or framework according to the convention used
    // by this test:
    //   source_name (source code)
    //   build_name  (build artifacts)

    QString source = sourcePath(name);
    QString build = buildPath(name);
    QString profile = name + ".pro";

    QString sourcePath = QFINDTESTDATA(source);
    QVERIFY(!sourcePath.isEmpty());

    // Clear/set up build dir
    QString buildPath = build;
    QVERIFY(QDir(buildPath).removeRecursively());
    QVERIFY(QDir().mkdir(buildPath));
    QVERIFY(QDir(buildPath).exists());

    // Build application
    QString sourceProFile = QDir(sourcePath).canonicalPath() + '/' + profile;
    QString errorMessage;
    QVERIFY2(qmake(sourceProFile, buildPath, &errorMessage), qPrintable(errorMessage));
    QVERIFY2(make(buildPath, &errorMessage), qPrintable(errorMessage));
}

bool changeInstallName(const QString &path, const QString &binary, const QString &from, const QString &to)
{
    QStringList args = QStringList() << binary << "-change" << from << to;
    QString errorMessage;
    return runProcess(g_installNameToolBinary, args, &errorMessage, path);
}

bool deploy(const QString &name, const QStringList &options, QString *errorMessage)
{
    QString bundle = name + ".app";
    QString path = buildPath(name);
    QStringList args = QStringList() << bundle << options;
#if defined(QT_DEBUG) && QT_CONFIG(debug_and_release)
    args << "-use-debug-libs";
#endif
    if (lcTests().isDebugEnabled())
        args << "-verbose=3";
    return runProcess(g_macdeployqtBinary, args, errorMessage, path);
}

bool run(const QString &name, QString *errorMessage)
{
    QString path = buildPath(name);
    QStringList args;
    QString binary = name + ".app/Contents/MacOS/" + name;
    return runProcess(binary, args, errorMessage, path);
}

bool runPrintLibraries(const QString &name, QString *errorMessage, QByteArray *stdErr)
{
    QString binary = name + ".app/Contents/MacOS/" + name;
    QString path = buildPath(name);
    QStringList args;
    QProcessEnvironment env = QProcessEnvironment();
    env.insert("DYLD_PRINT_LIBRARIES", "true");
    QByteArray stdOut;
    return runProcess(binary, args, errorMessage, path, env, 5000, &stdOut, stdErr);
}

void runVerifyDeployment(const QString &name)
{
    QString errorMessage;
    // Verify that the application runs after deployment and that it loads binaries from
    // the application bundle only.
    QByteArray libraries;
    QVERIFY2(runPrintLibraries(name, &errorMessage, &libraries), qPrintable(errorMessage));
    const QList<QString> parts = QString::fromLocal8Bit(libraries).split("dyld: loaded:");
    const QString qtPath = QLibraryInfo::path(QLibraryInfo::PrefixPath);
    // Let assume Qt is not installed in system
    for (const QString &part : parts) {
        const auto trimmed = part.trimmed();
        if (trimmed.isEmpty())
            continue;
        QVERIFY(!trimmed.startsWith(qtPath));
    }
}

class tst_macdeployqt : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void basicapp();
    void plugins_data();
    void plugins();
};

void tst_macdeployqt::initTestCase()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#endif

    if (QProcess::execute("xcode-select", { "-p" }) != 0)
        QSKIP("Xcode or Xcode command line tools not installed");

    // Set up test-global unique temporary directory
    g_temporaryDirectory = new QTemporaryDir();
    g_temporaryDirectory->setAutoRemove(!lcTests().isDebugEnabled());
    QVERIFY(g_temporaryDirectory->isValid());

    // Locate build and deployment tools
    g_macdeployqtBinary = QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/macdeployqt";
    QVERIFY(!g_macdeployqtBinary.isEmpty());
    g_qmakeBinary = QLibraryInfo::path(QLibraryInfo::BinariesPath) + "/qmake";
    QVERIFY(!g_qmakeBinary.isEmpty());
    g_makeBinary = QStandardPaths::findExecutable("make");
    QVERIFY(!g_makeBinary.isEmpty());
    g_installNameToolBinary = QStandardPaths::findExecutable("install_name_tool");
    QVERIFY(!g_installNameToolBinary.isEmpty());
}

void tst_macdeployqt::cleanupTestCase()
{
    delete g_temporaryDirectory;
}

// Verify that deployment of a basic Qt Gui application works
void tst_macdeployqt::basicapp()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#endif

    QString errorMessage;
    QString name = "basicapp";

    // Build and verify that the application runs before deployment
    build(name);
    QVERIFY2(run(name, &errorMessage), qPrintable(errorMessage));

    // Deploy application
    QVERIFY2(deploy(name, QStringList(), &errorMessage), qPrintable(errorMessage));

    // Verify deployment
    runVerifyDeployment(name);
}

void tst_macdeployqt::plugins_data()
{
     QTest::addColumn<QString>("name");
     QTest::newRow("sqlite") << "plugin_sqlite";
     QTest::newRow("tls") << "plugin_tls";
}

void tst_macdeployqt::plugins()
{
    QFETCH(QString, name);

    build(name);

    // Verify that the test app runs before deployment.
    QString errorMessage;
    if (!run(name, &errorMessage)) {
        qDebug() << qPrintable(errorMessage);
        QSKIP("Could not run test application before deployment");
    }

    QVERIFY2(deploy(name, QStringList(), &errorMessage), qPrintable(errorMessage));
    runVerifyDeployment(name);
}

QTEST_MAIN(tst_macdeployqt)
#include "tst_macdeployqt.moc"
