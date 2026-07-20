// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qcoreapplication.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qprocess.h>
#include <QtCore/qthread.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#if QT_CONFIG(systemsemaphore)
#  include <QtCore/qsystemsemaphore.h>
#  include <QtCore/qtipccommon.h>
#endif

#include <atomic>
#include <csignal>
#include <cstdio>

using namespace Qt::StringLiterals;

// HarmonyOS sandbox: the app's writable /data/storage/el2/base/files/ maps to
// /data/app/el2/100/base/<bundleName>/files/ in the hdc shell namespace, which
// shell can read but not write (SELinux). "tail -f" / inotify are blocked on
// that path too, so stdout is streamed via a device-side wc/tail polling loop
// that uses only plain read() syscalls.

// QTest normal exit code range is 0-127; use 251-254 for runner-level failures.
static constexpr int EXIT_ERROR = 254;
static constexpr int EXIT_NOEXITCODE = 253;
static constexpr int EXIT_CRASH = 252;
static constexpr int EXIT_TIMEOUT = 251;

// --device / QT_HARMONYOS_DEVICE, prepended as `-t <key>` to every hdc call.
static QString g_hdcConnectKey;

static QString shellSingleQuote(const QString &value)
{
    QString escaped = value;
    escaped.replace(u"'"_s, uR"('\'')"_s);
    return u"'"_s + escaped + u"'"_s;
}

static QString runHdc(const QString &hdcPath, const QStringList &args,
                      bool printOnFailure = false)
{
    QProcess proc;
    QStringList allArgs;
    if (!g_hdcConnectKey.isEmpty())
        allArgs << u"-t"_s << g_hdcConnectKey;
    allArgs += args;
    proc.start(hdcPath, allArgs);
    if (!proc.waitForStarted(5000)) {
        if (printOnFailure)
            fprintf(stderr, "harmonyostestrunner: failed to start hdc: %s\n",
                    qPrintable(hdcPath));
        return {};
    }
    proc.waitForFinished(30000);
    if (printOnFailure) {
        const QByteArray err = proc.readAllStandardError();
        if (!err.isEmpty())
            fprintf(stderr, "hdc %s stderr: %s\n", qPrintable(allArgs.join(u' ')),
                    err.constData());
    }
    return QString::fromUtf8(proc.readAllStandardOutput());
}

#if QT_CONFIG(systemsemaphore)
struct TestRunnerSystemSemaphore
{
    explicit TestRunnerSystemSemaphore(const QString &key)
        : nativeKey(QSystemSemaphore::platformSafeKey(key)),
          semaphore(nativeKey, 1, QSystemSemaphore::Open)
    {}
    ~TestRunnerSystemSemaphore() { release(); }

    // Acquire with a 30 s deadline: if a previous runner died -9 without
    // releasing, reset the semaphore via Create-mode and retry.
    void acquire()
    {
        std::atomic<bool> acquireResult { false };
        QThread *worker = QThread::create([this, &acquireResult]() {
            acquireResult.store(semaphore.acquire());
        });
        worker->start();
        if (!worker->wait(30000)) {
            fprintf(stderr, "harmonyostestrunner: semaphore stuck (previous runner "
                    "may have crashed) — resetting\n");
            {
                QSystemSemaphore reset{ nativeKey, 1, QSystemSemaphore::Create };
            } // destructor unblocks the worker
            worker->wait(5000);
        }
        delete worker;
        isAcquired.store(acquireResult.load());
    }

    void release()
    {
        bool expected = true;
        if (isAcquired.compare_exchange_strong(expected, false))
            isAcquired.store(!semaphore.release());
    }

    std::atomic<bool> isAcquired { false };
    QNativeIpcKey nativeKey;
    QSystemSemaphore semaphore;
};

static TestRunnerSystemSemaphore *g_runnerLock = nullptr;

static QString runnerLockKey(const QString &deviceKey, const QString &bundleName)
{
    const QString device = deviceKey.isEmpty() ? u"local"_s : deviceKey;
    return u"harmonyostestrunner_"_s + device + u'_' + bundleName;
}
#endif // QT_CONFIG(systemsemaphore)

static std::atomic<bool> g_interrupted { false };

static void sigHandler(int sig)
{
    std::signal(sig, SIG_DFL);
    // Not async-signal-safe; best effort so Ctrl-C doesn't strand the semaphore.
    // The next runner's 30 s deadline resets it anyway.
#if QT_CONFIG(systemsemaphore)
    if (g_runnerLock)
        g_runnerLock->release();
#endif
    g_interrupted.store(true);
}

static bool isProcessAlive(const QString &hdc, const QString &bundleName)
{
    return !runHdc(hdc, {u"shell"_s, u"pidof"_s, bundleName}).trimmed().isEmpty();
}

// Without this, OHOS may deliver the new test's Want to the still-dying
// previous process via onNewWant instead of creating a fresh one.
static void waitForProcessDeath(const QString &hdc, const QString &bundleName,
                                int timeoutSecs = 15)
{
    const int pollMs = 200;
    const int maxIterations = (timeoutSecs * 1000) / pollMs;
    for (int i = 0; i < maxIterations; ++i) {
        if (g_interrupted.load())
            return;
        if (!isProcessAlive(hdc, bundleName))
            return;
        QThread::msleep(pollMs);
    }
    fprintf(stderr, "harmonyostestrunner: warning: bundle process still alive after %d s "
            "force-stop wait — proceeding anyway\n", timeoutSecs);
}

static bool waitForProcessStart(const QString &hdc, const QString &bundleName,
                                const QString &shellExitCodePath, int timeoutSecs = 30)
{
    const int pollMs = 250;
    const int maxIterations = (timeoutSecs * 1000) / pollMs;
    for (int i = 0; i < maxIterations; ++i) {
        if (g_interrupted.load())
            return false;
        if (isProcessAlive(hdc, bundleName))
            return true;
        // Fast test may have finished before pidof could see it.
        const QString exitContent = runHdc(hdc, {u"shell"_s, u"cat"_s, shellExitCodePath});
        bool ok = false;
        exitContent.trimmed().toInt(&ok);
        if (ok)
            return true;
        QThread::msleep(pollMs);
    }
    return false;
}

static bool waitForStdoutFile(const QString &hdc, const QString &shellStdoutPath,
                              int timeoutSecs = 10)
{
    const int pollMs = 100;
    const int maxIterations = (timeoutSecs * 1000) / pollMs;
    for (int i = 0; i < maxIterations; ++i) {
        if (g_interrupted.load())
            return false;
        const QString out = runHdc(hdc, {u"shell"_s, u"cat"_s, shellStdoutPath});
        if (!out.contains(u"No such file or directory"_s))
            return true;
        QThread::msleep(pollMs);
    }
    return false;
}

// wc -c + tail -c +N runs device-side; both use plain read() syscalls (SELinux
// permits, unlike inotify), and one persistent hdc connection avoids per-poll
// reconnect overhead.
static bool setupStdoutLogger(QProcess &stdoutLogger, const QString &hdc,
                              const QString &shellStdoutPath)
{
    const QString loop =
        u"sz=0; f='"_s + shellStdoutPath
        + u"'; while true; do"
           " new=$(wc -c < \"$f\" 2>/dev/null);"
           " if [ \"${new:-0}\" -gt \"$sz\" ]; then"
           " tail -c +$((sz+1)) \"$f\" 2>/dev/null; sz=$new;"
           " fi; sleep 0.1; done"_s;

    // SeparateChannels so we can scan stdout for PASS/FAIL while forwarding it.
    stdoutLogger.setProcessChannelMode(QProcess::SeparateChannels);
    QStringList args;
    if (!g_hdcConnectKey.isEmpty())
        args << u"-t"_s << g_hdcConnectKey;
    args << u"shell"_s << loop;
    stdoutLogger.start(hdc, args);
    return stdoutLogger.waitForStarted(5000);
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT,  sigHandler);
    std::signal(SIGTERM, sigHandler);

    QCoreApplication app(argc, argv);
    app.setApplicationName(u"harmonyostestrunner"_s);
    app.setApplicationVersion(QString::fromLatin1(QT_VERSION_STR));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        u"Runs a single Qt auto test from an installed HarmonyOS test bundle HAP."_s);
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument(
        u"test-binary"_s,
        u"Path to the test shared library (e.g. /path/to/libtst_qobject.so)"_s);

    QCommandLineOption bundleNameOpt(
        u"bundle-name"_s,
        u"HarmonyOS bundle name of the installed test HAP (env: QT_HARMONYOS_BUNDLE_NAME)"_s,
        u"name"_s,
        qEnvironmentVariable("QT_HARMONYOS_BUNDLE_NAME", u"org.qtproject.autotests"_s));
    parser.addOption(bundleNameOpt);

    QCommandLineOption abilityNameOpt(
        u"ability-name"_s,
        u"HarmonyOS ability name inside the test HAP (env: QT_HARMONYOS_ABILITY_NAME)"_s,
        u"name"_s,
        qEnvironmentVariable("QT_HARMONYOS_ABILITY_NAME", u"QAbility"_s));
    parser.addOption(abilityNameOpt);

    QCommandLineOption hdcOpt(
        u"hdc"_s,
        u"Path to the hdc tool (env: QT_HARMONYOS_HDC)"_s,
        u"path"_s,
        qEnvironmentVariable("QT_HARMONYOS_HDC", u"hdc"_s));
    parser.addOption(hdcOpt);

    QCommandLineOption timeoutOpt(
        u"timeout"_s,
        u"Seconds to wait for a test to complete before aborting (env: QT_HARMONYOS_TEST_TIMEOUT)"_s,
        u"seconds"_s,
        qEnvironmentVariable("QT_HARMONYOS_TEST_TIMEOUT", u"300"_s));
    parser.addOption(timeoutOpt);

    QCommandLineOption noProgressTimeoutOpt(
        u"no-progress-timeout"_s,
        u"Seconds without a PASS/FAIL test case result before declaring the test hung "
        u"(env: QT_HARMONYOS_NO_PROGRESS_TIMEOUT, 0 = disabled)"_s,
        u"seconds"_s,
        qEnvironmentVariable("QT_HARMONYOS_NO_PROGRESS_TIMEOUT", u"60"_s));
    parser.addOption(noProgressTimeoutOpt);

    QCommandLineOption deviceOpt(
        u"device"_s,
        u"hdc connect key (-t) for the target device — required when multiple devices "
        u"are attached (env: QT_HARMONYOS_DEVICE)"_s,
        u"key"_s,
        qEnvironmentVariable("QT_HARMONYOS_DEVICE"));
    parser.addOption(deviceOpt);

    parser.process(app);

    g_hdcConnectKey = parser.value(deviceOpt);

    const QStringList positional = parser.positionalArguments();
    if (positional.isEmpty()) {
        fprintf(stderr, "harmonyostestrunner: no test binary specified\n");
        parser.showHelp(EXIT_ERROR);
    }

    const QString testBinaryPath = positional.first();
    const QString testLibName = QFileInfo(testBinaryPath).fileName();
    // Extra positionals (test function names, -v2, etc.) forwarded to the test.
    const QStringList testArgs = positional.mid(1);
    const QString bundleName = parser.value(bundleNameOpt);
    const QString abilityName = parser.value(abilityNameOpt);
    const QString hdc = parser.value(hdcOpt);
    const int timeoutSecs = parser.value(timeoutOpt).toInt();
    const int noProgressTimeoutSecs = parser.value(noProgressTimeoutOpt).toInt();

    // Unique per-run ID: shell can't delete files from the app sandbox (SELinux),
    // so uniqueness is the only way to avoid matching stale files.
    const QString runId = QString::number(QDateTime::currentMSecsSinceEpoch());

    const QString appBase = u"/data/storage/el2/base/files"_s;
    const QString appStdoutPath   = appBase + u"/qt_stdout_"_s   + runId + u".txt"_s;
    const QString appExitCodePath = appBase + u"/qt_exitcode_"_s + runId + u".txt"_s;

    const QString shellBase = u"/data/app/el2/100/base/"_s + bundleName + u"/files"_s;
    const QString shellStdoutPath   = shellBase + u"/qt_stdout_"_s   + runId + u".txt"_s;
    const QString shellExitCodePath = shellBase + u"/qt_exitcode_"_s + runId + u".txt"_s;

    const QString bundleCheckOutput =
        runHdc(hdc, {u"shell"_s, u"bm"_s, u"dump"_s, u"-n"_s, bundleName});
    if (bundleCheckOutput.contains(u"error"_s, Qt::CaseInsensitive)
        || bundleCheckOutput.trimmed().isEmpty()) {
        fprintf(stderr,
                "harmonyostestrunner: bundle '%s' is not installed on the device.\n"
                "  Build and sign the test HAP, then install it with:\n"
                "    hdc install <path/to/autotests-signed.hap>\n",
                qPrintable(bundleName));
        return EXIT_ERROR;
    }

#if QT_CONFIG(systemsemaphore)
    TestRunnerSystemSemaphore runnerLock(runnerLockKey(g_hdcConnectKey, bundleName));
    g_runnerLock = &runnerLock;
    runnerLock.acquire();
#endif

    runHdc(hdc, {u"shell"_s, u"aa"_s, u"force-stop"_s, bundleName});
    waitForProcessDeath(hdc, bundleName);

    // --ps for string want.parameters, --pb for boolean. Do NOT use -e: that
    // adds entities (no values) and makes QTest see an empty argv[1].
    QStringList aaStartArgs = {
        u"shell"_s, u"aa"_s, u"start"_s,
        u"-b"_s, bundleName,
        u"-a"_s, abilityName,
        u"--ps"_s, u"io.qt.appSharedLibNameOverride"_s, testLibName,
        u"--ps"_s, u"io.qt.debug.redirectedStdoutPath"_s, appStdoutPath,
        u"--ps"_s, u"io.qt.debug.exitCodePath"_s, appExitCodePath,
        // Without this the platform pushes an empty argv[1], which QTest reads
        // as a test function name and fails with "Function not found: ".
        u"--pb"_s, u"io.qt.useUriAsArg"_s, u"false"_s,
        // Keep main alive across window destroy/create for visual tests.
        u"--pb"_s, u"io.qt.useDefaultUiAbilityInstanceInQt"_s, u"false"_s,
    };

    aaStartArgs << u"--pb"_s << u"io.qt.watchdogEnabled"_s << u"false"_s;

    if (!testArgs.isEmpty()) {
        const QString json = QString::fromUtf8(
            QJsonDocument(QJsonArray::fromStringList(testArgs)).toJson(QJsonDocument::Compact));
        aaStartArgs += {u"--ps"_s, u"io.qt.appArgsJson"_s, shellSingleQuote(json)};
    }

    // aa start prints errors (screen locked, ability not found, ...) to stdout.
    {
        const QString aaOut = runHdc(hdc, aaStartArgs, /*printOnFailure=*/true);
        if (aaOut.contains(u"error"_s, Qt::CaseInsensitive)
            || aaOut.contains(u"failed"_s, Qt::CaseInsensitive)) {
            fprintf(stderr, "harmonyostestrunner: aa start: %s\n", qPrintable(aaOut.trimmed()));
        }
    }

    if (!waitForProcessStart(hdc, bundleName, shellExitCodePath)) {
        fprintf(stderr, "harmonyostestrunner: %s: timed out waiting for process to start\n",
                qPrintable(testLibName));
#if QT_CONFIG(systemsemaphore)
        runnerLock.release();
#endif
        return EXIT_ERROR;
    }

    waitForStdoutFile(hdc, shellStdoutPath);

    QProcess stdoutLogger;
    const bool hasStdoutLogger = setupStdoutLogger(stdoutLogger, hdc, shellStdoutPath);
    if (!hasStdoutLogger) {
        fprintf(stderr, "harmonyostestrunner: warning: failed to start stdout logger, "
                "output may be delayed\n");
    }

    // hdc shell always returns 0; can't use `test -f`. Cat the file and check
    // whether the contents parse as int.
    const int pollIntervalMs = 500;

    QElapsedTimer elapsed;
    elapsed.start();
    int testExitCode = -1;
    bool completed = false;
    int aliveCheckCounter = 0;
    qint64 lastTestProgressAt = -1;
    qint64 lastHeartbeatSecs = 0;

    while (!g_interrupted.load()
           && elapsed.elapsed() < qint64(timeoutSecs) * 1000)
    {
        if (hasStdoutLogger) {
            const QByteArray chunk = stdoutLogger.readAllStandardOutput();
            if (!chunk.isEmpty()) {
                fwrite(chunk.constData(), 1, static_cast<size_t>(chunk.size()), stdout);
                fflush(stdout);
                // Markers match QPlainTestLogger output in qtestlog.cpp. If
                // QTest's format ever changes, the no-progress watchdog silently
                // stops firing — hung tests only trip the overall timeout.
                if (chunk.contains("PASS   :") || chunk.contains("FAIL!  :")
                    || chunk.contains("Totals:"))
                    lastTestProgressAt = elapsed.elapsed();
            }
        }

        // Primary completion signal: exit-code file becomes parseable as int.
        {
            const QString exitContent =
                runHdc(hdc, {u"shell"_s, u"cat"_s, shellExitCodePath});
            bool ok = false;
            const int code = exitContent.trimmed().toInt(&ok);
            if (ok) {
                testExitCode = code;
                completed = true;
                break;
            }
        }

        // Liveness check every ~1.5 s. Re-read the exit-code file to cover the
        // race where the process exits cleanly between checks.
        if (++aliveCheckCounter % 3 == 0 && !isProcessAlive(hdc, bundleName)) {
            const QString exitContent =
                runHdc(hdc, {u"shell"_s, u"cat"_s, shellExitCodePath});
            bool ok = false;
            const int code = exitContent.trimmed().toInt(&ok);
            if (ok) {
                testExitCode = code;
                completed = true;
                break;
            }

            fprintf(stderr, "harmonyostestrunner: %s: process exited without writing "
                    "exit code — likely crashed\n", qPrintable(testLibName));
            testExitCode = EXIT_NOEXITCODE;
            completed = true;
            break;
        }

        if (noProgressTimeoutSecs > 0 && lastTestProgressAt >= 0
            && elapsed.elapsed() - lastTestProgressAt
               > qint64(noProgressTimeoutSecs) * 1000)
        {
            fprintf(stderr,
                    "harmonyostestrunner: %s: no test case progress for %d seconds "
                    "— main thread likely deadlocked, force-stopping\n",
                    qPrintable(testLibName), noProgressTimeoutSecs);
            runHdc(hdc, {u"shell"_s, u"aa"_s, u"force-stop"_s, bundleName});
            testExitCode = EXIT_CRASH;
            completed = true;
            break;
        }

        const qint64 elapsedSecs = elapsed.elapsed() / 1000;
        if (elapsedSecs >= lastHeartbeatSecs + 30) {
            fprintf(stderr, "harmonyostestrunner: %s still running (%lld s elapsed)\n",
                    qPrintable(testLibName), static_cast<long long>(elapsedSecs));
            lastHeartbeatSecs = elapsedSecs;
        }

        if (hasStdoutLogger)
            stdoutLogger.waitForReadyRead(pollIntervalMs);
        else
            QThread::msleep(pollIntervalMs);
    }

    if (hasStdoutLogger) {
        if (stdoutLogger.state() != QProcess::NotRunning) {
            // Drain bytes still in flight after the test finished — the 100 ms
            // device-side loop and hdc transport both add latency.
            while (stdoutLogger.waitForReadyRead(200)) {
                const QByteArray chunk = stdoutLogger.readAllStandardOutput();
                if (chunk.isEmpty())
                    break;
                fwrite(chunk.constData(), 1, static_cast<size_t>(chunk.size()), stdout);
                fflush(stdout);
            }
            stdoutLogger.terminate();
            stdoutLogger.waitForFinished(5000);
        }
        const QByteArray finalChunk = stdoutLogger.readAllStandardOutput();
        if (!finalChunk.isEmpty()) {
            fwrite(finalChunk.constData(), 1, static_cast<size_t>(finalChunk.size()), stdout);
            fflush(stdout);
        }
    }

    if (!completed) {
        if (g_interrupted.load()) {
            runHdc(hdc, {u"shell"_s, u"aa"_s, u"force-stop"_s, bundleName});
#if QT_CONFIG(systemsemaphore)
            runnerLock.release();
#endif
            return EXIT_ERROR;
        }
        fprintf(stderr,
                "harmonyostestrunner: TIMEOUT — %s did not complete within %d seconds\n",
                qPrintable(testLibName), timeoutSecs);
        runHdc(hdc, {u"shell"_s, u"aa"_s, u"force-stop"_s, bundleName});
#if QT_CONFIG(systemsemaphore)
        runnerLock.release();
#endif
        return EXIT_TIMEOUT;
    }

#if QT_CONFIG(systemsemaphore)
    runnerLock.release();
#endif
    return testExitCode;
}
