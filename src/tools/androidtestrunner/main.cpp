// Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QRegularExpression>
#include <QtCore/QSystemSemaphore>
#include <QtCore/QThread>
#include <QtCore/QXmlStreamReader>

#include <atomic>
#include <csignal>
#include <functional>
#if defined(Q_OS_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace Qt::StringLiterals;

static bool checkJunit(const QByteArray &data) {
    QXmlStreamReader reader{data};
    while (!reader.atEnd()) {
        reader.readNext();

        if (!reader.isStartElement())
            continue;

        if (reader.name() == "error"_L1)
            return false;

        const QString type = reader.attributes().value("type"_L1).toString();
        if (reader.name() == "failure"_L1) {
            if (type == "fail"_L1 || type == "xpass"_L1)
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
        const QString type = reader.attributes().value("type"_L1).toString();
        const bool isIncident = (reader.name() == "Incident"_L1);
        if (reader.isStartElement() && isIncident) {
            if (type == "fail"_L1 || type == "xpass"_L1)
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
    QString adbCommand{"adb"_L1};
    QString makeCommand;
    QString package;
    QString activity;
    QStringList testArgsList;
    QHash<QString, QString> outFiles;
    QStringList amStarttestArgs;
    QString apkPath;
    QString ndkStackPath;
    bool showLogcatOutput = false;
    const QHash<QString, std::function<bool(const QByteArray &)>> checkFiles = {
        {"txt"_L1, checkTxt},
        {"csv"_L1, checkCsv},
        {"xml"_L1, checkXml},
        {"lightxml"_L1, checkLightxml},
        {"xunitxml"_L1, checkJunit},
        {"junitxml"_L1, checkJunit},
        {"teamcity"_L1, checkTeamcity},
        {"tap"_L1, checkTap},
    };
};

static Options g_options;

struct TestInfo
{
    int sdkVersion = -1;
    int pid = -1;

    std::atomic<bool> isPackageInstalled { false };
    std::atomic<bool> isTestRunnerInterrupted { false };
};

static TestInfo g_testInfo;

static bool execCommand(const QString &program, const QStringList &args,
                        QByteArray *output = nullptr, bool verbose = false)
{
    const auto command = program + " "_L1 + args.join(u' ');

    if (verbose && g_options.verbose)
        qDebug("Execute %s.", command.toUtf8().constData());

    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted()) {
        qCritical("Cannot execute command %s.", qPrintable(command));
        return false;
    }

    // If the command is not adb, for example, make or ninja, it can take more that
    // QProcess::waitForFinished() 30 secs, so for that use a higher timeout.
    const int FinishTimeout = program.endsWith("adb"_L1) ? 30000 : g_options.timeoutSecs * 1000;
    if (!process.waitForFinished(FinishTimeout)) {
        qCritical("Execution of command %s timed out.", qPrintable(command));
        return false;
    }

    const auto stdOut = process.readAllStandardOutput();
    if (output)
        output->append(stdOut);

    if (verbose && g_options.verbose)
        qDebug() << stdOut.constData();

    return process.exitCode() == 0;
}

static bool execAdbCommand(const QStringList &args, QByteArray *output = nullptr,
                           bool verbose = true)
{
    return execCommand(g_options.adbCommand, args, output, verbose);
}

static bool execCommand(const QString &command, QByteArray *output = nullptr, bool verbose = true)
{
    auto args = QProcess::splitCommand(command);
    const auto program = args.first();
    args.removeOne(program);
    return execCommand(program, args, output, verbose);
}

static bool parseOptions()
{
    QStringList arguments = QCoreApplication::arguments();
    int i = 1;
    for (; i < arguments.size(); ++i) {
        const QString &argument = arguments.at(i);
        if (argument.compare("--adb"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.adbCommand = arguments.at(++i);
        } else if (argument.compare("--path"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.buildPath = arguments.at(++i);
        } else if (argument.compare("--make"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.makeCommand = arguments.at(++i);
        } else if (argument.compare("--apk"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.apkPath = arguments.at(++i);
        } else if (argument.compare("--activity"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.activity = arguments.at(++i);
        } else if (argument.compare("--skip-install-root"_L1, Qt::CaseInsensitive) == 0) {
            g_options.skipAddInstallRoot = true;
        } else if (argument.compare("--show-logcat"_L1, Qt::CaseInsensitive) == 0) {
            g_options.showLogcatOutput = true;
        } else if (argument.compare("--ndk-stack"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.ndkStackPath = arguments.at(++i);
        } else if (argument.compare("--timeout"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.timeoutSecs = arguments.at(++i).toInt();
        } else if (argument.compare("--help"_L1, Qt::CaseInsensitive) == 0) {
            g_options.helpRequested = true;
        } else if (argument.compare("--verbose"_L1, Qt::CaseInsensitive) == 0) {
            g_options.verbose = true;
        } else if (argument.compare("--"_L1, Qt::CaseInsensitive) == 0) {
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
        g_options.adbCommand += " -s %1"_L1.arg(serial);

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
    qWarning(       "Syntax: %s <options> -- [TESTARGS] \n"
                    "\n"
                    "  Runs an Android test on the default emulator/device or on the one\n"
                    "  specified by \"ANDROID_DEVICE_SERIAL\" environment variable.\n"
                    "\n"
                    "  Mandatory arguments:\n"
                    "    --path <path>: The path where androiddeployqt builds the android package.\n"
                    "\n"
                    "    --apk <apk path>: The test apk path. The apk has to exist already, if it\n"
                    "       does not exist the make command must be provided for building the apk.\n"
                    "\n"
                    "  Optional arguments:\n"
                    "    --make <make cmd>: make command, needed to install the qt library.\n"
                    "       For Qt 6, this can be \"cmake --build . --target <target>_make_apk\".\n"
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
                    "    --help: Displays this information.\n",
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
            if (reader.isStartElement() && reader.name() == "manifest"_L1)
                return reader.attributes().value("package"_L1).toString();
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
            if (reader.isStartElement() && reader.name() == "activity"_L1)
                return reader.attributes().value("android:name"_L1).toString();
        }
    }
    return {};
}

static void setOutputFile(QString file, QString format)
{
    if (file.isEmpty())
        file = "-"_L1;
    if (format.isEmpty())
        format = "txt"_L1;

    g_options.outFiles[format] = file;
}

static bool parseTestArgs()
{
    QRegularExpression oldFormats{"^-(txt|csv|xunitxml|junitxml|xml|lightxml|teamcity|tap)$"_L1};
    QRegularExpression newLoggingFormat{"^(.*),(txt|csv|xunitxml|junitxml|xml|lightxml|teamcity|tap)$"_L1};

    QString file;
    QString logType;
    QStringList unhandledArgs;
    for (int i = 0; i < g_options.testArgsList.size(); ++i) {
        const QString &arg = g_options.testArgsList[i].trimmed();
        if (arg == "--"_L1)
            continue;
        if (arg == "-o"_L1) {
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
                // Use triple literal quotes so that QProcess::splitCommand() in androidjnimain.cpp
                // keeps quotes characters inside the string.
                QString quotedArg = QString(arg).replace("\""_L1, "\\\"\\\"\\\""_L1);
                // Escape single quotes so they don't interfere with the shell command,
                // and so they get passed to the app as single quote inside the string.
                quotedArg.replace("'"_L1, "\'"_L1);
                // Add escaped double quote character so that args with spaces are treated as one.
                unhandledArgs << " \\\"%1\\\""_L1.arg(quotedArg);
            }
        }
    }
    if (g_options.outFiles.isEmpty() || !file.isEmpty() || !logType.isEmpty())
        setOutputFile(file, logType);

    QString testAppArgs;
    for (const auto &format : g_options.outFiles.keys())
        testAppArgs += "-o output.%1,%1 "_L1.arg(format);

    testAppArgs += unhandledArgs.join(u' ').trimmed();
    testAppArgs = "\"%1\""_L1.arg(testAppArgs.trimmed());
    const QString activityName = "%1/%2"_L1.arg(g_options.package).arg(g_options.activity);

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

    g_options.amStarttestArgs = { "shell"_L1, "am"_L1, "start"_L1,
                                  "-n"_L1, activityName,
                                  "-e"_L1, "applicationArguments"_L1, testAppArgs,
                                  testEnvVars
                                   };

    return true;
}

static bool obtainPid() {
    QByteArray output;
    const QStringList psArgs = { "shell"_L1, "ps | grep ' %1'"_L1.arg(g_options.package) };
    if (!execAdbCommand(psArgs, &output, false))
        return false;

    const QList<QByteArray> lines = output.split(u'\n');
    if (lines.size() < 1)
        return false;

    QList<QByteArray> columns = lines.first().simplified().replace(u'\t', u' ').split(u' ');
    if (columns.size() < 3)
        return false;

    if (g_testInfo.pid == -1) {
        bool ok = false;
        int pid = columns.at(1).toInt(&ok);
        if (ok)
            g_testInfo.pid = pid;
    }

    return true;
}

static bool isRunning() {
    QByteArray output;
    const QStringList psArgs = { "shell"_L1, "ps | grep ' %1'"_L1.arg(g_options.package) };
    if (!execAdbCommand(psArgs, &output, false))
        return false;

    return output.indexOf(QLatin1StringView(" " + g_options.package.toUtf8())) > -1;
}

static void waitForStartedAndFinished()
{
    // wait to start and set PID
    QDeadlineTimer startDeadline(10000);
    do {
        if (obtainPid())
            break;
        QThread::msleep(100);
    } while (!startDeadline.hasExpired() && !g_testInfo.isTestRunnerInterrupted.load());

    // Wait to finish
    QDeadlineTimer finishedDeadline(g_options.timeoutSecs * 1000);
    do {
        if (!isRunning())
            break;
        QThread::msleep(250);
    } while (!finishedDeadline.hasExpired() && !g_testInfo.isTestRunnerInterrupted.load());

    if (finishedDeadline.hasExpired())
        qWarning() << "Timed out while waiting for the test to finish";
}

static void obtainSdkVersion()
{
    // SDK version is necessary, as in SDK 23 pidof is broken, so we cannot obtain the pid.
    // Also, Logcat cannot filter by pid in SDK 23, so we don't offer the --show-logcat option.
    QByteArray output;
    const QStringList versionArgs = { "shell"_L1, "getprop"_L1, "ro.build.version.sdk"_L1 };
    execAdbCommand(versionArgs, &output, false);
    bool ok = false;
    int sdkVersion = output.toInt(&ok);
    if (ok)
        g_testInfo.sdkVersion = sdkVersion;
    else
        qCritical() << "Unable to obtain the SDK version of the target.";
}

static bool pullFiles()
{
    bool ret = true;
    QByteArray userId;
    // adb get-current-user command is available starting from API level 26.
    if (g_testInfo.sdkVersion >= 26) {
        const QStringList userIdArgs = {"shell"_L1, "cmd"_L1, "activity"_L1, "get-current-user"_L1};
        if (!execAdbCommand(userIdArgs, &userId, false)) {
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
        const QStringList fullCatArgs = { "shell"_L1, "run-as %1 --user %2 %3"_L1.arg(
                g_options.package, QString::fromUtf8(userId.simplified()), catCmd) };

        QByteArray output;
        if (!execAdbCommand(fullCatArgs, &output, false)) {
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
            qDebug() << output.constData();
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
    QStringList logcatArgs = { "logcat"_L1 };
    if (g_testInfo.sdkVersion <= 23 || g_testInfo.pid == -1)
        logcatArgs << "-t"_L1 << formattedTime;
    else
        logcatArgs << "-d"_L1 << "--pid=%1"_L1.arg(QString::number(g_testInfo.pid));

    QByteArray logcat;
    if (!execAdbCommand(logcatArgs, &logcat, false)) {
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
    const QStringList abiArgs = { "shell"_L1, "getprop"_L1, "ro.product.cpu.abi"_L1 };
    QByteArray abi;
    if (!execAdbCommand(abiArgs, &abi, false)) {
        qWarning() << "Warning: failed to get the device abi, fallback to first libs dir";
        return {};
    }

    return QString::fromUtf8(abi.simplified());
}

void printLogcatCrashBuffer(const QString &formattedTime)
{
    bool useNdkStack = false;
    auto libsPath = "%1/libs/"_L1.arg(g_options.buildPath);

    if (!g_options.ndkStackPath.isEmpty()) {
        QString abi = getDeviceABI();
        if (abi.isEmpty()) {
            QStringList subDirs = QDir(libsPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            if (!subDirs.isEmpty())
                abi = subDirs.first();
        }

        if (!abi.isEmpty()) {
            libsPath += abi;
            useNdkStack = true;
        } else {
            qWarning() << "Warning: failed to get the libs abi, ndk-stack cannot be used.";
        }
    } else {
        qWarning() << "Warning: ndk-stack path not provided and couldn't be deduced "
                      "using the ANDROID_NDK_ROOT environment variable.";
    }

    QProcess adbCrashProcess;
    QProcess ndkStackProcess;

    if (useNdkStack) {
        adbCrashProcess.setStandardOutputProcess(&ndkStackProcess);
        ndkStackProcess.start(g_options.ndkStackPath, { "-sym"_L1, libsPath });
    }

    const QStringList adbCrashArgs = { "logcat"_L1, "-b"_L1, "crash"_L1, "-t"_L1, formattedTime };
    adbCrashProcess.start(g_options.adbCommand, adbCrashArgs);

    if (!adbCrashProcess.waitForStarted()) {
        qCritical() << "Error: failed to run adb logcat crash command.";
        return;
    }

    if (useNdkStack && !ndkStackProcess.waitForStarted()) {
        qCritical() << "Error: failed to run ndk-stack command.";
        return;
    }

    if (!adbCrashProcess.waitForFinished()) {
        qCritical() << "Error: adb command timed out.";
        return;
    }

    if (useNdkStack && !ndkStackProcess.waitForFinished()) {
        qCritical() << "Error: ndk-stack command timed out.";
        return;
    }

    const QByteArray crash = useNdkStack ? ndkStackProcess.readAllStandardOutput()
                                         : adbCrashProcess.readAllStandardOutput();
    if (crash.isEmpty()) {
        qWarning() << "The retrieved crash logcat is empty";
        return;
    }

    qDebug() << "****** Begin logcat crash buffer output ******";
    qDebug().noquote() << crash;
    qDebug() << "****** End logcat crash buffer output ******";
}

static QString getCurrentTimeString()
{
    const QString timeFormat = (g_testInfo.sdkVersion <= 23) ?
            "%m-%d %H:%M:%S.000"_L1 : "%Y-%m-%d %H:%M:%S.%3N"_L1;

    QStringList dateArgs = { "shell"_L1, "date"_L1, "+'%1'"_L1.arg(timeFormat) };
    QByteArray output;
    if (!execAdbCommand(dateArgs, &output, false)) {
        qWarning() << "Date/time adb command failed";
        return {};
    }

    return QString::fromUtf8(output.simplified());
}

static bool uninstallTestPackage()
{
    return execAdbCommand({ "uninstall"_L1, g_options.package }, nullptr);
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
    if (!g_testInfo.isPackageInstalled.load())
        _exit(-1);
    g_testInfo.isTestRunnerInterrupted.store(true);
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
        qCritical() << "It is required to provide a make command with the \"--make\" parameter "
                       "to generate the apk.";
        return 1;
    }
    if (!execCommand(g_options.makeCommand, nullptr, true)) {
        if (!g_options.skipAddInstallRoot) {
            // we need to run make INSTALL_ROOT=path install to install the application file(s) first
            if (!execCommand("%1 INSTALL_ROOT=%2 install"_L1.arg(g_options.makeCommand,
                                        QDir::toNativeSeparators(g_options.buildPath)), nullptr)) {
                return 1;
            }
        } else {
            if (!execCommand(g_options.makeCommand, nullptr))
                return 1;
        }
    }

    if (!QFile::exists(g_options.apkPath)) {
        qCritical("No apk \"%s\" found after running the make command. "
                  "Check the provided path and the make command.",
                  qPrintable(g_options.apkPath));
        return 1;
    }

    obtainSdkVersion();

    QString manifest = g_options.buildPath + "/AndroidManifest.xml"_L1;
    g_options.package = packageNameFromAndroidManifest(manifest);
    if (g_options.activity.isEmpty())
        g_options.activity = activityFromAndroidManifest(manifest);

    // parseTestArgs depends on g_options.package
    if (!parseTestArgs())
        return 1;

    // do not install or run packages while another test is running
    testRunnerLock.acquire();

    const QStringList installArgs = { "install"_L1, "-r"_L1, "-g"_L1, g_options.apkPath };
    g_testInfo.isPackageInstalled.store(execAdbCommand(installArgs, nullptr));
    if (!g_testInfo.isPackageInstalled)
        return 1;

    const QString formattedTime = getCurrentTimeString();

    // start the tests
    bool success = execAdbCommand(g_options.amStarttestArgs, nullptr);

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

    testRunnerLock.release();

    if (g_testInfo.isTestRunnerInterrupted.load()) {
        qCritical() << "The androidtestrunner was interrupted and the was test cleaned up.";
        return 1;
    }

    return success ? 0 : 1;
}
