// Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QtCore/QDeadlineTimer>
#include <QtCore/qdebug.h>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QRegularExpression>
#include <QtCore/QSystemSemaphore>
#include <QtCore/QThread>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QFileInfo>
#include <QtCore/QSysInfo>

#include <atomic>
#include <csignal>
#include <functional>
#include <optional>
#if defined(Q_OS_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

using namespace Qt::StringLiterals;


// QTest-based test processes may exit with up to 127 for normal test failures
static constexpr int HIGHEST_QTEST_EXITCODE = 127;
// Something went wrong in androidtestrunner, in general
static constexpr int EXIT_ERROR = 254;
// More specific exit codes for failures in androidtestrunner:
static constexpr int EXIT_NOEXITCODE = 253; // Failed to transfer exit code from device
static constexpr int EXIT_ANR        = 252; // Android ANR error (Application Not Responding)
static constexpr int EXIT_NORESULTS  = 251; // Failed to transfer result files from device


struct Options
{
    bool helpRequested = false;
    bool verbose = false;
    bool skipAddInstallRoot = false;
    int timeoutSecs = 600; // 10 minutes
    int resultsPullRetries = 3;
    QString buildPath;
    QString manifestPath;
    QString adbCommand{"adb"_L1};
    QString bundletoolPath;
    QString serial;
    QString makeCommand;
    QString package;
    QString activity;
    QStringList permissions;
    QStringList testArgsList;
    QString stdoutFileName;
    QHash<QString, QString> outFiles;
    QStringList amStarttestArgs;
    QString packagePath;
    QString ndkStackPath;
    QList<QStringList> preTestRunAdbCommands;
    bool showLogcatOutput = false;
    std::optional<QProcess> stdoutLogger;
};

static Options g_options;

struct TestInfo
{
    int sdkVersion = -1;
    int pid = -1;
    QString userId;

    std::atomic<bool> isPackageInstalled { false };
    std::atomic<bool> isTestRunnerInterrupted { false };
};

static TestInfo g_testInfo;

// QTest-based processes return 0 if all tests PASSed, or the number of FAILs up to 127.
// Other exitcodes signify abnormal termination and are system-dependent.
static bool isTestExitCodeNormal(const int ec)
{
    return (ec >= 0  &&  ec <= HIGHEST_QTEST_EXITCODE);
}

static bool execCommand(const QString &program, const QStringList &args,
                        QByteArray *output = nullptr, bool verbose = false)
{
    const auto command = program + " "_L1 + args.join(u' ');

    if (verbose && g_options.verbose)
        fprintf(stdout,"Execute %s.\n", command.toUtf8().constData());

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
        fprintf(stdout, "%s\n", stdOut.constData());

    return process.exitCode() == 0;
}

static bool execAdbCommand(const QStringList &args, QByteArray *output = nullptr,
                           bool verbose = true)
{
    if (g_options.serial.isEmpty())
        return execCommand(g_options.adbCommand, args, output, verbose);

    QStringList argsWithSerial = {"-s"_L1, g_options.serial};
    argsWithSerial.append(args);

    return execCommand(g_options.adbCommand, argsWithSerial, output, verbose);
}

static bool execBundletoolCommand(const QStringList &args, QByteArray *output = nullptr,
                                  bool verbose = true)
{
    QString java("java"_L1);
    QStringList argsFull = QStringList() << "-jar"_L1 << g_options.bundletoolPath << args;
    return execCommand(java, argsFull, output, verbose);
}

static void setPackagePath(const QString &path)
{
    if (!g_options.packagePath.isEmpty()) {
        qCritical("Both --aab and --apk options provided. This is not supported.");
        g_options.helpRequested = true;
        return;
    }
    g_options.packagePath = path;
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
        } else if (argument.compare("--bundletool"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.bundletoolPath = arguments.at(++i);
        } else if (argument.compare("--path"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.buildPath = arguments.at(++i);
        } else if (argument.compare("--manifest"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.manifestPath = arguments.at(++i);
        } else if (argument.compare("--make"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.makeCommand = arguments.at(++i);
        } else if (argument.compare("--apk"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                setPackagePath(arguments.at(++i));
        } else if (argument.compare("--aab"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                setPackagePath(arguments.at(++i));
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
        } else if (argument.compare("--pre-test-adb-command"_L1, Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else {
                g_options.preTestRunAdbCommands += QProcess::splitCommand(arguments.at(++i));
            }
        } else if (argument.compare("--"_L1, Qt::CaseInsensitive) == 0) {
            ++i;
            break;
        } else {
            g_options.testArgsList << arguments.at(i);
        }
    }

    if (!g_options.skipAddInstallRoot) {
        // we need to run make INSTALL_ROOT=path install to install the application file(s) first
        g_options.makeCommand = "%1 INSTALL_ROOT=%2 install"_L1
            .arg(g_options.makeCommand)
            .arg(QDir::toNativeSeparators(g_options.buildPath));
    }

    for (;i < arguments.size(); ++i)
        g_options.testArgsList << arguments.at(i);

    if (g_options.helpRequested || g_options.buildPath.isEmpty() || g_options.packagePath.isEmpty())
        return false;

    g_options.serial = qEnvironmentVariable("ANDROID_SERIAL");
    if (g_options.serial.isEmpty())
        g_options.serial = qEnvironmentVariable("ANDROID_DEVICE_SERIAL");

    if (g_options.ndkStackPath.isEmpty()) {
        const QString ndkPath = qEnvironmentVariable("ANDROID_NDK_ROOT");
        const QString ndkStackPath = ndkPath + QDir::separator() + "ndk-stack"_L1;
        if (QFile::exists(ndkStackPath))
            g_options.ndkStackPath = ndkStackPath;
    }

    if (g_options.manifestPath.isEmpty())
        g_options.manifestPath = g_options.buildPath + "/AndroidManifest.xml"_L1;

    return true;
}

static void printHelp()
{
    qWarning("Syntax: %s <options> -- [TESTARGS] \n"
             "\n"
             "  Runs a Qt for Android test on an emulator or a device. Specify a device\n"
             "  using the environment variables ANDROID_SERIAL or ANDROID_DEVICE_SERIAL.\n"
             "  Returns the number of failed tests, -1 on test runner deployment related\n"
             "  failures or zero on success."
             "\n"
             "  Mandatory arguments:\n"
             "    --path <path>: The path where androiddeployqt builds the android package.\n"
             "\n"
             "    --make <make cmd>: make command to create an APK, for example:\n"
             "       \"cmake --build <build-dir> --target <target>_make_apk\".\n"
             "\n"
             "    --apk <apk path>: The test apk path. The apk has to exist already, if it\n"
             "       does not exist the make command must be provided for building the apk.\n"
             "\n"
             "    --aab <aab path>: The test aab path. The aab has to exist already, if it\n"
             "       does not exist the make command must be provided for building the aab.\n"
             "\n"
             "  Optional arguments:\n"
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
             "    --show-logcat: Print Logcat output to stdout. If an ANR occurs during\n"
             "       the test run, logs from the system_server process are included.\n"
             "\n"
             "    --ndk-stack: Path to ndk-stack tool that symbolizes crash stacktraces.\n"
             "       By default, ANDROID_NDK_ROOT env var is used to deduce the tool path.\n"
             "\n"
             "    -- Arguments that will be passed to the test application.\n"
             "\n"
             "    --verbose: Prints out information during processing.\n"
             "\n"
             "    --pre-test-adb-command <command>: call the adb <command> after\n"
             "       installation and before the test run.\n"
             "\n"
             "    --manifest <path>: Custom path to the AndroidManifest.xml.\n"
             "\n"
             "    --bundletool <bundletool path>: The path to Android bundletool.\n"
             "       See https://developer.android.com/tools/bundletool for details.\n"
             "\n"
             "    --help: Displays this information.\n",
             qPrintable(QCoreApplication::arguments().at(0)));
}

static bool processAndroidManifest()
{
    QFile androidManifestXml(g_options.manifestPath);
    if (!androidManifestXml.open(QIODevice::ReadOnly)) {
        qCritical("Unable to read android manifest '%s'", qPrintable(g_options.manifestPath));
        return false;
    }

    QXmlStreamReader reader(&androidManifestXml);
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement())
            continue;

        if (reader.name() == "manifest"_L1)
            g_options.package = reader.attributes().value("package"_L1).toString();
        else if (reader.name() == "activity"_L1 && g_options.activity.isEmpty())
            g_options.activity = reader.attributes().value("android:name"_L1).toString();
        else if (reader.name() == "uses-permission"_L1)
            g_options.permissions.append(reader.attributes().value("android:name"_L1).toString());
    }
    return true;
}

static QStringList queryDangerousPermissions()
{
    QByteArray output;
    const QStringList args({ "shell"_L1, "dumpsys"_L1, "package"_L1, "permissions"_L1 });
    if (!execAdbCommand(args, &output, false)) {
        qWarning("Failed to query permissions via dumpsys");
        return {};
    }

    /*
     * Permissions section from this command look like:
     *
     * Permission [android.permission.INTERNET] (c8cafdc):
     *     sourcePackage=android
     *     uid=1000 gids=[3003] type=0 prot=normal|instant
     *     perm=PermissionInfo{5f5bfbb android.permission.INTERNET}
     *     flags=0x0
     */
     const static QRegularExpression regex("^\\s*Permission\\s+\\[([^\\]]+)\\]\\s+\\(([^)]+)\\):"_L1);
    QStringList dangerousPermissions;
    QString currentPerm;

    const QStringList lines = QString::fromUtf8(output).split(u'\n');
    for (const QString &line : lines) {
        QRegularExpressionMatch match = regex.match(line);
        if (match.hasMatch()) {
            currentPerm = match.captured(1);
            continue;
        }

        if (currentPerm.isEmpty())
            continue;

        int protIndex = line.indexOf("prot="_L1);
        if (protIndex == -1)
            continue;

        QString protectionTypes = line.mid(protIndex + 5).trimmed();
        if (protectionTypes.contains("dangerous"_L1, Qt::CaseInsensitive)) {
            dangerousPermissions.append(currentPerm);
            currentPerm.clear();
        }
    }

    return dangerousPermissions;
}

static void setOutputFile(QString file, QString format)
{
    if (format.isEmpty())
        format = "txt"_L1;

    if ((file.isEmpty() || file == u'-')) {
        if (g_options.outFiles.contains(format)) {
            file = g_options.outFiles.value(format);
        } else {
            file = "stdout.%1"_L1.arg(format);
            g_options.outFiles[format] = file;
        }
        g_options.stdoutFileName = QFileInfo(file).fileName();
    } else {
        g_options.outFiles[format] = file;
    }
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
    for (auto it = g_options.outFiles.constBegin(); it != g_options.outFiles.constEnd(); ++it)
        testAppArgs += "-o %1,%2 "_L1.arg(QFileInfo(it.value()).fileName(), it.key());

    testAppArgs += unhandledArgs.join(u' ').trimmed();
    testAppArgs = "\"%1\""_L1.arg(testAppArgs.trimmed());
    const QString activityName = "%1/%2"_L1.arg(g_options.package).arg(g_options.activity);

    // Pass over any qt or testlib env vars if set
    QString testEnvVars;
    const QStringList envVarsList = QProcessEnvironment::systemEnvironment().toStringList();
    for (const QString &var : envVarsList) {
        if (var.startsWith("QTEST_"_L1) || var.startsWith("QT_"_L1))
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

static int getPid(const QString &package)
{
    QByteArray output;
    const QStringList psArgs = { "shell"_L1, "ps | grep ' %1'"_L1.arg(package) };
    if (!execAdbCommand(psArgs, &output, false))
        return false;

    const QList<QByteArray> lines = output.split(u'\n');
    if (lines.size() < 1)
        return false;

    QList<QByteArray> columns = lines.first().simplified().replace(u'\t', u' ').split(u' ');
    if (columns.size() < 3)
        return false;

    bool ok = false;
    int pid = columns.at(1).toInt(&ok);
    if (ok)
        return pid;

    return -1;
}

static QString runCommandAsUserArgs(const QString &cmd)
{
    return "run-as %1 --user %2 %3"_L1.arg(g_options.package, g_testInfo.userId, cmd);
}

static bool isRunning() {
    if (g_testInfo.pid < 1)
        return false;

    QByteArray output;
    const QStringList psArgs = { "shell"_L1, "ps"_L1, "-p"_L1, QString::number(g_testInfo.pid),
                                 "|"_L1, "grep"_L1, "-o"_L1, " %1$"_L1.arg(g_options.package) };
    bool psSuccess = false;
    for (int i = 1; i <= 3; ++i) {
        psSuccess = execAdbCommand(psArgs, &output, false);
        if (psSuccess)
            break;
        QThread::msleep(250);
    }

    return psSuccess && output.trimmed() == g_options.package.toUtf8();
}

static void waitForStarted()
{
    // wait to start and set PID
    QDeadlineTimer startDeadline(10000);
    do {
        g_testInfo.pid = getPid(g_options.package);
        if (g_testInfo.pid > 0)
            break;
        QThread::msleep(100);
    } while (!startDeadline.hasExpired() && !g_testInfo.isTestRunnerInterrupted.load());
}

static void waitForLoggingStarted()
{
    const QString lsCmd = "ls files/%1"_L1.arg(g_options.stdoutFileName);
    const QStringList adbLsCmd = { "shell"_L1, runCommandAsUserArgs(lsCmd) };

    QDeadlineTimer deadline(5000);
    do {
        if (execAdbCommand(adbLsCmd, nullptr, false))
            break;
        QThread::msleep(100);
    } while (!deadline.hasExpired() && !g_testInfo.isTestRunnerInterrupted.load());
}

static bool setupStdoutLogger()
{
    // Start tail to get results to stdout as soon as they're available
    const QString tailPipeCmd = "tail -n +1 -f files/%1"_L1.arg(g_options.stdoutFileName);
    const QStringList adbTailCmd = { "shell"_L1, runCommandAsUserArgs(tailPipeCmd) };

    g_options.stdoutLogger.emplace();
    g_options.stdoutLogger->setProcessChannelMode(QProcess::ForwardedOutputChannel);
    g_options.stdoutLogger->start(g_options.adbCommand, adbTailCmd);

    if (!g_options.stdoutLogger->waitForStarted()) {
        qCritical() << "Error: failed to run adb command to fetch stdout test results.";
        g_options.stdoutLogger = std::nullopt;
        return false;
    }

    return true;
}

static bool stopStdoutLogger()
{
    if (!g_options.stdoutLogger.has_value()) {
        // In case this ever happens, it setupStdoutLogger() wasn't called, whether
        // that's on purpose or not, return true since what it does is achieved.
        qCritical() << "Trying to stop the stdout logger process while it's been uninitialised";
        return true;
    }

    if (g_options.stdoutLogger->state() == QProcess::NotRunning) {
        // We expect the tail command to be running until we stop it, so if it's
        // not running it might have been terminated outside of the test runner.
        qCritical() << "The stdout logger process was terminated unexpectedly, "
                       "It might have been terminated by an external process";
        return false;
    }

    g_options.stdoutLogger->terminate();

    if (!g_options.stdoutLogger->waitForFinished()) {
        qCritical() << "Error: adb test results tail command timed out.";
        return false;
    }

    return true;
}

static void waitForFinished()
{
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

static QString userId()
{
    // adb get-current-user command is available starting from API level 26.
    QByteArray userId;
    if (g_testInfo.sdkVersion >= 26) {
        const QStringList userIdArgs = {"shell"_L1, "cmd"_L1, "activity"_L1, "get-current-user"_L1};
        if (!execAdbCommand(userIdArgs, &userId, false)) {
            qCritical() << "Error: failed to retrieve the user ID";
            userId.clear();
        }
    }

    if (userId.isEmpty())
        userId = "0";

    return QString::fromUtf8(userId.simplified());
}

static QStringList runningDevices()
{
    QByteArray output;
    execAdbCommand({ "devices"_L1 }, &output, false);

    QStringList devices;
    for (const QByteArray &line : output.split(u'\n')) {
        if (line.contains("\tdevice"_L1))
            devices.append(QString::fromUtf8(line.split(u'\t').first()));
    }

    return devices;
}

static bool pullResults()
{
    for (auto it = g_options.outFiles.constBegin(); it != g_options.outFiles.constEnd(); ++it) {
        const QString filePath = it.value();
        const QString fileName = QFileInfo(filePath).fileName();
        // Get only stdout from cat and get rid of stderr and fail later if the output is empty
        const QString catCmd = "cat files/%1 2> /dev/null"_L1.arg(fileName);
        const QStringList fullCatArgs = { "shell"_L1, runCommandAsUserArgs(catCmd) };

        bool catSuccess = false;
        QByteArray output;

        for (int i = 1; i <= g_options.resultsPullRetries; ++i) {
            catSuccess = execAdbCommand(fullCatArgs, &output, false);
            if (!catSuccess)
                continue;
            else if (!output.isEmpty())
                break;
        }

        if (!catSuccess) {
            qCritical() << "Error: failed to retrieve the test result file %1."_L1.arg(fileName);
            return false;
        }

        if (output.isEmpty()) {
            qCritical() << "Error: the test result file %1 is empty."_L1.arg(fileName);
            return false;
        }

        QFile out{filePath};
        if (!out.open(QIODevice::WriteOnly)) {
            qCritical() << "Error: failed to open %1 to write results to host."_L1.arg(filePath);
            return false;
        }
        out.write(output);
    }

    return true;
}

static QString getAbiLibsPath()
{
    QString libsPath = "%1/libs/"_L1.arg(g_options.buildPath);
    const QStringList abiArgs = { "shell"_L1, "getprop"_L1, "ro.product.cpu.abi"_L1 };
    QByteArray abi;
    if (!execAdbCommand(abiArgs, &abi, false)) {
        QStringList subDirs = QDir(libsPath).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (!subDirs.isEmpty())
            abi = subDirs.first().toUtf8();
    }

    abi = abi.trimmed();
    if (abi.isEmpty())
        qWarning() << "Failed to get the libs abi, falling to host architecture";

    QString hostArch = QSysInfo::currentCpuArchitecture();
    if (hostArch == "x86_64"_L1)
        abi = "arm64-x86_64";
    else if (hostArch == "arm64"_L1)
        abi = "arm64-v8a";
    else if (hostArch == "i386"_L1)
        abi = "x86";
    else
        abi = "armeabi-v7a";

    return libsPath + QString::fromUtf8(abi);
}

void printLogcatCrash(const QByteArray &logcat)
{
    // No crash report, do nothing
    if (logcat.isEmpty())
        return;

    QByteArray crashLogcat(logcat);
    if (!g_options.ndkStackPath.isEmpty()) {
        QProcess ndkStackProc;
        ndkStackProc.start(g_options.ndkStackPath, { "-sym"_L1, getAbiLibsPath() });

        if (ndkStackProc.waitForStarted()) {
            ndkStackProc.write(crashLogcat);
            ndkStackProc.closeWriteChannel();

            if (ndkStackProc.waitForReadyRead())
                crashLogcat = ndkStackProc.readAllStandardOutput();

            ndkStackProc.terminate();
            if (!ndkStackProc.waitForFinished())
                qCritical() << "Error: ndk-stack command timed out.";
        } else {
            qCritical() << "Error: failed to run ndk-stack command.";
            return;
        }
    } else {
        qWarning() << "Warning: ndk-stack path not provided and couldn't be deduced "
                      "using the ANDROID_NDK_ROOT environment variable.";
    }

    if (!crashLogcat.startsWith("********** Crash dump"))
        qDebug() << "[androidtestrunner] ********** BEGIN crash dump **********";
    qDebug().noquote() << crashLogcat.trimmed();
    qDebug() << "[androidtestrunner] ********** END crash dump **********";
}

void analyseLogcat(const QString &timeStamp, int *exitCode)
{
    QStringList logcatArgs = { "shell"_L1, "logcat"_L1, "-t"_L1, "'%1'"_L1.arg(timeStamp),
                               "-v"_L1, "brief"_L1 };

    const bool useColor = qEnvironmentVariable("QTEST_ENVIRONMENT") != "ci"_L1;
    if (useColor)
        logcatArgs << "-v"_L1 << "color"_L1;

    QByteArray logcat;
    if (!execAdbCommand(logcatArgs, &logcat, false)) {
        qCritical() << "Error: failed to fetch logcat of the test";
        return;
    }

    if (logcat.isEmpty()) {
        qWarning() << "The retrieved logcat is empty";
        return;
    }

    const QByteArray crashMarker("*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***");
    int crashMarkerIndex = logcat.indexOf(crashMarker);
    QByteArray crashLogcat;

    if (crashMarkerIndex != -1) {
        crashLogcat = logcat.mid(crashMarkerIndex);
        logcat = logcat.left(crashMarkerIndex);
    }

    // Check for ANRs
    const bool anrOccurred = logcat.contains("ANR in %1"_L1.arg(g_options.package).toUtf8());
    if (anrOccurred) {
        // Rather improbable, but if the test managed to return a non-crash exitcode then overwrite
        // it to signify that something blew up. Same if we didn't manage to collect an exit code.
        // Preserve all other exitcodes, they might be useful crash information from the device.
        if (isTestExitCodeNormal(*exitCode) || *exitCode == EXIT_NOEXITCODE)
            *exitCode = EXIT_ANR;
        qCritical("[androidtestrunner] An ANR has occurred while running the test '%s';"
                  " consult logcat for additional logs from the system_server process",
                  qPrintable(g_options.package));
    }

    int systemServerPid = getPid("system_server"_L1);

    static const QRegularExpression logcatRegEx{
        "(?:^\\x1B\\[[0-9]+m)?" // color
        "(\\w)/"                // message type  1. capture
        ".*"                    // source
        "(\\(\\s*\\d*\\)):"     // pid           2. capture
        "\\s*"
        ".*"                    // message
        "(?:\\x1B\\[[0-9]+m)?"  // color
        "[\\n\\r]*$"_L1
    };

    QByteArrayList testLogcat;
    for (const QByteArray &line : logcat.split(u'\n')) {
        QRegularExpressionMatch match = logcatRegEx.match(QString::fromUtf8(line));
        if (match.hasMatch()) {
            const QString msgType = match.captured(1);
            const QString pidStr = match.captured(2);
            const int capturedPid = pidStr.mid(1, pidStr.size() - 2).trimmed().toInt();
            if (capturedPid == g_testInfo.pid || msgType == u'F')
                testLogcat.append(line);
            else if (anrOccurred && capturedPid == systemServerPid)
                testLogcat.append(line);
        } else {
            // If we can't match then just print everything
            testLogcat.append(line);
        }
    }

    // If we have an unpredictable exitcode, possibly a crash, attempt to print both logcat and the
    // crash buffer which includes the crash stacktrace that is not included in the default logcat.
    const bool testCrashed = (   !isTestExitCodeNormal(*exitCode)
                              && !g_testInfo.isTestRunnerInterrupted.load());
    if (testCrashed) {
        qDebug() << "[androidtestrunner] ********** BEGIN logcat dump **********";
        qDebug().noquote() << testLogcat.join(u'\n').trimmed();
        qDebug() << "[androidtestrunner] ********** END logcat dump **********";

        if (!crashLogcat.isEmpty())
            printLogcatCrash(crashLogcat);
    }
}

static QString getCurrentTimeString()
{
    const QString timeFormat = (g_testInfo.sdkVersion <= 23) ?
            "%m-%d %H:%M:%S.000"_L1 : "%Y-%m-%d %H:%M:%S.%3N"_L1;

    QStringList dateArgs = { "shell"_L1, "date"_L1, "+'%1'"_L1.arg(timeFormat) };
    QByteArray output;
    if (!execAdbCommand(dateArgs, &output, false)) {
        qWarning() << "[androidtestrunner] ERROR in command: adb shell date";
        return {};
    }

    return QString::fromUtf8(output.simplified());
}

static int testExitCode()
{
    QByteArray exitCodeOutput;
    const QString exitCodeCmd = "cat files/qtest_last_exit_code 2> /dev/null"_L1;
    if (!execAdbCommand({ "shell"_L1, runCommandAsUserArgs(exitCodeCmd) }, &exitCodeOutput, false)) {
        qCritical() << "[androidtestrunner] ERROR in command: adb shell cat files/qtest_last_exit_code";
        return EXIT_NOEXITCODE;
    }
    qDebug() << "[androidtestrunner] Test exitcode: " << exitCodeOutput;

    bool ok;
    int exitCode = exitCodeOutput.toInt(&ok);

    return ok ? exitCode : EXIT_NOEXITCODE;
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
        _exit(EXIT_ERROR);
    g_testInfo.isTestRunnerInterrupted.store(true);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    QCoreApplication a(argc, argv);
    if (!parseOptions()) {
        printHelp();
        return EXIT_ERROR;
    }

    if (g_options.makeCommand.isEmpty()) {
        qCritical() << "It is required to provide a make command with the \"--make\" parameter "
                       "to generate the apk.";
        return EXIT_ERROR;
    }

    QByteArray buildOutput;
    if (!execCommand(g_options.makeCommand, &buildOutput, true)) {
        qCritical("The APK build command \"%s\" failed\n\n%s",
            qPrintable(g_options.makeCommand), buildOutput.constData());
        return EXIT_ERROR;
    }

    if (!QFile::exists(g_options.packagePath)) {
        qCritical("No apk \"%s\" found after running the make command. "
                  "Check the provided path and the make command.",
                  qPrintable(g_options.packagePath));
        return EXIT_ERROR;
    }

    const QStringList devices = runningDevices();
    if (devices.isEmpty()) {
        qCritical("No connected devices or running emulators can be found.");
        return EXIT_ERROR;
    } else if (!g_options.serial.isEmpty() && !devices.contains(g_options.serial)) {
        qCritical("No connected device or running emulator with serial '%s' can be found.",
                  qPrintable(g_options.serial));
        return EXIT_ERROR;
    }

    obtainSdkVersion();

    g_testInfo.userId = userId();

    if (!QFile::exists(g_options.manifestPath)) {
        qCritical("Unable to find '%s'.", qPrintable(g_options.manifestPath));
        return EXIT_ERROR;
    }

    if (!processAndroidManifest())
        return EXIT_ERROR;

    if (g_options.package.isEmpty()) {
        qCritical("Unable to get package name for '%s'", qPrintable(g_options.packagePath));
        return EXIT_ERROR;
    }

    // parseTestArgs depends on g_options.package
    if (!parseTestArgs())
        return EXIT_ERROR;

    // do not install or run packages while another test is running
    testRunnerLock.acquire();

    if (g_options.packagePath.endsWith(".apk"_L1)) {
        const QStringList installArgs = { "install"_L1, "-r"_L1, g_options.packagePath };
        g_testInfo.isPackageInstalled.store(execAdbCommand(installArgs, nullptr));
        if (!g_testInfo.isPackageInstalled)
            return EXIT_ERROR;
    } else if (g_options.packagePath.endsWith(".aab"_L1)) {
        QFileInfo aab(g_options.packagePath);
        const auto apksFilePath = aab.absoluteDir().absoluteFilePath(aab.baseName() + ".apks"_L1);
        if (!execBundletoolCommand({ "build-apks"_L1, "--bundle"_L1, g_options.packagePath,
                                     "--output"_L1, apksFilePath, "--local-testing"_L1,
                                     "--overwrite"_L1 }))
            return EXIT_ERROR;

        if (!execBundletoolCommand({ "install-apks"_L1, "--apks"_L1, apksFilePath }))
            return EXIT_ERROR;
    }

    const QStringList dangerousPermisisons = queryDangerousPermissions();
    for (const auto &permission : g_options.permissions) {
        if (!dangerousPermisisons.contains(permission))
            continue;

        if (!execAdbCommand({ "shell"_L1, "pm"_L1, "grant"_L1, g_options.package, permission },
                            nullptr)) {
            qWarning("Unable to grant '%s' to '%s'. Probably the Android version mismatch.",
                        qPrintable(permission), qPrintable(g_options.package));
        }
    }

    // Call additional adb command if set after installation and before starting the test
    for (const auto &command : g_options.preTestRunAdbCommands) {
        QByteArray output;
        if (!execAdbCommand(command, &output)) {
            qCritical("The pre test ADB command \"%s\" failed with output:\n%s",
                  qUtf8Printable(command.join(u' ')), output.constData());
            return EXIT_ERROR;
        }
    }

    // Pre test start
    const QString formattedStartTime = getCurrentTimeString();

    // Start the test
    if (!execAdbCommand(g_options.amStarttestArgs, nullptr))
        return EXIT_ERROR;

    waitForStarted();
    waitForLoggingStarted();

    if (!setupStdoutLogger())
        return EXIT_ERROR;

    waitForFinished();

    // Post test run
    if (!stopStdoutLogger())
        return EXIT_ERROR;

    int exitCode = testExitCode();

    if (g_options.showLogcatOutput)
        analyseLogcat(formattedStartTime, &exitCode);

    const bool pullRes = pullResults();
    if (!pullRes && isTestExitCodeNormal(exitCode))
        exitCode = EXIT_NORESULTS;

    if (!uninstallTestPackage())
        return EXIT_ERROR;

    testRunnerLock.release();

    if (g_testInfo.isTestRunnerInterrupted.load()) {
        qCritical() << "The androidtestrunner was interrupted and the was test cleaned up.";
        return EXIT_ERROR;
    }

    return exitCode;
}
