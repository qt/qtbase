/****************************************************************************
**
** Copyright (C) 2019 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#include <QCoreApplication>
#include <QDir>
#include <QHash>
#include <QRegularExpression>
#include <QSystemSemaphore>
#include <QXmlStreamReader>

#include <algorithm>
#include <chrono>
#include <functional>
#include <thread>

#ifdef Q_CC_MSVC
#define popen _popen
#define QT_POPEN_READ "rb"
#define pclose _pclose
#else
#define QT_POPEN_READ "r"
#endif

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
    if (data.indexOf(QLatin1String("\nFAIL!  : ")) >= 0)
        return false;
    if (data.indexOf(QLatin1String("\nXPASS  : ")) >= 0)
        return false;
    // Look for "********* Finished testing of tst_QTestName *********"
    static const QRegularExpression testTail(QLatin1String("\\*+ +Finished testing of .+ +\\*+"));
    return testTail.match(QLatin1String(data)).hasMatch();
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
    return lines.last().startsWith(QLatin1String("##teamcity[testSuiteFinished "));
}

static bool checkTap(const QByteArray &data) {
    // This will still report blacklisted fails because QTest with TAP
    // is not putting any data about that.
    if (data.indexOf("\nnot ok ") >= 0)
        return false;

    static const QRegularExpression testTail(QLatin1String("ok [0-9]* - cleanupTestCase\\(\\)"));
    return testTail.match(QLatin1String(data)).hasMatch();
}

struct Options
{
    bool helpRequested = false;
    bool verbose = false;
    bool skipAddInstallRoot = false;
    std::chrono::seconds timeout{480}; // 8 minutes
    QString buildPath;
    QString adbCommand{QStringLiteral("adb")};
    QString makeCommand;
    QString package;
    QString activity;
    QStringList testArgsList;
    QHash<QString, QString> outFiles;
    QString testArgs;
    QString apkPath;
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

// Copy-pasted from qmake/library/ioutil.cpp
inline static bool hasSpecialChars(const QString &arg, const uchar (&iqm)[16])
{
    for (int x = arg.length() - 1; x >= 0; --x) {
        ushort c = arg.unicode()[x].unicode();
        if ((c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7))))
            return true;
    }
    return false;
}

static QString shellQuoteUnix(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    if (!arg.length())
        return QStringLiteral("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        ret.replace(QLatin1Char('\''), QStringLiteral("'\\''"));
        ret.prepend(QLatin1Char('\''));
        ret.append(QLatin1Char('\''));
    }
    return ret;
}

static QString shellQuoteWin(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars "&()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x45, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };

    if (!arg.length())
        return QStringLiteral("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        // Quotes are escaped and their preceding backslashes are doubled.
        // It's impossible to escape anything inside a quoted string on cmd
        // level, so the outer quoting must be "suspended".
        ret.replace(QRegularExpression(QStringLiteral("(\\\\*)\"")), QStringLiteral("\"\\1\\1\\^\"\""));
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

static QString shellQuote(const QString &arg)
{
    if (QDir::separator() == QLatin1Char('\\'))
        return shellQuoteWin(arg);
    else
        return shellQuoteUnix(arg);
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
        } else if (argument.compare(QStringLiteral("--timeout"), Qt::CaseInsensitive) == 0) {
            if (i + 1 == arguments.size())
                g_options.helpRequested = true;
            else
                g_options.timeout = std::chrono::seconds{arguments.at(++i).toInt()};
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
    return true;
}

static void printHelp()
{//                 "012345678901234567890123456789012345678901234567890123456789012345678901"
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
                    "    --timeout <seconds>: Timeout to run the test. Default is 5 minutes.\n"
                    "\n"
                    "    --skip-install-root: Do not append INSTALL_ROOT=... to the make command.\n"
                    "\n"
                    "    --show-logcat: Print Logcat output to stdout.\n"
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

    g_options.testArgs = QStringLiteral("shell am start -e applicationArguments \"%1\" -n %2/%3")
            .arg(shellQuote(g_options.testArgs.trimmed()))
            .arg(g_options.package)
            .arg(g_options.activity);

    return true;
}

static bool isRunning() {
    QByteArray output;
    if (!execCommand(QStringLiteral("%1 shell \"ps | grep ' %2'\"").arg(g_options.adbCommand,
                                                                        shellQuote(g_options.package)), &output)) {

        return false;
    }
    return output.indexOf(QLatin1String(" " + g_options.package.toUtf8())) > -1;
}

static bool waitToFinish()
{
    using clock = std::chrono::system_clock;
    auto start = clock::now();
    // wait to start
    while (!isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if ((clock::now() - start) > std::chrono::seconds{10})
            return false;
    }

    if (g_options.sdkVersion > 23) { // pidof is broken in SDK 23, non-existent before
        QByteArray output;
        const QString command(QStringLiteral("%1 shell pidof -s %2")
                                      .arg(g_options.adbCommand, shellQuote(g_options.package)));
        execCommand(command, &output, g_options.verbose);
        bool ok = false;
        int pid = output.toInt(&ok); // If we got more than one pid, fail.
        if (ok) {
            g_options.pid = pid;
        } else {
            fprintf(stderr,
                    "Unable to obtain the PID of the running unit test. Command \"%s\" "
                    "returned \"%s\"\n",
                    command.toUtf8().constData(), output.constData());
            fflush(stderr);
        }
    }

    // Wait to finish
    while (isRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        if ((clock::now() - start) > g_options.timeout)
            return false;
    }
    return true;
}

static void obtainSDKVersion()
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
    for (auto it = g_options.outFiles.constBegin(); it != g_options.outFiles.end(); ++it) {
        // Get only stdout from cat and get rid of stderr and fail later if the output is empty
        const QString catCmd = QStringLiteral("cat files/output.%1 2> /dev/null").arg(it.key());

        QByteArray output;
        if (!execCommand(QStringLiteral("%1 shell 'run-as %2 %3'")
                         .arg(g_options.adbCommand, g_options.package, catCmd), &output)) {
            // Cannot find output file. Check in path related to current user
            QByteArray userId;
            execCommand(QStringLiteral("%1 shell cmd activity get-current-user")
                        .arg(g_options.adbCommand), &userId);
            const QString userIdSimplified(QString::fromUtf8(userId).simplified());
            if (!execCommand(QStringLiteral("%1 shell 'run-as %2 --user %3 %4'")
                        .arg(g_options.adbCommand, g_options.package, userIdSimplified, catCmd),
                         &output)) {
                return false;
            }
        }

        if (output.isEmpty()) {
            fprintf(stderr, "Failed to get the test output from the target. Either the output "
                            "is empty or androidtestrunner failed to retrieve it.\n");
            return false;
        }

        auto checkerIt = g_options.checkFiles.find(it.key());
        ret = ret && checkerIt != g_options.checkFiles.end() && checkerIt.value()(output);
        if (it.value() == QStringLiteral("-")){
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

struct RunnerLocker
{
    RunnerLocker()
    {
        runner.acquire();
    }
    ~RunnerLocker()
    {
        runner.release();
    }
    QSystemSemaphore runner{QStringLiteral("androidtestrunner"), 1, QSystemSemaphore::Open};
};

int main(int argc, char *argv[])
{
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

    obtainSDKVersion();

    RunnerLocker lock; // do not install or run packages while another test is running
    if (!execCommand(QStringLiteral("%1 install -r -g %2")
                        .arg(g_options.adbCommand, g_options.apkPath), nullptr, g_options.verbose)) {
        return 1;
    }

    QString manifest = g_options.buildPath + QStringLiteral("/AndroidManifest.xml");
    g_options.package = packageNameFromAndroidManifest(manifest);
    if (g_options.activity.isEmpty())
        g_options.activity = activityFromAndroidManifest(manifest);

    // parseTestArgs depends on g_options.package
    if (!parseTestArgs())
        return 1;

    // start the tests
    bool res = execCommand(QStringLiteral("%1 %2").arg(g_options.adbCommand, g_options.testArgs),
                           nullptr, g_options.verbose)
            && waitToFinish();

    // get logcat output
    if (res && g_options.showLogcatOutput) {
        if (g_options.sdkVersion <= 23) {
            fprintf(stderr, "Cannot show logcat output on Android 23 and below.\n");
            fflush(stderr);
        } else if (g_options.pid > 0) {
            fprintf(stdout, "Logcat output:\n");
            res &= execCommand(QStringLiteral("%1 logcat -d --pid=%2")
                                       .arg(g_options.adbCommand)
                                       .arg(g_options.pid),
                               nullptr, true);
            fprintf(stdout, "End Logcat output.\n");
        }
    }

    if (res)
        res &= pullFiles();
    res &= execCommand(QStringLiteral("%1 uninstall %2").arg(g_options.adbCommand, g_options.package),
                       nullptr, g_options.verbose);
    fflush(stdout);
    return res ? 0 : 1;
}
