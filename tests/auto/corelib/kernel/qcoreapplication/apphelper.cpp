// Copyright (C) 2024 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qcoreapplication_p.h>
#include <qplatformdefs.h>
#include <qthread.h>
#include <qtimer.h>

#if QT_CONFIG(library)
#  include <qpluginloader.h>
#endif

#include <string_view>
#include <thread>

#include <stdlib.h>
#include <stdio.h>
#ifdef Q_OS_WIN
#  include <io.h>
#else
#  include <sys/types.h>
#  include <unistd.h>
#endif

#if defined(QT_WIDGETS_LIB)
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
using TestApplication = QApplication;
#elif defined(QT_GUI_LIB)
#include <QtGui/QGuiApplication>
#include <QtGui/QWindow>
using TestApplication = QGuiApplication;
#else
using TestApplication = QCoreApplication;
#endif

#ifndef STDIN_FILENO
#  define STDIN_FILENO      0
#endif

#include "maybeshow.h"

static int exitFromEventLoop(int argc, char **argv)
{
    TestApplication app(argc, argv);
    [[maybe_unused]] auto w = maybeShowSomething();
    QTimer::singleShot(200, &app, [] { ::exit(EXIT_SUCCESS); });
    return app.exec();
}

static int exitFromThread(int argc, char **argv)
{
    TestApplication app(argc, argv);
    [[maybe_unused]] auto w = maybeShowSomething();
    auto thr = QThread::create([] {
        if (!QThread::currentThread())
            fprintf(stderr, "QThread::currentThread is null!\n");
        ::exit(EXIT_SUCCESS);
    });
    QTimer::singleShot(200, thr, [&thr] {
        thr->start();

        // block the GUI forever, otherwise the unloading of the QPA plugins
        // could cause a crash if something is running.
        char c;
        int r = QT_READ(STDIN_FILENO, &c, 1);
        Q_UNUSED(r);
    });
    app.exec();
    Q_UNREACHABLE_RETURN(EXIT_FAILURE);
}

// mix of the two above
static int exitFromThreadedEventLoop(int argc, char **argv)
{
    // disable the Glib event loop because that causes at exit:
    //  GLib-CRITICAL **: 11:48:09.717: g_source_unref_internal: assertion 'source != NULL' failed
    // (I don't think it's *our* bug).
    qputenv("QT_NO_GLIB", "1");

    TestApplication app(argc, argv);
    [[maybe_unused]] auto w = maybeShowSomething();
    struct Thread : public QThread {
        void run() override
        {
            QObject dummy;
            QTimer::singleShot(1, &dummy, [] { ::exit(EXIT_SUCCESS); });
            exec();
        }
    } thr;

    QTimer::singleShot(200, &thr, [&thr] { thr.start(); });
    app.exec();
    Q_UNREACHABLE_RETURN(EXIT_FAILURE);
}

// see QTBUG-134080
static int exitWithPlugins(int argc, char **argv)
{
#if defined(Q_OS_QNX)
    // The plugin loading fails: "The shared library was not found".
    puts("Plugin doesn't get deployed.");
    return -1;
#elif defined(Q_OS_WIN)
    puts("Not possible on Windows: DLL destruction order does not follow C++ Standard.");
    return -1;
#elif QT_CONFIG(library)
    TestApplication app(argc, argv);

    QString pluginName = app.applicationDirPath() + "/apphelper_"
#if defined(QT_WIDGETS_LIB)
            "widgets"
#elif defined(QT_GUI_LIB)
            "gui"
#else
            "core"
#endif
            "_plugin";

    QPluginLoader loader(pluginName);
    QObject *instance = loader.instance();
    if (!instance) {
        fprintf(stderr, "Did not get an instance from the plugin: %s\n",
                qPrintable(loader.errorString()));
    }

    [[maybe_unused]] auto w = maybeShowSomething();
    QTimer::singleShot(200, &app, &TestApplication::quit);
    return app.exec();
#else
    puts("Qt built without plugin support")
    return -1;
#endif
}

// see QTBUG-130895
static int mainAppInAThread(int argc, char **argv)
{
#if defined(Q_OS_APPLE) && defined(QT_GUI_LIB)
    // *** Assertion failure in -[NSMenu _setMenuName:], NSMenu.m:777
    // *** Terminating app due to uncaught exception 'NSInternalInconsistencyException', reason: 'API misuse: setting the main menu on a non-main thread. Main menu contents should only be modified from the main thread.'
    puts("QGuiApplication in a thread fails inside Apple libs");
    return -1;
#endif

    // note: when using std::thread in MinGW, we exercise different code paths
    // from QThread (winpthreads vs native)
    auto callable = [](int argc, char **argv) {
        TestApplication app(argc, argv);
    };

    if (auto id = QCoreApplicationPrivate::theMainThreadId.loadRelaxed()) {
        fprintf(stderr, "QCoreApplicationPrivate::theMainThreadId (%p) predates QCoreApplication!\n",
                id);
    }

    std::thread thr(callable, argc, argv);
    auto handle = thr.native_handle();
    thr.join();

    auto id = QCoreApplicationPrivate::theMainThreadId.loadRelaxed();
    if (id) {
        FILE *stream = stdout;
#if defined(QT_GUI_LIB)
        // QtGui has several Q_GLOBAL_STATICs surviving with live QObject (e.g.
        // the QFactoryLoaders), so theMainThreadId has not been unset yet.
#elif defined(Q_OS_WIN) && defined(Q_CC_GNU)
        // Windows threading and cleaning of global static is messed up.
#else
        // Ensure that theMainThread is gone, so we could now start a new
        // thread with QCoreApplication.
        stream = stderr;
#endif
        fprintf(stream, "QCoreApplicationPrivate::theMainThreadId (%p) still extant (native handle %#llx)\n",
                id, quint64(quintptr(handle)));
        if (stream == stderr)
            return EXIT_FAILURE;
        fflush(stream);
    }

    return EXIT_SUCCESS;
}

static int usage(const char *name)
{
    printf("%s <subtest>\n", name);
    return EXIT_FAILURE;
}

int main(int argc, char **argv)
{
    if (argc == 1)
        return usage(argv[0]);

    // remove some environment options that cause Qt to print warnings
    qunsetenv("QT_SCALE_FACTOR_ROUNDING_POLICY");

#  if defined(Q_OS_MACOS)
    // Work around system framework spamming logs with
    // "+[IMKClient subclass]: chose IMKClient_Legacy"
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSSequoia)
        qputenv("CFLOG_FORCE_DISABLE_STDERR", "1");

    // Don't steal app focus from main test
    qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");
#  endif

    std::string_view subtest(argv[1]);
    if (subtest == "exitFromEventLoop")
        return exitFromEventLoop(argc - 1, argv + 1);
    if (subtest == "exitFromThread")
        return exitFromThread(argc - 1, argv + 1);
    if (subtest == "exitFromThreadedEventLoop")
        return exitFromThreadedEventLoop(argc - 1, argv + 1);
    if (subtest == "exitWithPlugins")
        return exitWithPlugins(argc - 1, argv + 1);
    if (subtest == "mainAppInAThread")
        return mainAppInAThread(argc - 1, argv + 1);

    fprintf(stderr, "%s: unknown test %s\n", argv[0], argv[1]);
    return EXIT_FAILURE;
}
