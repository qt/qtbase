// Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QRegularExpression>
#include <QSystemSemaphore>
#include <QXmlStreamReader>

#include <algorithm>
#include <functional>
#include <atomic>
#include <csignal>

#if defined(Q_OS_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include <QtCore/QDeadlineTimer>
#include <QtCore/QThread>
#include <QtCore/QProcessEnvironment>

#include <shellquote_shared.h>

#ifdef Q_CC_MSVC
#define popen _popen
#define QT_POPEN_READ "rb"
#define pclose _pclose
#else
#define QT_POPEN_READ "r"
#endif

using namespace Qt::StringLiterals;

static bool checkJunit(const QByteArray &data) {
    QXmlStreamReader reader{data};
    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement())
            continue;

        if (reader.name() == QStringLiteral("error"))
            return false;

        const QString type = reader.attributes().value(QStringLiteral("type")).toString();
        if (reader.name() == QStringLiteral("failure")) {
            if (type == QStringLiteral("fail") || type == QStringLiteral("xpass"))
                return false;
        }
    }

    // Fail if there's an error after reading through all the xml output
    return !reader.hasError();
}

static bool checkTxt(const QByteArray &data) {
    if (data.indexOf("\nFAIL!  : "_L1) >= 0)
        return false;
    if (data.indexOf("\nXPASS  : "_L1) >= 0)
        return false;
    // Look for "********* Finished testing of tst_QTestName *********"
    static const QRegularExpression testTail("\\*+ +Finished testing of .+ +\\*+"_L1);
    return testTail.match(QLatin1StringView(data)).hasMatch();
}

static bool checkCsv(const QByteArray &data) {
    // The csv format is only suitable for benchmarks,
    // so this is not much useful to determine test failure/success.
    // FIXME: warn the user early on about this.
    Q_UNUSED(data);
    return true;
}

static bool checkXml(const QByteArray &data) {
    QXmlStreamReader reader{data};
    while (!reader.atEnd()) {
        reader.readNext();
        const QString type = reader.attributes().value(QStringLiteral("type")).toString();
        const bool isIncident = (reader.name() == QStringLiteral("Incident"));
        if (reader.isStartElement() && isIncident) {
            if (type == QStringLiteral("fail") || type == QStringLiteral("xpass"))
                return false;
        }
    }

    // Fail if there's an error after reading through all the xml output
    return !reader.hasError();
}

static bool checkLightxml(const QByteArray &data) {
    // lightxml intentionally skips the root element, which technically makes it
    // not valid XML. We'll add that ourselves for the purpose of validation.
    QByteArray newData = data;
    newData.prepend("<root>");
    newData.append("</root>");
    return checkXml(newData);
}

static bool checkTeamcity(const QByteArray &data) {
    if (data.indexOf("' message='Failure! |[Loc: ") >= 0)
        return false;
    const QList<QByteArray> lines = data.trimmed().split('\n');
    if (lines.isEmpty())
        return false;
    return lines.last().startsWith("##teamcity[testSuiteFinished "_L1);
}

static bool checkTap(const QByteArray &data) {
    // This will still report blacklisted fails because QTest with TAP
    // is not putting any data about that.
    if (data.indexOf("\nnot ok ") >= 0)
        return false;

    static const QRegularExpression testTail("ok [0-9]* - cleanupTestCase\\(\\)"_L1);
    return testTail.match(QLatin1StringView(data)).hasMatch();
}

struct Options
{
    bool helpRequested = false;
    bool verbose = false;
    bool skipAddInstallRoot = false;
    int timeoutSecs = 600; // 10 minutes
    QString buildPath;
    QString adbCommand{QStringLiteral("adb")};
    QString makeCommand;
    QString package;
    QString activity;
    QStringList testArgsList;
    QHash<QString, QString> outFiles;
    QString testArgs;
    QString apkPath;
    QString ndkStackPath;
    int sdkVersion = -1;
    int pid = -1;
    bool showLogcatOutput = false;
    const QHash<QString, std::function<bool(const QByteArray &)>> checkFiles = {
        {QStringLiteral("txt"), checkTxt},
        {QStringLiteral("csv"), checkCsv},
        {QStringLiteral("xml"), checkXml},
        {QStringLiteral("lightxml"), checkLightxml},
        {QStringLiteral("xunitxml"), checkJunit},
        {QStringLiteral("junitxml"), checkJunit},
        {QStringLiteral("teamcity"), checkTeamcity},
        {QStringLiteral("tap"), checkTap},
    };
};

static Options g_options;

static bool execCommand(const QString &command, QByteArray *output = nullptr, bool verbose = false)
{
    if (verbose)
        fprintf(stdout, "Execute %s.\n", command.toUtf8().constData());
    FILE *process = popen(command.toUtf8().constData(), QT_POPEN_READ);

    if (!process) {
        fprintf(stderr, "Cannot execute command %s.\n", qPrintable(command));
        return false;
    }
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), process)) {
        if (output)
            output->append(buffer);
        if (verbose)
            fprintf(stdout, "%s", buffer);
    }

    fflush(stdout);
    fflush(stderr);

    return pclose(process) == 0;
}

static bool parseOptions()
{
    QStringList arguments = QCoreApplication::arguments();
    int i = 1;
    for (; i < arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument.compare(QStringLiteral("--adb"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.adbCommand = arguments.at(++i);
        } else if (argument.compare(QStringLiteral("--path"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.buildPath = arguments.at(++i);
        } else if (argument.compare(QStringLiteral("--make"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.makeCommand = arguments.at(++i);
        } else if (argument.compare(QStringLiteral("--apk"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.apkPath = arguments.at(++i);
        } else if (argument.compare(QStringLiteral("--activity"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.activity = arguments.at(++i);
        } else if (argument.compare(QStringLiteral("--skip-install-root"), Qt::CaseInsensitive) == 0) {
            g_options.skipAddInstallRoot = true;
        } else if (argument.compare(QStringLiteral("--show-logcat"), Qt::CaseInsensitive) == 0) {
            g_options.showLogcatOutput = true;
        } else if (argument.compare("--ndk-stack"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.ndkStackPath = arguments.at(++i);
        } else if (argument.compare(QStringLiteral("--timeout"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.timeoutSecs = arguments.at(++i).toInt();
        } else if (argument.compare(QStringLiteral("--help"), Qt::CaseInsensitive) == 0) {
            g_options.helpRequested = true;
        } else if (argument.compare(QStringLiteral("--verbose"), Qt::CaseInsensitive) == 0) {
            g_options.verbose = true;
        } else if (argument.compare(QStringLiteral("--"), Qt::CaseInsensitive) == 0) {
            ++i;
            break;
        } else {
            g_options.testArgsList << arguments.at(i);
        }
    }
    for (;i < arguments.size(); ++i)
        g_options.testArgsList << arguments.at(i);

    if (g_options.helpRequested || g_options.buildPath.isEmpty() || g_options.apkPath.isEmpty())
        return false;

    QString serial = qEnvironmentVariable("ANDROID_DEVICE_SERIAL");
    if (!serial.isEmpty())
        g_options.adbCommand += QStringLiteral(" -s %1").arg(serial);

    if (g_options.ndkStackPath.isEmpty()) {
        const QString ndkPath = qEnvironmentVariable("ANDROID_NDK_ROOT");
        const QString ndkStackPath = ndkPath + QDir::separator() + "ndk-stack"_L1;
        if (QFile::exists(ndkStackPath))
            g_options.ndkStackPath = ndkStackPath;
    }

    return true;
}

static void printHelp()
{
    fprintf(stderr, "Syntax: %s <options> -- [TESTARGS] \n"
                    "\n"
                    "  Creates an Android package in a temp directory <destination> and\n"
                    "  runs it on the default emulator/device or on the one specified by\n"
                    "  \"ANDROID_DEVICE_SERIAL\" environment variable.\n"
                    "\n"
                    "  Mandatory arguments:\n"
                    "    --path <path>: The path where androiddeployqt builds the android package.\n"
                    "\n"
                    "    --apk <apk path>: The test apk path. The apk has to exist already, if it\n"
                    "       does not exist the make command must be provided for building the apk.\n"
                    "\n"
                    "  Optional arguments:\n"
                    "    --make <make cmd>: make command, needed to install the qt library.\n"
                    "       For Qt 5.14+ this can be \"make apk\".\n"
                    "\n"
                    "    --adb <adb cmd>: The Android ADB command. If missing the one from\n"
                    "       $PATH will be used.\n"
                    "\n"
                    "    --activity <acitvity>: The Activity to run. If missing the first\n"
                    "       activity from AndroidManifest.qml file will be used.\n"
                    "\n"
                    "    --timeout <seconds>: Timeout to run the test. Default is 10 minutes.\n"
                    "\n"
                    "    --skip-install-root: Do not append INSTALL_ROOT=... to the make command.\n"
                    "\n"
                    "    --show-logcat: Print Logcat output to stdout.\n"
                    "\n"
                    "    --ndk-stack: Path to ndk-stack tool that symbolizes crash stacktraces.\n"
                    "       By default, ANDROID_NDK_ROOT env var is used to deduce the tool path.\n"
                    "\n"
                    "    -- Arguments that will be passed to the test application.\n"
                    "\n"
                    "    --verbose: Prints out information during processing.\n"
                    "\n"
                    "    --help: Displays this information.\n\n",
                    qPrintable(QCoreApplication::arguments().at(0))
            );
}

static QString packageNameFromAndroidManifest(const QString &androidManifestPath)
{
    QFile androidManifestXml(androidManifestPath);
    if (androidManifestXml.open(QIODevice::ReadOnly)) {
        QXmlStreamReader reader(&androidManifestXml);
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isStartElement() && reader.name() == QStringLiteral("manifest"))
                return reader.attributes().value(QStringLiteral("package")).toString();
        }
    }
    return {};
}

static QString activityFromAndroidManifest(const QString &androidManifestPath)
{
    QFile androidManifestXml(androidManifestPath);
    if (androidManifestXml.open(QIODevice::ReadOnly)) {
        QXmlStreamReader reader(&androidManifestXml);
        while (!reader.atEnd()) {
            reader.readNext();
            if (reader.isStartElement() && reader.name() == QStringLiteral("activity"))
                return reader.attributes().value(QStringLiteral("android:name")).toString();
        }
    }
    return {};
}

static void setOutputFile(QString file, QString format)
{
    if (file.isEmpty())
        file = QStringLiteral("-");
    if (format.isEmpty())
        format = QStringLiteral("txt");

    g_options.outFiles[format] = file;
}

static bool parseTestArgs()
{
    QRegularExpression oldFormats{QStringLiteral("^-(txt|csv|xunitxml|junitxml|xml|lightxml|teamcity|tap)$")};
    QRegularExpression newLoggingFormat{QStringLiteral("^(.*),(txt|csv|xunitxml|junitxml|xml|lightxml|teamcity|tap)$")};

    QString file;
    QString logType;
    QStringList unhandledArgs;
    for (int i = 0; i < g_options.testArgsList.size(); ++i) {
        const QString &arg = g_options.testArgsList[i].trimmed();
        if (arg == QStringLiteral("--"))
            continue;
        if (arg == QStringLiteral("-o")) {
            if (i >= g_options.testArgsList.size() - 1)
                return false; // missing file argument

            const auto &filePath = g_options.testArgsList[++i];
            const auto match = newLoggingFormat.match(filePath);
            if (!match.hasMatch()) {
                file = filePath;
            } else {
                const auto capturedTexts = match.capturedTexts();
                setOutputFile(capturedTexts.at(1), capturedTexts.at(2));
            }
        } else {
            auto match = oldFormats.match(arg);
            if (match.hasMatch()) {
                logType = match.capturedTexts().at(1);
            } else {
                unhandledArgs << QStringLiteral(" \\\"%1\\\"").arg(arg);
            }
        }
    }
    if (g_options.outFiles.isEmpty() || !file.isEmpty() || !logType.isEmpty())
        setOutputFile(file, logType);

    for (const auto &format : g_options.outFiles.keys())
        g_options.testArgs += QStringLiteral(" -o output.%1,%1").arg(format);

    g_options.testArgs += unhandledArgs.join(u' ');

    // Pass over any testlib env vars if set
    QString testEnvVars;
    const QStringList envVarsList = QProcessEnvironment::systemEnvironment().toStringList();
    for (const QString &var : envVarsList) {
        if (var.startsWith("QTEST_"_L1))
            testEnvVars += "%1 "_L1.arg(var);
    }

    if (!testEnvVars.isEmpty()) {
        testEnvVars = QString::fromUtf8(testEnvVars.trimmed().toUtf8().toBase64());
        testEnvVars = "-e extraenvvars \"%4\""_L1.arg(testEnvVars);
    }

    g_options.testArgs = "shell am start -n %1/%2 -e applicationArguments \"%3\" %4"_L1
                                 .arg(g_options.package)
                                 .arg(g_options.activity)
                                 .arg(shellQuote(g_options.testArgs.trimmed()))
                                 .arg(testEnvVars)
                                 .trimmed();

    return true;
}

static bool obtainPid() {
    QByteArray output;
    const auto psCmd = "%1 shell \"ps | grep ' %2'\""_L1.arg(g_options.adbCommand,
                                                            shellQuote(g_options.package));
    if (!execCommand(psCmd, &output))
        return false;

    const QList<QByteArray> lines = output.split(u'\n');
    if (lines.size() < 1)
        return false;

    QList<QByteArray> columns = lines.first().simplified().replace(u'\t', u' ').split(u' ');
    if (columns.size() < 3)
        return false;

    if (g_options.pid == -1) {
        bool ok = false;
        int pid = columns.at(1).toInt(&ok);
        if (ok)
            g_options.pid = pid;
    }

    return true;
}

static bool isRunning() {
    QByteArray output;
    const auto psCmd = "%1 shell \"ps | grep ' %2'\""_L1.arg(g_options.adbCommand,
                                                            shellQuote(g_options.package));
    if (!execCommand(psCmd, &output))
        return false;

    return output.indexOf(QLatin1StringView(" " + g_options.package.toUtf8())) > -1;
}

std::atomic<bool> isPackageInstalled { false };
std::atomic<bool> isTestRunnerInterrupted { false };

static void waitForStartedAndFinished()
{
    // wait to start and set PID
    QDeadlineTimer startDeadline(10000);
    do {
        if (obtainPid())
            break;
        QThread::msleep(100);
    } while (!startDeadline.hasExpired() && !isTestRunnerInterrupted.load());

    // Wait to finish
    QDeadlineTimer finishedDeadline(g_options.timeoutSecs * 1000);
    do {
        if (!isRunning())
            break;
        QThread::msleep(250);
    } while (!finishedDeadline.hasExpired() && !isTestRunnerInterrupted.load());

    if (finishedDeadline.hasExpired())
        qWarning() << "Timed out while waiting for the test to finish";
}

static void obtainSdkVersion()
{
    // SDK version is necessary, as in SDK 23 pidof is broken, so we cannot obtain the pid.
    // Also, Logcat cannot filter by pid in SDK 23, so we don't offer the --show-logcat option.
    QByteArray output;
    const QString command(
            QStringLiteral("%1 shell getprop ro.build.version.sdk").arg(g_options.adbCommand));
    execCommand(command, &output, g_options.verbose);
    bool ok = false;
    int sdkVersion = output.toInt(&ok);
    if (ok) {
        g_options.sdkVersion = sdkVersion;
    } else {
        fprintf(stderr,
                "Unable to obtain the SDK version of the target. Command \"%s\" "
                "returned \"%s\"\n",
                command.toUtf8().constData(), output.constData());
        fflush(stderr);
    }
}

static bool pullFiles()
{
    bool ret = true;
    QByteArray userId;
    // adb get-current-user command is available starting from API level 26.
    if (g_options.sdkVersion >= 26) {
        const QString userIdCmd = "%1 shell cmd activity get-current-user"_L1.arg(g_options.adbCommand);
        if (!execCommand(userIdCmd, &userId)) {
            qCritical() << "Error: failed to retrieve the user ID";
            return false;
        }
    } else {
        userId = "0";
    }

    for (auto it = g_options.outFiles.constBegin(); it != g_options.outFiles.end(); ++it) {
        // Get only stdout from cat and get rid of stderr and fail later if the output is empty
        const QString outSuffix = it.key();
        const QString catCmd = "cat files/output.%1 2> /dev/null"_L1.arg(outSuffix);
        const QString fullCatCmd = "%1 shell 'run-as %2 --user %3 %4'"_L1.arg(
                g_options.adbCommand, g_options.package, QString::fromUtf8(userId.simplified()),
                catCmd);

        QByteArray output;
        if (!execCommand(fullCatCmd, &output)) {
            qCritical() << "Error: failed to retrieve the test's output.%1 file."_L1.arg(outSuffix);
            return false;
        }

        if (output.isEmpty()) {
            qCritical() << "Error: the test's output.%1 is empty."_L1.arg(outSuffix);
            return false;
        }

        auto checkerIt = g_options.checkFiles.find(outSuffix);
        ret &= (checkerIt != g_options.checkFiles.end() && checkerIt.value()(output));
        if (it.value() == "-"_L1) {
            fprintf(stdout, "%s", output.constData());
            fflush(stdout);
        } else {
            QFile out{it.value()};
            if (!out.open(QIODevice::WriteOnly))
                return false;
            out.write(output);
        }
    }
    return ret;
}

void printLogcat(const QString &formattedTime)
{
    QString logcatCmd = "%1 logcat "_L1.arg(g_options.adbCommand);
    if (g_options.sdkVersion <= 23 || g_options.pid == -1)
        logcatCmd += "-t '%1'"_L1.arg(formattedTime);
    else
        logcatCmd += "-d --pid=%1"_L1.arg(QString::number(g_options.pid));

    QByteArray logcat;
    if (!execCommand(logcatCmd, &logcat)) {
        qCritical() << "Error: failed to fetch logcat of the test";
        return;
    }

    if (logcat.isEmpty()) {
        qWarning() << "The retrieved logcat is empty";
        return;
    }

    qDebug() << "****** Begin logcat output ******";
    qDebug().noquote() << logcat;
    qDebug() << "****** End logcat output ******";
}

static QString getDeviceABI()
{
    const QString abiCmd = "%1 shell getprop ro.product.cpu.abi"_L1.arg(g_options.adbCommand);
    QByteArray abi;
    if (!execCommand(abiCmd, &abi)) {
        qWarning() << "Warning: failed to get the device abi, fallback to first libs dir";
        return {};
    }

    return QString::fromUtf8(abi.simplified());
}

void printLogcatCrashBuffer(const QString &formattedTime)
{
    QString crashCmd = "%1 logcat -b crash -t '%2'"_L1.arg(g_options.adbCommand, formattedTime);

    if (!g_options.ndkStackPath.isEmpty()) {
        auto libsPath = "%1/libs/"_L1.arg(g_options.buildPath);
        QString abi = getDeviceABI();
        if (abi.isEmpty()) {
            QStringList subDirs = QDir(libsPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (!subDirs.isEmpty())
                abi = subDirs.first();
        }

        if (!abi.isEmpty()) {
            libsPath += abi;
            crashCmd += " | %1 -sym %2"_L1.arg(g_options.ndkStackPath, libsPath);
        } else {
            qWarning() << "Warning: failed to get the libs abi, ndk-stack cannot be used.";
        }
    } else {
        qWarning() << "Warning: ndk-stack path not provided and couldn't be deduced "
                      "using the ANDROID_NDK_ROOT environment variable.";
    }

    QByteArray crashLogcat;
    if (!execCommand(crashCmd, &crashLogcat)) {
        qCritical() << "Error: failed to fetch logcat crash buffer";
        return;
    }

    if (crashLogcat.isEmpty()) {
        qDebug() << "The retrieved logcat crash buffer is empty";
        return;
    }

    qDebug() << "****** Begin logcat crash buffer output ******";
    qDebug().noquote() << crashLogcat;
    qDebug() << "****** End logcat crash buffer output ******";
}

static QString getCurrentTimeString()
{
    const QString timeFormat = (g_options.sdkVersion <= 23) ?
            "%m-%d\\ %H:%M:%S.000"_L1 : "%Y-%m-%d\\ %H:%M:%S.%3N"_L1;

    QString dateCmd = "%1 shell date +'%2'"_L1.arg(g_options.adbCommand, timeFormat);
    QByteArray output;
    if (!execCommand(dateCmd, &output)) {
        qWarning() << "Date/time adb command failed";
        return {};
    }

    return QString::fromUtf8(output.simplified());
}

static bool uninstallTestPackage()
{
    return execCommand(QStringLiteral("%1 uninstall %2").arg(g_options.adbCommand,
                       g_options.package), nullptr, g_options.verbose);
}

struct TestRunnerSystemSemaphore
{
    TestRunnerSystemSemaphore() { }
    ~TestRunnerSystemSemaphore() { release(); }

    void acquire() { isAcquired.store(semaphore.acquire()); }

    void release()
    {
        bool expected = true;
        // NOTE: There's still could be tiny time gap between the compare_exchange_strong() call
        // and release() call where the thread could be interrupted, if that's ever an issue,
        // this code could be checked and improved further.
        if (isAcquired.compare_exchange_strong(expected, false))
            isAcquired.store(!semaphore.release());
    }

    std::atomic<bool> isAcquired { false };
    QSystemSemaphore semaphore { QSystemSemaphore::platformSafeKey(u"androidtestrunner"_s),
                                   1, QSystemSemaphore::Open };
};

TestRunnerSystemSemaphore testRunnerLock;

void sigHandler(int signal)
{
    std::signal(signal, SIG_DFL);
    testRunnerLock.release();
    // Ideally we shouldn't be doing such calls from a signal handler,
    // and we can't use QSocketNotifier because this tool doesn't spin
    // a main event loop. Since, there's no other alternative to do this,
    // let's do the cleanup anyway.
    if (!isPackageInstalled.load())
        _exit(-1);
    isTestRunnerInterrupted.store(true);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    QCoreApplication a(argc, argv);
    if (!parseOptions()) {
        printHelp();
        return 1;
    }

    if (g_options.makeCommand.isEmpty()) {
        fprintf(stderr,
                "It is required to provide a make command with the \"--make\" parameter "
                "to generate the apk.\n");
        return 1;
    }
    if (!execCommand(g_options.makeCommand, nullptr, true)) {
        if (!g_options.skipAddInstallRoot) {
            // we need to run make INSTALL_ROOT=path install to install the application file(s) first
            if (!execCommand(QStringLiteral("%1 INSTALL_ROOT=%2 install")
                             .arg(g_options.makeCommand, QDir::toNativeSeparators(g_options.buildPath)), nullptr, g_options.verbose)) {
                return 1;
            }
        } else {
            if (!execCommand(QStringLiteral("%1")
                             .arg(g_options.makeCommand), nullptr, g_options.verbose)) {
                return 1;
            }
        }
    }

    if (!QFile::exists(g_options.apkPath)) {
        fprintf(stderr,
                "No apk \"%s\" found after running the make command. Check the provided path and "
                "the make command.\n",
                qPrintable(g_options.apkPath));
        return 1;
    }

    obtainSdkVersion();

    QString manifest = g_options.buildPath + QStringLiteral("/AndroidManifest.xml");
    g_options.package = packageNameFromAndroidManifest(manifest);
    if (g_options.activity.isEmpty())
        g_options.activity = activityFromAndroidManifest(manifest);

    // parseTestArgs depends on g_options.package
    if (!parseTestArgs())
        return 1;

    // do not install or run packages while another test is running
    testRunnerLock.acquire();

    isPackageInstalled.store(execCommand(QStringLiteral("%1 install -r -g %2")
                    .arg(g_options.adbCommand, g_options.apkPath), nullptr, g_options.verbose));
    if (!isPackageInstalled)
        return 1;

    const QString formattedTime = getCurrentTimeString();

    // start the tests
    const auto startCmd = "%1 %2"_L1.arg(g_options.adbCommand, g_options.testArgs);
    bool success = execCommand(startCmd, nullptr, g_options.verbose);

    waitForStartedAndFinished();

    if (success) {
        success &= pullFiles();
        if (g_options.showLogcatOutput)
            printLogcat(formattedTime);
    }

    // If we have a failure, attempt to print both logcat and the crash buffer which
    // includes the crash stacktrace that is not included in the default logcat.
    if (!success) {
        printLogcat(formattedTime);
        printLogcatCrashBuffer(formattedTime);
    }

    success &= uninstallTestPackage();
    fflush(stdout);

    testRunnerLock.release();

    if (isTestRunnerInterrupted.load()) {
        qCritical() << "The androidtestrunner was interrupted and the was test cleaned up.";
        return 1;
    }

    return success ? 0 : 1;
}
