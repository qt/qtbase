// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qcoreapplication.h"

#include <QtCore/QtCore>
#include <QTest>

#include <private/qabstracteventdispatcher_p.h> // for qGlobalPostedEventsCount()
#include <private/qcoreapplication_p.h>
#include <private/qcoreevent_p.h>
#include <private/qeventloop_p.h>
#include <private/qthread_p.h>

#ifdef Q_OS_WIN
#include <QtCore/qt_windows.h>
#endif

Q_LOGGING_CATEGORY(lcTests, "qt.tests." QT_STRINGIFY(tst_QCoreApplication))

typedef QCoreApplication TestApplication;

class EventSpy : public QObject
{
   Q_OBJECT

public:
    QList<int> recordedEvents;
    std::function<void(QObject *, QEvent *)> eventCallback;
    bool eventFilter(QObject *target, QEvent *event) override
    {
        recordedEvents.append(event->type());
        if (eventCallback)
            eventCallback(target, event);
        return false;
    }
};

class ThreadedEventReceiver : public QObject
{
    Q_OBJECT
public:
    QList<int> recordedEvents;
    bool event(QEvent *event) override
    {
        if (event->type() != QEvent::Type(QEvent::User + 1))
            return QObject::event(event);
        recordedEvents.append(event->type());
        QThread::currentThread()->quit();
        QCoreApplication::quit();
        moveToThread(0);
        return true;
    }
};

class Thread : public QDaemonThread
{
    void run() override
    {
        QThreadData *data = QThreadData::current();
        QVERIFY(!data->requiresCoreApplication);        // daemon thread
        data->requiresCoreApplication = requiresCoreApplication;
        QThread::run();
    }

public:
    Thread() : requiresCoreApplication(true) {}
    bool requiresCoreApplication;
};

void tst_QCoreApplication::sendEventsOnProcessEvents()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    EventSpy spy;
    app.installEventFilter(&spy);

    QCoreApplication::postEvent(&app,  new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();
    QVERIFY(spy.recordedEvents.contains(QEvent::User + 1));
}

void tst_QCoreApplication::getSetCheck()
{
    // do not crash
    QString v = QCoreApplication::applicationVersion();
    v = QLatin1String("3.0.0 prerelease 1");
    QCoreApplication::setApplicationVersion(v);
    QCOMPARE(QCoreApplication::applicationVersion(), v);

    // Test the property
    {
        int argc = 1;
        char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
        TestApplication app(argc, argv);
        QCOMPARE(app.property("applicationVersion").toString(), v);
    }
    v = QString();
    QCoreApplication::setApplicationVersion(v);
    QCOMPARE(QCoreApplication::applicationVersion(), v);
}

void tst_QCoreApplication::qAppName()
{
#ifdef QT_QGUIAPPLICATIONTEST
    const char* appName = "tst_qguiapplication";
#else
    const char* appName = "tst_qcoreapplication";
#endif

    {
        int argc = 1;
        char *argv[] = { const_cast<char*>(appName) };
        TestApplication app(argc, argv);
        QCOMPARE(::qAppName(), QString::fromLatin1(appName));
        QCOMPARE(QCoreApplication::applicationName(), QString::fromLatin1(appName));
    }
    // The application name should still be available after destruction;
    // global statics often rely on this.
    QCOMPARE(QCoreApplication::applicationName(), QString::fromLatin1(appName));

    // Setting the appname before creating the application should work (QTBUG-45283)
    const QString wantedAppName("my app name");
    {
        int argc = 1;
        char *argv[] = { const_cast<char*>(appName) };
        QCoreApplication::setApplicationName(wantedAppName);
        TestApplication app(argc, argv);
        QCOMPARE(::qAppName(), QString::fromLatin1(appName));
        QCOMPARE(QCoreApplication::applicationName(), wantedAppName);
    }
    QCOMPARE(QCoreApplication::applicationName(), wantedAppName);

    // Restore to initial value
    QCoreApplication::setApplicationName(QString());
    QCOMPARE(QCoreApplication::applicationName(), QString());
}

void tst_QCoreApplication::qAppVersion()
{
#if defined(Q_OS_WIN)
    const char appVersion[] = "1.2.3.4";
#elif defined(Q_OS_DARWIN) || defined(Q_OS_ANDROID)
    const char appVersion[] = "1.2.3";
#else
    const char appVersion[] = "";
#endif

    {
        int argc = 0;
        char *argv[] = { nullptr };
        TestApplication app(argc, argv);
        QCOMPARE(QCoreApplication::applicationVersion(), QString::fromLatin1(appVersion));
    }
    // The application version should still be available after destruction
    QCOMPARE(QCoreApplication::applicationVersion(), QString::fromLatin1(appVersion));

    // Setting the appversion before creating the application should work
    const QString wantedAppVersion("0.0.1");
    {
        int argc = 0;
        char *argv[] = { nullptr };
        QCoreApplication::setApplicationVersion(wantedAppVersion);
        TestApplication app(argc, argv);
        QCOMPARE(QCoreApplication::applicationVersion(), wantedAppVersion);
    }
    QCOMPARE(QCoreApplication::applicationVersion(), wantedAppVersion);

    // Restore to initial value
    QCoreApplication::setApplicationVersion(QString());
    QCOMPARE(QCoreApplication::applicationVersion(), QString());
}

void tst_QCoreApplication::argc()
{
    {
        int argc = 1;
        char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
        TestApplication app(argc, argv);
        QCOMPARE(argc, 1);
        QCOMPARE(app.arguments().size(), 1);
    }

    {
        int argc = 4;
        char *argv[] = { const_cast<char*>(QTest::currentAppName()),
                         const_cast<char*>("arg1"),
                         const_cast<char*>("arg2"),
                         const_cast<char*>("arg3") };
        TestApplication app(argc, argv);
        QCOMPARE(argc, 4);
        QCOMPARE(app.arguments().size(), 4);
    }

    {
        int argc = 0;
        char **argv = 0;
        TestApplication app(argc, argv);
        QCOMPARE(argc, 0);
        QCOMPARE(app.arguments().size(), 0);
    }

    {
        int argc = 2;
        char *argv[] = { const_cast<char*>(QTest::currentAppName()),
                         const_cast<char*>("-qmljsdebugger=port:3768,block") };
        TestApplication app(argc, argv);
        QCOMPARE(argc, 1);
        QCOMPARE(app.arguments().size(), 1);
    }
}

#if QT_CONFIG(library)
static char argv0name[] =
#ifdef QT_GUI_LIB
        "tst_qguiapplication"
#else
        "tst_qcoreapplication"
#endif
        ;
static char *argv0 = argv0name;

static bool isPathListIncluded(const QStringList &l, const QStringList &r)
{
    int size = r.size();
    if (size > l.size())
        return false;
#if defined (Q_OS_WIN)
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif
    int i = 0, j = 0;
    for ( ; i < l.size() && j < r.size(); ++i) {
        if (QDir::toNativeSeparators(l[i]).compare(QDir::toNativeSeparators(r[j]), cs) == 0) {
            ++j;
            i = -1;
        }
    }
    return j == r.size();
}

void tst_QCoreApplication::libraryPaths()
{
    const QString testDir = QFileInfo(QFINDTESTDATA("CMakeLists.txt")).absolutePath();
    QVERIFY(!testDir.isEmpty());
    {
        QCoreApplication::setLibraryPaths(QStringList() << testDir);
        QCOMPARE(QCoreApplication::libraryPaths(), (QStringList() << testDir));

        // creating QCoreApplication adds the applicationDirPath to the libraryPath
        int argc = 1;
        QCoreApplication app(argc, &argv0);
        QString appDirPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

        QStringList actual = QCoreApplication::libraryPaths();
        actual.sort();
        QStringList expected;
        expected << testDir << appDirPath;
        expected = QSet<QString>(expected.constBegin(), expected.constEnd()).values();
        expected.sort();

        QVERIFY2(isPathListIncluded(actual, expected),
                 qPrintable("actual:\n - " + actual.join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));
    }
    {
        // creating QCoreApplication adds the applicationDirPath and plugin install path to the libraryPath
        int argc = 1;
        QCoreApplication app(argc, &argv0);
        QString appDirPath = QCoreApplication::applicationDirPath();
        QString installPathPlugins =  QLibraryInfo::path(QLibraryInfo::PluginsPath);

        QStringList actual = QCoreApplication::libraryPaths();
        actual.sort();

        QStringList expected;
        expected << installPathPlugins << appDirPath;
        expected = QSet<QString>(expected.constBegin(), expected.constEnd()).values();
        expected.sort();

#if defined(Q_OS_VXWORKS)
        QEXPECT_FAIL("", "QTBUG-130736: Actual paths on VxWorks differ from expected", Abort);
#endif
        QVERIFY2(isPathListIncluded(actual, expected),
                 qPrintable("actual:\n - " + actual.join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));

        // setting the library paths overrides everything
        QCoreApplication::setLibraryPaths(QStringList() << testDir);
        QVERIFY2(isPathListIncluded(QCoreApplication::libraryPaths(), (QStringList() << testDir)),
                 qPrintable("actual:\n - " + QCoreApplication::libraryPaths().join("\n - ") +
                            "\nexpected:\n - " + testDir));
    }
    {
        qCDebug(lcTests) << "Initial library path:" << QCoreApplication::libraryPaths();

        int count = QCoreApplication::libraryPaths().size();
#if 0
        QCOMPARE(count, 1); // before creating QCoreApplication, only the PluginsPath is in the libraryPaths()
#endif
        QString installPathPlugins =  QLibraryInfo::path(QLibraryInfo::PluginsPath);
        QCoreApplication::addLibraryPath(installPathPlugins);
        qCDebug(lcTests) << "installPathPlugins" << installPathPlugins;
        qCDebug(lcTests) << "After adding plugins path:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().size(), count);
        QCoreApplication::addLibraryPath(testDir);
        QCOMPARE(QCoreApplication::libraryPaths().size(), count + 1);

        // creating QCoreApplication adds the applicationDirPath to the libraryPath
        int argc = 1;
        QCoreApplication app(argc, &argv0);
        QString appDirPath = QCoreApplication::applicationDirPath();
        qCDebug(lcTests) << QCoreApplication::libraryPaths();
        // On Windows CE these are identical and might also be the case for other
        // systems too
        if (appDirPath != installPathPlugins)
            QCOMPARE(QCoreApplication::libraryPaths().size(), count + 2);
    }
    {
        int argc = 1;
        QCoreApplication app(argc, &argv0);

        qCDebug(lcTests) << "Initial library path:" << QCoreApplication::libraryPaths();
        int count = QCoreApplication::libraryPaths().size();
        QString installPathPlugins =  QLibraryInfo::path(QLibraryInfo::PluginsPath);
        QCoreApplication::addLibraryPath(installPathPlugins);
        qCDebug(lcTests) << "installPathPlugins" << installPathPlugins;
        qCDebug(lcTests) << "After adding plugins path:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().size(), count);

        QString appDirPath = QCoreApplication::applicationDirPath();

        QCoreApplication::addLibraryPath(appDirPath);
        QCoreApplication::addLibraryPath(appDirPath + "/..");
        qCDebug(lcTests) << "appDirPath" << appDirPath;
        qCDebug(lcTests) << "After adding appDirPath && appDirPath + /..:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().size(), count + 1);
#ifdef Q_OS_MACOS
        QCoreApplication::addLibraryPath(appDirPath + "/../MacOS");
#else
        QCoreApplication::addLibraryPath(appDirPath + "/tmp/..");
#endif
        qCDebug(lcTests) << "After adding appDirPath + /tmp/..:" << QCoreApplication::libraryPaths();
        QCOMPARE(QCoreApplication::libraryPaths().size(), count + 1);
    }
}

void tst_QCoreApplication::libraryPaths_qt_plugin_path()
{
    int argc = 1;

    QCoreApplication app(argc, &argv0);
    QString appDirPath = QCoreApplication::applicationDirPath();

    // Our hook into libraryPaths() initialization: Set the QT_PLUGIN_PATH environment variable
    QString installPathPluginsDeCanon = appDirPath + QString::fromLatin1("/tmp/..");
    QByteArray ascii = QFile::encodeName(installPathPluginsDeCanon);
    qputenv("QT_PLUGIN_PATH", ascii);

    QVERIFY(!QCoreApplication::libraryPaths().contains(appDirPath + QString::fromLatin1("/tmp/..")));
}

void tst_QCoreApplication::libraryPaths_qt_plugin_path_2()
{
#ifdef Q_OS_UNIX
    QByteArray validPath = QDir("/tmp").canonicalPath().toLatin1();
    QByteArray nonExistentPath = "/nonexistent";
    QByteArray pluginPath = validPath + ':' + nonExistentPath;
#elif defined(Q_OS_WIN)
    QByteArray validPath = "C:\\windows";
    QByteArray nonExistentPath = "Z:\\nonexistent";
    QByteArray pluginPath = validPath + ';' + nonExistentPath;
#endif

    {
        // Our hook into libraryPaths() initialization: Set the QT_PLUGIN_PATH environment variable
        qputenv("QT_PLUGIN_PATH", pluginPath);

        int argc = 1;

        QCoreApplication app(argc, &argv0);

        // library path list should contain the default plus the one valid path
        QStringList expected =
            QStringList()
            << QLibraryInfo::path(QLibraryInfo::PluginsPath)
            << QDir(QCoreApplication::applicationDirPath()).canonicalPath()
            << QDir(QDir::fromNativeSeparators(QString::fromLatin1(validPath))).canonicalPath();

#if defined(Q_OS_VXWORKS)
        QEXPECT_FAIL("", "QTBUG-130736: Actual paths on VxWorks differ from expected", Abort);
#endif
        QVERIFY2(isPathListIncluded(QCoreApplication::libraryPaths(), expected),
                 qPrintable("actual:\n - " + QCoreApplication::libraryPaths().join("\n - ") +
                            "\nexpected:\n - " + expected.join("\n - ")));
    }

    {
        int argc = 1;

        QCoreApplication app(argc, &argv0);

        // library paths are initialized by the QCoreApplication, setting
        // the environment variable here doesn't work
        qputenv("QT_PLUGIN_PATH", pluginPath);

        // library path list should contain the default
        QStringList expected =
            QStringList()
            << QLibraryInfo::path(QLibraryInfo::PluginsPath)
            << QCoreApplication::applicationDirPath();
        QVERIFY(isPathListIncluded(QCoreApplication::libraryPaths(), expected));

        qputenv("QT_PLUGIN_PATH", nullptr);
    }
}
#endif

class EventGenerator : public QObject
{
    Q_OBJECT

public:
    QObject *other;

    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::MaxUser) {
            QCoreApplication::sendPostedEvents(other, 0);
        } else if (e->type() <= QEvent::User + 999) {
            // post a new event in response to this posted event
            int offset = e->type() - QEvent::User;
            offset = (offset * 10 + offset % 10);
            QCoreApplication::postEvent(this, new QEvent(QEvent::Type(QEvent::User + offset)), offset);
        }

        return QObject::event(e);
    }
};

void tst_QCoreApplication::postEvent()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    EventSpy spy;
    EventGenerator odd, even;
    odd.other = &even;
    odd.installEventFilter(&spy);
    even.other = &odd;
    even.installEventFilter(&spy);

    QCoreApplication::postEvent(&odd,  new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::postEvent(&even, new QEvent(QEvent::Type(QEvent::User + 2)));

    QCoreApplication::postEvent(&odd,  new QEvent(QEvent::Type(QEvent::User + 3)), 1);
    QCoreApplication::postEvent(&even, new QEvent(QEvent::Type(QEvent::User + 4)), 2);

    QCoreApplication::postEvent(&odd,  new QEvent(QEvent::Type(QEvent::User + 5)), -2);
    QCoreApplication::postEvent(&even, new QEvent(QEvent::Type(QEvent::User + 6)), -1);

    QList<int> expected;
    expected << QEvent::User + 4
             << QEvent::User + 3
             << QEvent::User + 1
             << QEvent::User + 2
             << QEvent::User + 6
             << QEvent::User + 5;

    QCoreApplication::sendPostedEvents();
    // live lock protection ensures that we only send the initial events
    QCOMPARE(spy.recordedEvents, expected);

    expected.clear();
    expected << QEvent::User + 66
             << QEvent::User + 55
             << QEvent::User + 44
             << QEvent::User + 33
             << QEvent::User + 22
             << QEvent::User + 11;

    spy.recordedEvents.clear();
    QCoreApplication::sendPostedEvents();
    // expect next sequence events
    QCOMPARE(spy.recordedEvents, expected);

    // have the generators call sendPostedEvents() on each other in
    // response to an event
    QCoreApplication::postEvent(&odd, new QEvent(QEvent::MaxUser), INT_MAX);
    QCoreApplication::postEvent(&even, new QEvent(QEvent::MaxUser), INT_MAX);

    expected.clear();
    expected << int(QEvent::MaxUser)
             << int(QEvent::MaxUser)
             << QEvent::User + 555
             << QEvent::User + 333
             << QEvent::User + 111
             << QEvent::User + 666
             << QEvent::User + 444
             << QEvent::User + 222;

    spy.recordedEvents.clear();
    QCoreApplication::sendPostedEvents();
    QCOMPARE(spy.recordedEvents, expected);

    expected.clear();
    expected << QEvent::User + 6666
             << QEvent::User + 5555
             << QEvent::User + 4444
             << QEvent::User + 3333
             << QEvent::User + 2222
             << QEvent::User + 1111;

    spy.recordedEvents.clear();
    QCoreApplication::sendPostedEvents();
    QCOMPARE(spy.recordedEvents, expected);

    // no more events
    expected.clear();
    spy.recordedEvents.clear();
    QCoreApplication::sendPostedEvents();
    QCOMPARE(spy.recordedEvents, expected);
}

void tst_QCoreApplication::removePostedEvents()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    EventSpy spy;
    QObject one, two;
    one.installEventFilter(&spy);
    two.installEventFilter(&spy);

    QList<int> expected;

    // remove all events for one object
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 2)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 3)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 4)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 5)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 6)));
    QCoreApplication::removePostedEvents(&one);
    expected << QEvent::User + 4
             << QEvent::User + 5
             << QEvent::User + 6;
    QCoreApplication::sendPostedEvents();
    QCOMPARE(spy.recordedEvents, expected);
    spy.recordedEvents.clear();
    expected.clear();

    // remove all events for all objects
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 7)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 8)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 9)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 10)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 11)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 12)));
    QCoreApplication::removePostedEvents(0);
    QCoreApplication::sendPostedEvents();
    QVERIFY(spy.recordedEvents.isEmpty());

    // remove a specific type of event for one object
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 13)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 14)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 15)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 16)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 17)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 18)));
    QCoreApplication::removePostedEvents(&one, QEvent::User + 13);
    QCoreApplication::removePostedEvents(&two, QEvent::User + 18);
    QCoreApplication::sendPostedEvents();
    expected << QEvent::User + 14
             << QEvent::User + 15
             << QEvent::User + 16
             << QEvent::User + 17;
    QCOMPARE(spy.recordedEvents, expected);
    spy.recordedEvents.clear();
    expected.clear();

    // remove a specific type of event for all objects
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 19)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 19)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 20)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 20)));
    QCoreApplication::postEvent(&one, new QEvent(QEvent::Type(QEvent::User + 21)));
    QCoreApplication::postEvent(&two, new QEvent(QEvent::Type(QEvent::User + 21)));
    QCoreApplication::removePostedEvents(0, QEvent::User + 20);
    QCoreApplication::sendPostedEvents();
    expected << QEvent::User + 19
             << QEvent::User + 19
             << QEvent::User + 21
             << QEvent::User + 21;
    QCOMPARE(spy.recordedEvents, expected);
    spy.recordedEvents.clear();
    expected.clear();
}

#if QT_CONFIG(thread)
class DeliverInDefinedOrderThread : public QThread
{
    Q_OBJECT

public:
    DeliverInDefinedOrderThread()
        : QThread()
    { }

signals:
    void progress(int);

protected:
    void run() override
    {
        emit progress(1);
        emit progress(2);
        emit progress(3);
        emit progress(4);
        emit progress(5);
        emit progress(6);
        emit progress(7);
    }
};

class DeliverInDefinedOrderObject : public QObject
{
    Q_OBJECT

    QPointer<QThread> thread;
    int count;
    int startCount;
    int loopLevel;

public:
    DeliverInDefinedOrderObject(QObject *parent)
        : QObject(parent), thread(0), count(0), startCount(0), loopLevel(0)
    { }

signals:
    void done();

public slots:
    void startThread()
    {
        QVERIFY(!thread);
        thread = new DeliverInDefinedOrderThread();
        connect(thread, SIGNAL(progress(int)), this, SLOT(threadProgress(int)));
        connect(thread, SIGNAL(finished()), this, SLOT(threadFinished()));
        connect(thread, SIGNAL(destroyed()), this, SLOT(threadDestroyed()));
        thread->start();

        QCoreApplication::postEvent(this, new QEvent(QEvent::MaxUser), -1);
    }

    void threadProgress(int v)
    {
        ++count;
        QCOMPARE(v, count);

        QCoreApplication::postEvent(this, new QEvent(QEvent::MaxUser), -1);
    }

    void threadFinished()
    {
        QCOMPARE(count, 7);
        count = 0;
        thread->deleteLater();

        QCoreApplication::postEvent(this, new QEvent(QEvent::MaxUser), -1);
    }

    void threadDestroyed()
    {
        if (++startCount < 20)
            startThread();
        else
            emit done();
    }

public:
    bool event(QEvent *event) override
    {
        switch (event->type()) {
        case QEvent::User:
        {
            ++loopLevel;
            if (loopLevel == 2) {
                // Ready. Starts a thread that emits (queued) signals, which should be handled in order
                startThread();
            }
            QCoreApplication::postEvent(this, new QEvent(QEvent::MaxUser), -1);
            (void) QEventLoop().exec();
            break;
        }
        default:
            break;
        }
        return QObject::event(event);
    }
};

void tst_QCoreApplication::deliverInDefinedOrder()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    DeliverInDefinedOrderObject obj(&app);
    // causes sendPostedEvents() to recurse twice
    QCoreApplication::postEvent(&obj, new QEvent(QEvent::User));
    QCoreApplication::postEvent(&obj, new QEvent(QEvent::User));

    QObject::connect(&obj, SIGNAL(done()), &app, SLOT(quit()));
    app.exec();
}
#endif // QT_CONFIG(thread)

void tst_QCoreApplication::applicationPid()
{
    QVERIFY(QCoreApplication::applicationPid() > 0);
}

#ifdef QT_BUILD_INTERNAL
class GlobalPostedEventsCountObject : public QObject
{
    Q_OBJECT

public:
    QList<qsizetype> globalPostedEventsCount;

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::User)
            globalPostedEventsCount.append(qGlobalPostedEventsCount());
        return QObject::event(event);
    }
};

void tst_QCoreApplication::globalPostedEventsCount()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    QCoreApplication::sendPostedEvents();
    QCOMPARE(qGlobalPostedEventsCount(), qsizetype(0));

    GlobalPostedEventsCountObject x;
    QCoreApplication::postEvent(&x, new QEvent(QEvent::User));
    QCoreApplication::postEvent(&x, new QEvent(QEvent::User));
    QCoreApplication::postEvent(&x, new QEvent(QEvent::User));
    QCoreApplication::postEvent(&x, new QEvent(QEvent::User));
    QCoreApplication::postEvent(&x, new QEvent(QEvent::User));
    QCOMPARE(qGlobalPostedEventsCount(), qsizetype(5));

    QCoreApplication::sendPostedEvents();
    QCOMPARE(qGlobalPostedEventsCount(), qsizetype(0));

    const QList<qsizetype> expected = {4, 3, 2, 1, 0};
    QCOMPARE(x.globalPostedEventsCount, expected);
}
#endif // QT_BUILD_INTERNAL

class ProcessEventsAlwaysSendsPostedEventsObject : public QObject
{
public:
    int counter;

    inline ProcessEventsAlwaysSendsPostedEventsObject()
        : counter(0)
    { }

    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::User)
            ++counter;
        return QObject::event(event);
    }
};

void tst_QCoreApplication::processEventsAlwaysSendsPostedEvents()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    ProcessEventsAlwaysSendsPostedEventsObject object;
    QElapsedTimer t;
    t.start();
    int i = 1;
    do {
        QCoreApplication::postEvent(&object, new QEvent(QEvent::User));
        QCoreApplication::processEvents();
        QCOMPARE(object.counter, i);
        ++i;
    } while (t.elapsed() < 1000);
}

#ifdef Q_OS_WIN
void tst_QCoreApplication::sendPostedEventsInNativeLoop()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    bool signalReceived = false;

    // Post a message to the queue
    QMetaObject::invokeMethod(this, [&signalReceived]() {
        signalReceived = true;
    }, Qt::QueuedConnection);

    QElapsedTimer elapsedTimer;
    elapsedTimer.start();

    // Exec own message loop
    MSG msg;
    forever {
        if (elapsedTimer.hasExpired(3000) || signalReceived)
            break;

        if (!::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            QThread::msleep(100);
            continue;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    QVERIFY(signalReceived);
}
#endif // Q_OS_WIN

class QuitBlocker : public QObject
{
   Q_OBJECT

public:
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::Quit) {
            event->ignore();
            return true;
        }

        return false;
    }
};

void tst_QCoreApplication::quit()
{
    TestApplication::quit(); // Should not do anything

    {
        int argc = 1;
        char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
        TestApplication app(argc, argv);

        EventSpy spy;
        app.installEventFilter(&spy);

        {
            QTimer::singleShot(0, &app, SLOT(quit()));
            app.exec();
            QVERIFY(spy.recordedEvents.contains(QEvent::Quit));
        }

        spy.recordedEvents.clear();

        {
            QTimer::singleShot(0, qApp, SLOT(quit()));
            app.exec();
            QVERIFY(spy.recordedEvents.contains(QEvent::Quit));
        }

        spy.recordedEvents.clear();

        {
            QTimer::singleShot(0, [&]{ TestApplication::quit(); });
            app.exec();
            QVERIFY(spy.recordedEvents.contains(QEvent::Quit));
        }

        spy.recordedEvents.clear();

        {
            QuitBlocker quitBlocker;
            app.installEventFilter(&quitBlocker);

            QTimer::singleShot(0, [&]{ TestApplication::quit(); });
            QTimer::singleShot(200, [&]{ TestApplication::exit(); });
            app.exec();
            QVERIFY(!spy.recordedEvents.contains(QEvent::Quit));
        }
    }

    TestApplication::quit(); // Should not do anything
}

void tst_QCoreApplication::reexec()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    // exec once
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    QCOMPARE(app.exec(), 0);

    // and again
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    QCOMPARE(app.exec(), 0);
}

void tst_QCoreApplication::execAfterExit()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    app.exit(1);
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    QCOMPARE(app.exec(), 0);
}

void tst_QCoreApplication::eventLoopExecAfterExit()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    // exec once and exit
    QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
    QCOMPARE(app.exec(), 0);

    // and again, but this time using a QEventLoop
    QEventLoop loop;
    QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
    QCOMPARE(loop.exec(), 0);
}

class DummyEventDispatcher : public QAbstractEventDispatcher {
public:
    DummyEventDispatcher() : QAbstractEventDispatcher(), visited(false) {}
    bool processEvents(QEventLoop::ProcessEventsFlags) override {
        visited = true;
        emit awake();
        QCoreApplication::sendPostedEvents();
        return false;
    }
    void registerSocketNotifier(QSocketNotifier *) override {}
    void unregisterSocketNotifier(QSocketNotifier *) override {}
    void registerTimer(int , qint64 , Qt::TimerType, QObject *) override {}
    bool unregisterTimer(int ) override { return false; }
    bool unregisterTimers(QObject *) override { return false; }
    QList<TimerInfo> registeredTimers(QObject *) const override { return QList<TimerInfo>(); }
    int remainingTime(int) override { return 0; }
    void wakeUp() override {}
    void interrupt() override {}

#ifdef Q_OS_WIN
    bool registerEventNotifier(QWinEventNotifier *) { return false; }
    void unregisterEventNotifier(QWinEventNotifier *) { }
#endif

    bool visited;
};

void tst_QCoreApplication::customEventDispatcher()
{
    // there should be no ED yet
    QVERIFY(!QCoreApplication::eventDispatcher());
    DummyEventDispatcher *ed = new DummyEventDispatcher;
    QCoreApplication::setEventDispatcher(ed);
    // the new ED should be set
    QCOMPARE(QCoreApplication::eventDispatcher(), ed);
    // test the alternative API of QAbstractEventDispatcher
    QCOMPARE(QAbstractEventDispatcher::instance(), ed);
    QPointer<DummyEventDispatcher> weak_ed(ed);
    QVERIFY(!weak_ed.isNull());
    {
        int argc = 1;
        char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
        TestApplication app(argc, argv);
        // instantiating app should not overwrite the ED
        QCOMPARE(QCoreApplication::eventDispatcher(), ed);
        QMetaObject::invokeMethod(&app, "quit", Qt::QueuedConnection);
        app.exec();
        // the custom ED has really been used?
        QVERIFY(ed->visited);
    }
    // ED has been deleted?
    QVERIFY(weak_ed.isNull());
}

class JobObject : public QObject
{
    Q_OBJECT
public:

    explicit JobObject(QEventLoop *loop, QObject *parent = nullptr)
        : QObject(parent), locker(loop)
    {
        QTimer::singleShot(1000, this, SLOT(timeout()));
    }

    explicit JobObject(QObject *parent = nullptr)
        : QObject(parent)
    {
        QTimer::singleShot(1000, this, SLOT(timeout()));
    }

public slots:
    void startSecondaryJob()
    {
        new JobObject();
    }

private slots:
    void timeout()
    {
        emit done();
        deleteLater();
    }

signals:
    void done();

private:
    QEventLoopLocker locker;
};

class QuitTester : public QObject
{
    Q_OBJECT
public:
    QuitTester(QObject *parent = nullptr)
      : QObject(parent)
    {
        QTimer::singleShot(0, this, SLOT(doTest()));
    }

private slots:
    void doTest()
    {
        QCoreApplicationPrivate *privateClass = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));

        {
            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 0);
            // Test with a lock active so that the refcount doesn't drop to zero during these tests, causing a quit.
            // (until we exit the scope)
            QEventLoopLocker locker;

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 1);

            JobObject *job1 = new JobObject(this);

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 2);

            delete job1;

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 1);

            job1 = new JobObject(this);

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 2);

            JobObject *job2 = new JobObject(this);

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 3);

            delete job1;

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 2);

            JobObject *job3 = new JobObject(job2);
            Q_UNUSED(job3);

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 3);

            JobObject *job4 = new JobObject(job2);
            Q_UNUSED(job4);

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 4);

            delete job2;

            QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 1);

        }
        QCOMPARE(privateClass->quitLockRef.loadRelaxed(), 0);
    }
};

void tst_QCoreApplication::testQuitLock()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    QuitTester tester;
    app.exec();
}


void tst_QCoreApplication::QTBUG31606_QEventDestructorDeadLock()
{
    class MyEvent : public QEvent
    { public:
        MyEvent() : QEvent(QEvent::Type(QEvent::User + 1)) {}
        ~MyEvent() {
            QCoreApplication::postEvent(qApp, new QEvent(QEvent::Type(QEvent::User+2)));
        }
    };

    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    EventSpy spy;
    app.installEventFilter(&spy);

    QCoreApplication::postEvent(&app, new MyEvent);
    QCoreApplication::processEvents();
    QVERIFY(spy.recordedEvents.contains(QEvent::User + 1));
    QVERIFY(!spy.recordedEvents.contains(QEvent::User + 2));
    QCoreApplication::processEvents();
    QVERIFY(spy.recordedEvents.contains(QEvent::User + 2));
}

// this is almost identical to sendEventsOnProcessEvents
void tst_QCoreApplication::applicationEventFilters_mainThread()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    EventSpy spy;
    app.installEventFilter(&spy);

    QCoreApplication::postEvent(&app,  new QEvent(QEvent::Type(QEvent::User + 1)));
    QTimer::singleShot(10, &app, SLOT(quit()));
    app.exec();
    QVERIFY(spy.recordedEvents.contains(QEvent::User + 1));
}

void tst_QCoreApplication::applicationEventFilters_auxThread()
{
    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);
    QThread thread;
    ThreadedEventReceiver receiver;
    receiver.moveToThread(&thread);

    EventSpy spy;
    app.installEventFilter(&spy);

    // this is very similar to sendEventsOnProcessEvents
    QCoreApplication::postEvent(&receiver,  new QEvent(QEvent::Type(QEvent::User + 1)));
    QTimer::singleShot(1000, &app, SLOT(quit()));
    thread.start();
    app.exec();
    QVERIFY(thread.wait(1000));
    QVERIFY(receiver.recordedEvents.contains(QEvent::User + 1));
    QVERIFY(!spy.recordedEvents.contains(QEvent::User + 1));
}

void tst_QCoreApplication::threadedEventDelivery_data()
{
    QTest::addColumn<bool>("requiresCoreApplication");
    QTest::addColumn<bool>("createCoreApplication");
    QTest::addColumn<bool>("eventsReceived");

    // invalid combination:
    //QTest::newRow("default-without-coreapp") << true << false << false;
    QTest::newRow("default") << true << true << true;
    QTest::newRow("independent-without-coreapp") << false << false << true;
    QTest::newRow("independent-with-coreapp") << false << true << true;
}

// posts the event before the QCoreApplication is destroyed, starts thread after
void tst_QCoreApplication::threadedEventDelivery()
{
    QFETCH(bool, requiresCoreApplication);
    QFETCH(bool, createCoreApplication);
    QFETCH(bool, eventsReceived);

    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    QScopedPointer<TestApplication> app(createCoreApplication ? new TestApplication(argc, argv) : 0);

    Thread thread;
    thread.requiresCoreApplication = requiresCoreApplication;
    ThreadedEventReceiver receiver;
    receiver.moveToThread(&thread);
    QCoreApplication::postEvent(&receiver, new QEvent(QEvent::Type(QEvent::User + 1)));

    thread.start();
    QVERIFY(thread.wait(1000));
    QCOMPARE(receiver.recordedEvents.contains(QEvent::User + 1), eventsReceived);

}

void tst_QCoreApplication::testTrWithPercantegeAtTheEnd()
{
    QCoreApplication::translate("testcontext", "this will crash%", "testdisamb", 3);
}

#if QT_CONFIG(library)
void tst_QCoreApplication::addRemoveLibPaths()
{
    QStringList paths = QCoreApplication::libraryPaths();
    if (paths.isEmpty())
        QSKIP("Cannot add/remove library paths if there are none.");

    QString currentDir = QDir().absolutePath();
    QCoreApplication::addLibraryPath(currentDir);
    QVERIFY(QCoreApplication::libraryPaths().contains(currentDir));

    QCoreApplication::removeLibraryPath(paths[0]);
    QVERIFY(!QCoreApplication::libraryPaths().contains(paths[0]));

    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);

    // If libraryPaths only contains currentDir, neither will be in libraryPaths now.
    if (paths.size() != 1 && currentDir != paths[0]) {
        // Check that modifications stay alive across the creation of an application.
        QVERIFY(QCoreApplication::libraryPaths().contains(currentDir));
        QVERIFY(!QCoreApplication::libraryPaths().contains(paths[0]));
    }

    QStringList replace;
    replace << currentDir << paths[0];
    QCoreApplication::setLibraryPaths(replace);
    QCOMPARE(QCoreApplication::libraryPaths(), replace);
}
#endif

static bool theMainThreadIsSet()
{
    // QCoreApplicationPrivate::mainThread() has a Q_ASSERT we'd trigger
    return QCoreApplicationPrivate::theMainThreadId.loadRelaxed() != nullptr;
}

static bool theMainThreadWasUnset = !theMainThreadIsSet(); // global static
void tst_QCoreApplication::theMainThread()
{
    QVERIFY2(theMainThreadWasUnset, "Something set the theMainThread before main()");
    QVERIFY(theMainThreadIsSet()); // we have at LEAST one QObject alive: tst_QCoreApplication

    int argc = 1;
    char *argv[] = { const_cast<char*>(QTest::currentAppName()) };
    TestApplication app(argc, argv);
    QVERIFY(QCoreApplicationPrivate::theMainThreadId.loadRelaxed());
    QVERIFY(QThread::isMainThread());
    QCOMPARE(app.thread(), thread());
    QCOMPARE(app.thread(), QThread::currentThread());
}

static void createQObjectOnDestruction()
{
    // Make sure that we can create a QObject (and thus have an associated
    // QThread) after the last QObject has been destroyed (especially after
    // QCoreApplication has).

    // Before the fixes, this would cause a dangling pointer dereference. If
    // the problem comes back, it's possible that the following causes no
    // effect.
    QObject obj;
    obj.thread()->setProperty("testing", 1);
    if (!theMainThreadIsSet())
        qFatal("theMainThreadIsSet() returned false");

    // because we created a QObject after QCoreApplicationData was destroyed,
    // the QAdoptedThread won't get cleaned up
}
Q_DESTRUCTOR_FUNCTION(createQObjectOnDestruction)

void tst_QCoreApplication::testDeleteLaterFromBeforeOutermostEventLoop()
{
    int argc = 0;
    QCoreApplication app(argc, nullptr);

    EventSpy *spy = new EventSpy();
    QPointer<QObject> spyPointer = spy;

    app.installEventFilter(spy);
    spy->eventCallback = [spy](QObject *, QEvent *event) {
        if (event->type() == QEvent::User + 1)
            spy->deleteLater();
    };

    QCoreApplication::postEvent(&app, new QEvent(QEvent::Type(QEvent::User + 1)));
    QCoreApplication::processEvents();

    QEventLoop loop;
    QTimer::singleShot(0, &loop, &QEventLoop::quit);
    loop.exec();
    QVERIFY(!spyPointer);
}

void tst_QCoreApplication::setIndividualAttributes_data()
{
    QTest::addColumn<Qt::ApplicationAttribute>("attribute");

    const QMetaEnum &metaEnum = Qt::staticMetaObject.enumerator(Qt::staticMetaObject.indexOfEnumerator("ApplicationAttribute"));
    // - 1 to avoid AA_AttributeCount.
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
        const auto attribute = static_cast<Qt::ApplicationAttribute>(metaEnum.value(i));
        if (attribute == Qt::AA_AttributeCount)
            continue;

        QTest::addRow("%s", metaEnum.key(i)) << attribute;
    }
}

void tst_QCoreApplication::setIndividualAttributes()
{
    QFETCH(Qt::ApplicationAttribute, attribute);

    const auto originalValue = QCoreApplication::testAttribute(attribute);
    auto cleanup = qScopeGuard([=]() {
        QCoreApplication::setAttribute(attribute, originalValue);
    });

    QCoreApplication::setAttribute(attribute, true);
    QVERIFY(QCoreApplication::testAttribute(attribute));

    QCoreApplication::setAttribute(attribute, false);
    QVERIFY(!QCoreApplication::testAttribute(attribute));
}

void tst_QCoreApplication::setMultipleAttributes()
{
    const auto originalDontUseNativeMenuWindowsValue = QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuWindows);
    const auto originalDisableSessionManagerValue = QCoreApplication::testAttribute(Qt::AA_DisableSessionManager);
    auto cleanup = qScopeGuard([=]() {
        QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows, originalDontUseNativeMenuWindowsValue);
        QCoreApplication::setAttribute(Qt::AA_DisableSessionManager, originalDisableSessionManagerValue);
    });

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows, true);
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager, true);
    QVERIFY(QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuWindows));
    QVERIFY(QCoreApplication::testAttribute(Qt::AA_DisableSessionManager));

    QCoreApplication::setAttribute(Qt::AA_DontUseNativeMenuWindows, false);
    QCoreApplication::setAttribute(Qt::AA_DisableSessionManager, false);
    QVERIFY(!QCoreApplication::testAttribute(Qt::AA_DontUseNativeMenuWindows));
    QVERIFY(!QCoreApplication::testAttribute(Qt::AA_DisableSessionManager));
}

#ifndef QT_QGUIAPPLICATIONTEST
QTEST_APPLESS_MAIN(tst_QCoreApplication)
#endif

#include "tst_qcoreapplication.moc"
