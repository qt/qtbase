// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcoreapplication.h"
#include "qcoreapplication_p.h"

#ifndef QT_NO_QOBJECT
#include "qabstracteventdispatcher.h"
#include "qcoreevent.h"
#include "qeventloop.h"
#endif
#include "qmetaobject.h"
#include <private/qproperty_p.h>
#include "qcorecmdlineargs_p.h"
#include <qdatastream.h>
#include <qdebug.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qmutex.h>
#include <private/qloggingregistry_p.h>
#include <qscopeguard.h>
#include <qstandardpaths.h>
#ifndef QT_NO_QOBJECT
#include <qthread.h>
#include <qthreadstorage.h>
#if QT_CONFIG(future)
#include <QtCore/qpromise.h>
#endif
#include <private/qthread_p.h>
#if QT_CONFIG(thread)
#include <qthreadpool.h>
#include <private/qthreadpool_p.h>
#endif
#endif
#include <qelapsedtimer.h>
#include <qlibraryinfo.h>
#include <qvarlengtharray.h>
#include <private/qfactoryloader_p.h>
#include <private/qfunctions_p.h>
#include <private/qlocale_p.h>
#include <private/qlocking_p.h>
#include <private/qhooks_p.h>

#if QT_CONFIG(permissions)
#include <private/qpermissions_p.h>
#endif

#ifndef QT_NO_QOBJECT
#if defined(Q_OS_UNIX)
# if defined(Q_OS_DARWIN)
#  include "qeventdispatcher_cf_p.h"
# else
#  if !defined(QT_NO_GLIB)
#   include "qeventdispatcher_glib_p.h"
#  endif
# endif
# include "qeventdispatcher_unix_p.h"
#endif
#ifdef Q_OS_WIN
#include "qeventdispatcher_win_p.h"
#endif
#endif // QT_NO_QOBJECT

#if defined(Q_OS_ANDROID)
#include <QtCore/qjniobject.h>
#endif

#ifdef Q_OS_DARWIN
#  include "qcore_mac_p.h"
#endif

#include <stdlib.h>

#ifdef Q_OS_UNIX
#  include <locale.h>
#  ifndef Q_OS_INTEGRITY
#    include <langinfo.h>
#  endif
#  include <unistd.h>
#  include <sys/types.h>

#  include "qcore_unix_p.h"
#endif

#if __has_include(<sys/auxv.h>)     // Linux and FreeBSD
#  include <sys/auxv.h>
#endif

#ifdef Q_OS_VXWORKS
#  include <taskLib.h>
#endif

#ifdef Q_OS_WASM
#include <emscripten/val.h>
#endif

#ifdef QT_BOOTSTRAPPED
#include <private/qtrace_p.h>
#else
#include <qtcore_tracepoints_p.h>
#endif

#include <algorithm>
#include <memory>
#include <string>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_TRACE_PREFIX(qtcore,
   "#include <qcoreevent.h>"
);
Q_TRACE_METADATA(qtcore, "ENUM { AUTO, RANGE User ... MaxUser } QEvent::Type;");
Q_TRACE_POINT(qtcore, QCoreApplication_postEvent_entry, QObject *receiver, QEvent *event, QEvent::Type type);
Q_TRACE_POINT(qtcore, QCoreApplication_postEvent_exit);
Q_TRACE_POINT(qtcore, QCoreApplication_postEvent_event_compressed, QObject *receiver, QEvent *event);
Q_TRACE_POINT(qtcore, QCoreApplication_postEvent_event_posted, QObject *receiver, QEvent *event, QEvent::Type type);
Q_TRACE_POINT(qtcore, QCoreApplication_sendEvent, QObject *receiver, QEvent *event, QEvent::Type type);
Q_TRACE_POINT(qtcore, QCoreApplication_sendSpontaneousEvent, QObject *receiver, QEvent *event, QEvent::Type type);
Q_TRACE_POINT(qtcore, QCoreApplication_notify_entry, QObject *receiver, QEvent *event, QEvent::Type type);
Q_TRACE_POINT(qtcore, QCoreApplication_notify_exit, bool consumed, bool filtered);

#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
extern QString qAppFileName();
#endif

Q_CONSTINIT bool QCoreApplicationPrivate::setuidAllowed = false;

#if !defined(Q_OS_WIN)
#ifdef Q_OS_DARWIN
QString QCoreApplicationPrivate::infoDictionaryStringProperty(const QString &propertyName)
{
    QString bundleName;
    QCFString cfPropertyName = propertyName.toCFString();
    CFTypeRef string = CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(),
                                                            cfPropertyName);
    if (string)
        bundleName = QString::fromCFString(static_cast<CFStringRef>(string));
    return bundleName;
}
#endif
QString QCoreApplicationPrivate::appName() const
{
    QString applicationName;
#ifdef Q_OS_DARWIN
    applicationName = infoDictionaryStringProperty(QStringLiteral("CFBundleName"));
#endif
    if (applicationName.isEmpty() && argv[0]) {
        char *p = strrchr(argv[0], '/');
        applicationName = QString::fromLocal8Bit(p ? p + 1 : argv[0]);
    }

    return applicationName;
}
QString QCoreApplicationPrivate::appVersion() const
{
    QString applicationVersion;
#ifdef QT_BOOTSTRAPPED
#elif defined(Q_OS_DARWIN)
    applicationVersion = infoDictionaryStringProperty(QStringLiteral("CFBundleVersion"));
#elif defined(Q_OS_ANDROID)
    QJniObject context(QNativeInterface::QAndroidApplication::context());
    if (context.isValid()) {
        QJniObject pm = context.callObjectMethod(
            "getPackageManager", "()Landroid/content/pm/PackageManager;");
        QJniObject pn = context.callObjectMethod<jstring>("getPackageName");
        if (pm.isValid() && pn.isValid()) {
            QJniObject packageInfo = pm.callObjectMethod(
                "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;",
                pn.object(), 0);
            if (packageInfo.isValid()) {
                QJniObject versionName = packageInfo.getObjectField(
                    "versionName", "Ljava/lang/String;");
                if (versionName.isValid())
                    return versionName.toString();
            }
        }
    }
#endif
    return applicationVersion;
}
#endif // !Q_OS_WIN

Q_CONSTINIT QString *QCoreApplicationPrivate::cachedApplicationFilePath = nullptr;

bool QCoreApplicationPrivate::checkInstance(const char *function)
{
    bool b = (QCoreApplication::self != nullptr);
    if (!b)
        qWarning("QApplication::%s: Please instantiate the QApplication object first", function);
    return b;
}

#if QT_CONFIG(commandlineparser)
void QCoreApplicationPrivate::addQtOptions(QList<QCommandLineOption> *options)
{
    options->append(QCommandLineOption(QStringLiteral("qmljsdebugger"),
                QStringLiteral("Activates the QML/JS debugger with a specified port. The value must be of format port:1234[,block]. \"block\" makes the application wait for a connection."),
                QStringLiteral("value")));
}
#endif

void QCoreApplicationPrivate::processCommandLineArguments()
{
    int j = argc ? 1 : 0;
    for (int i = 1; i < argc; ++i) {
        if (!argv[i])
            continue;
        if (*argv[i] != '-') {
            argv[j++] = argv[i];
            continue;
        }
        const char *arg = argv[i];
        if (arg[1] == '-') // startsWith("--")
            ++arg;
        if (strncmp(arg, "-qmljsdebugger=", 15) == 0) {
            qmljs_debug_arguments = QString::fromLocal8Bit(arg + 15);
        } else if (strcmp(arg, "-qmljsdebugger") == 0 && i < argc - 1) {
            ++i;
            qmljs_debug_arguments = QString::fromLocal8Bit(argv[i]);
        } else {
            argv[j++] = argv[i];
        }
    }

    if (j < argc) {
        argv[j] = nullptr;
        argc = j;
    }
}

// Support for introspection

extern "C" void Q_DECL_EXPORT_OVERRIDABLE qt_startup_hook()
{
}

typedef QList<QtStartUpFunction> QStartUpFuncList;
Q_GLOBAL_STATIC(QStartUpFuncList, preRList)
typedef QList<QtCleanUpFunction> QVFuncList;
Q_GLOBAL_STATIC(QVFuncList, postRList)
Q_CONSTINIT static QBasicMutex globalRoutinesMutex;
Q_CONSTINIT static bool preRoutinesCalled = false;

/*!
    \internal

    Adds a global routine that will be called from the QCoreApplication
    constructor. The public API is Q_COREAPP_STARTUP_FUNCTION.
*/
void qAddPreRoutine(QtStartUpFunction p)
{
    QStartUpFuncList *list = preRList();
    if (!list)
        return;

    if (preRoutinesCalled) {
        Q_ASSERT(QCoreApplication::instance());
        p();
    }

    // Due to C++11 parallel dynamic initialization, this can be called
    // from multiple threads.
    const auto locker = qt_scoped_lock(globalRoutinesMutex);
    list->prepend(p); // in case QCoreApplication is re-created, see qt_call_pre_routines
}

void qAddPostRoutine(QtCleanUpFunction p)
{
    QVFuncList *list = postRList();
    if (!list)
        return;
    const auto locker = qt_scoped_lock(globalRoutinesMutex);
    list->prepend(p);
}

void qRemovePostRoutine(QtCleanUpFunction p)
{
    QVFuncList *list = postRList();
    if (!list)
        return;
    const auto locker = qt_scoped_lock(globalRoutinesMutex);
    list->removeAll(p);
}

static void qt_call_pre_routines()
{
    // After will be allowed invoke QtStartUpFunction when calling qAddPreRoutine
    preRoutinesCalled = true;

    if (!preRList.exists())
        return;

    const QStartUpFuncList list = [] {
        const auto locker = qt_scoped_lock(globalRoutinesMutex);
        // Unlike qt_call_post_routines, we don't empty the list, because
        // Q_COREAPP_STARTUP_FUNCTION is a macro, so the user expects
        // the function to be executed every time QCoreApplication is created.
        return *preRList;
    }();

    for (QtStartUpFunction f : list)
        f();
}

void Q_CORE_EXPORT qt_call_post_routines()
{
    if (!postRList.exists())
        return;

    forever {
        QVFuncList list;
        {
            // extract the current list and make the stored list empty
            const auto locker = qt_scoped_lock(globalRoutinesMutex);
            qSwap(*postRList, list);
        }

        if (list.isEmpty())
            break;
        for (QtCleanUpFunction f : std::as_const(list))
            f();
    }
}


#ifndef QT_NO_QOBJECT

// app starting up if false
Q_CONSTINIT bool QCoreApplicationPrivate::is_app_running = false;
 // app closing down if true
Q_CONSTINIT bool QCoreApplicationPrivate::is_app_closing = false;

qsizetype qGlobalPostedEventsCount()
{
    const QPostEventList &l = QThreadData::current()->postEventList;
    return l.size() - l.startOffset;
}

Q_CONSTINIT QAbstractEventDispatcher *QCoreApplicationPrivate::eventDispatcher = nullptr;

#endif // QT_NO_QOBJECT

Q_CONSTINIT QCoreApplication *QCoreApplication::self = nullptr;
Q_CONSTINIT uint QCoreApplicationPrivate::attribs =
    (1 << Qt::AA_SynthesizeMouseForUnhandledTouchEvents) |
    (1 << Qt::AA_SynthesizeMouseForUnhandledTabletEvents);

struct QCoreApplicationData {
    QCoreApplicationData() noexcept {
        applicationNameSet = false;
        applicationVersionSet = false;
    }
    ~QCoreApplicationData() {
#ifndef QT_NO_QOBJECT
        // cleanup the QAdoptedThread created for the main() thread
        if (auto *t = QCoreApplicationPrivate::theMainThread.loadAcquire()) {
            QThreadData *data = QThreadData::get2(t);
            data->deref(); // deletes the data and the adopted thread
        }
#endif
    }

    QString orgName, orgDomain;
    QString application; // application name, initially from argv[0], can then be modified.
    QString applicationVersion;
    bool applicationNameSet; // true if setApplicationName was called
    bool applicationVersionSet; // true if setApplicationVersion was called

#if QT_CONFIG(library)
    std::unique_ptr<QStringList> app_libpaths;
    std::unique_ptr<QStringList> manual_libpaths;
#endif

};

Q_GLOBAL_STATIC(QCoreApplicationData, coreappdata)

#ifndef QT_NO_QOBJECT
Q_CONSTINIT static bool quitLockEnabled = true;
#endif

#if defined(Q_OS_WIN)
// Check whether the command line arguments passed to QCoreApplication
// match those passed into main(), to see if the user has modified them
// before passing them on to us. We do this by comparing to the global
// __argv/__argc (MS extension). Deep comparison is required since
// argv/argc is rebuilt by our WinMain entrypoint.
static inline bool isArgvModified(int argc, char **argv)
{
    if (__argc != argc || !__argv /* wmain() */)
        return true;
    if (__argv == argv)
        return false;
    for (int a = 0; a < argc; ++a) {
        if (argv[a] != __argv[a] && strcmp(argv[a], __argv[a]))
            return true;
    }
    return false;
}

static inline bool contains(int argc, char **argv, const char *needle)
{
    for (int a = 0; a < argc; ++a) {
        if (!strcmp(argv[a], needle))
            return true;
    }
    return false;
}
#endif // Q_OS_WIN

QCoreApplicationPrivate::QCoreApplicationPrivate(int &aargc, char **aargv)
    :
#ifndef QT_NO_QOBJECT
      QObjectPrivate(),
#endif
      argc(aargc)
    , argv(aargv)
#if defined(Q_OS_WIN)
    , origArgc(0)
    , origArgv(nullptr)
#endif
    , application_type(QCoreApplicationPrivate::Tty)
#ifndef QT_NO_QOBJECT
    , in_exec(false)
    , aboutToQuitEmitted(false)
    , threadData_clean(false)
#else
    , q_ptr(nullptr)
#endif
{
    static const char *const empty = "";
    if (argc == 0 || argv == nullptr) {
        argc = 0;
        argv = const_cast<char **>(&empty);
    }
#if defined(Q_OS_WIN)
    if (!isArgvModified(argc, argv)) {
        origArgc = argc;
        origArgv = new char *[argc];
        std::copy(argv, argv + argc, QT_MAKE_CHECKED_ARRAY_ITERATOR(origArgv, argc));
    }
#endif // Q_OS_WIN

#ifndef QT_NO_QOBJECT
    QCoreApplicationPrivate::is_app_closing = false;

#  if defined(Q_OS_UNIX)
    if (Q_UNLIKELY(!setuidAllowed && (geteuid() != getuid())))
        qFatal("FATAL: The application binary appears to be running setuid, this is a security hole.");
#  endif // Q_OS_UNIX

    QThread *cur = QThread::currentThread(); // note: this may end up setting theMainThread!
    if (cur != theMainThread.loadAcquire())
        qWarning("WARNING: QApplication was not created in the main() thread.");
#endif
}

QCoreApplicationPrivate::~QCoreApplicationPrivate()
{
#ifndef QT_NO_QOBJECT
    cleanupThreadData();
#endif
#if defined(Q_OS_WIN)
    delete [] origArgv;
    if (consoleAllocated)
        FreeConsole();
#endif
    QCoreApplicationPrivate::clearApplicationFilePath();
}

#ifndef QT_NO_QOBJECT

void QCoreApplicationPrivate::cleanupThreadData()
{
    auto thisThreadData = threadData.loadRelaxed();

    if (thisThreadData && !threadData_clean) {
#if QT_CONFIG(thread)
        void *data = &thisThreadData->tls;
        QThreadStorageData::finish((void **)data);
#endif

        // need to clear the state of the mainData, just in case a new QCoreApplication comes along.
        const auto locker = qt_scoped_lock(thisThreadData->postEventList.mutex);
        for (const QPostEvent &pe : std::as_const(thisThreadData->postEventList)) {
            if (pe.event) {
                --pe.receiver->d_func()->postedEvents;
                pe.event->m_posted = false;
                delete pe.event;
            }
        }
        thisThreadData->postEventList.clear();
        thisThreadData->postEventList.recursion = 0;
        thisThreadData->quitNow = false;
        threadData_clean = true;
    }
}

void QCoreApplicationPrivate::createEventDispatcher()
{
    Q_Q(QCoreApplication);
    QThreadData *data = QThreadData::current();
    Q_ASSERT(!data->hasEventDispatcher());
    eventDispatcher = data->createEventDispatcher();
    eventDispatcher->setParent(q);
}

void QCoreApplicationPrivate::eventDispatcherReady()
{
}

Q_CONSTINIT QBasicAtomicPointer<QThread> QCoreApplicationPrivate::theMainThread = Q_BASIC_ATOMIC_INITIALIZER(nullptr);
QThread *QCoreApplicationPrivate::mainThread()
{
    Q_ASSERT(theMainThread.loadRelaxed() != nullptr);
    return theMainThread.loadRelaxed();
}

bool QCoreApplicationPrivate::threadRequiresCoreApplication()
{
    QThreadData *data = QThreadData::current(false);
    if (!data)
        return true;    // default setting
    return data->requiresCoreApplication;
}

void QCoreApplicationPrivate::checkReceiverThread(QObject *receiver)
{
    QThread *currentThread = QThread::currentThread();
    QThread *thr = receiver->thread();
    Q_ASSERT_X(currentThread == thr || !thr,
               "QCoreApplication::sendEvent",
               QString::asprintf("Cannot send events to objects owned by a different thread. "
                                 "Current thread 0x%p. Receiver '%ls' (of type '%s') was created in thread 0x%p",
                                 currentThread, qUtf16Printable(receiver->objectName()),
                                 receiver->metaObject()->className(), thr)
               .toLocal8Bit().data());
    Q_UNUSED(currentThread);
    Q_UNUSED(thr);
}

#endif // QT_NO_QOBJECT

void QCoreApplicationPrivate::appendApplicationPathToLibraryPaths()
{
#if QT_CONFIG(library)
    QStringList *app_libpaths = coreappdata()->app_libpaths.get();
    if (!app_libpaths)
        coreappdata()->app_libpaths.reset(app_libpaths = new QStringList);
    QString app_location = QCoreApplication::applicationFilePath();
    app_location.truncate(app_location.lastIndexOf(u'/'));
    app_location = QDir(app_location).canonicalPath();
    if (QFile::exists(app_location) && !app_libpaths->contains(app_location))
        app_libpaths->append(app_location);
#endif
}

QString qAppName()
{
    if (!QCoreApplicationPrivate::checkInstance("qAppName"))
        return QString();
    return QCoreApplication::instance()->d_func()->appName();
}

void QCoreApplicationPrivate::initConsole()
{
#ifdef Q_OS_WINDOWS
    const QString env = qEnvironmentVariable("QT_WIN_DEBUG_CONSOLE");
    if (env.isEmpty())
        return;
    if (env.compare(u"new"_s, Qt::CaseInsensitive) == 0) {
        if (AllocConsole() == FALSE)
            return;
        consoleAllocated = true;
    } else if (env.compare(u"attach"_s, Qt::CaseInsensitive) == 0) {
        // If the calling process is already attached to a console,
        // the error code returned is ERROR_ACCESS_DENIED.
        if (!::AttachConsole(ATTACH_PARENT_PROCESS) && ::GetLastError() != ERROR_ACCESS_DENIED)
            return;
    } else {
        // Unknown input, don't make any decision for the user.
        return;
    }
    // The std{in,out,err} handles are read-only, so we need to pass in dummies.
    FILE *in = nullptr;
    FILE *out = nullptr;
    FILE *err = nullptr;
    freopen_s(&in, "CONIN$", "r", stdin);
    freopen_s(&out, "CONOUT$", "w", stdout);
    freopen_s(&err, "CONOUT$", "w", stderr);
    // However, things wouldn't work if the runtime did not preserve the pointers.
    Q_ASSERT(in == stdin);
    Q_ASSERT(out == stdout);
    Q_ASSERT(err == stderr);
#endif
}

void QCoreApplicationPrivate::initLocale()
{
#if defined(QT_BOOTSTRAPPED)
    // Don't try to control bootstrap library locale or encoding.
#elif defined(Q_OS_UNIX)
    Q_CONSTINIT static bool qt_locale_initialized = false;
    if (qt_locale_initialized)
        return;
    qt_locale_initialized = true;

    // By default the portable "C"/POSIX locale is selected and active.
    // Apply the locale from the environment, via setlocale(), which will
    // read LC_ALL, LC_<category>, and LANG, in order (for each category).
    setlocale(LC_ALL, "");

    // Next, let's ensure that LC_CTYPE is UTF-8, since QStringConverter's
    // QLocal8Bit hard-codes this, and we need to be consistent.
#  if defined(Q_OS_INTEGRITY)
    setlocale(LC_CTYPE, "UTF-8");
#  elif defined(Q_OS_QNX)
    // QNX has no nl_langinfo, so we can't check.
    // FIXME: Shouldn't we still setlocale("UTF-8")?
#  elif defined(Q_OS_ANDROID) && __ANDROID_API__ < __ANDROID_API_O__
    // Android 6 still lacks nl_langinfo(), so we can't check.
    // FIXME: Shouldn't we still setlocale("UTF-8")?
#  else
    // std::string's SSO usually saves this the need to allocate:
    const std::string oldEncoding = nl_langinfo(CODESET);
    if (!Q_LIKELY(qstricmp(oldEncoding.data(), "UTF-8") == 0
                  || qstricmp(oldEncoding.data(), "utf8") == 0)) {
        const QByteArray oldLocale = setlocale(LC_ALL, nullptr);
        QByteArray newLocale;
        bool warnOnOverride = true;
#    if defined(Q_OS_DARWIN)
        // Don't warn unless the char encoding has been changed from the
        // default "C" encoding, or the user touched any of the locale
        // environment variables to force the "C" char encoding.
        warnOnOverride = qstrcmp(setlocale(LC_CTYPE, nullptr), "C") != 0
            || getenv("LC_ALL") || getenv("LC_CTYPE") || getenv("LANG");

        // No need to try language or region specific CTYPEs, as they
        // all point back to the same generic UTF-8 CTYPE.
        newLocale = setlocale(LC_CTYPE, "UTF-8");
#    else
        newLocale = setlocale(LC_CTYPE, nullptr);
        if (qsizetype dot = newLocale.indexOf('.'); dot != -1)
            newLocale.truncate(dot);    // remove encoding, if any
        if (qsizetype at = newLocale.indexOf('@'); at != -1)
            newLocale.truncate(at);     // remove variant, as the old de_DE@euro
        newLocale += ".UTF-8";
        newLocale = setlocale(LC_CTYPE, newLocale);

        // If that locale doesn't exist, try some fallbacks:
        if (newLocale.isEmpty())
            newLocale = setlocale(LC_CTYPE, "C.UTF-8");
        if (newLocale.isEmpty())
            newLocale = setlocale(LC_CTYPE, "C.utf8");
#    endif

        if (newLocale.isEmpty()) {
            // Failed to set a UTF-8 locale.
            qWarning("Detected locale \"%s\" with character encoding \"%s\", which is not UTF-8.\n"
                     "Qt depends on a UTF-8 locale, but has failed to switch to one.\n"
                     "If this causes problems, reconfigure your locale. See the locale(1) manual\n"
                     "for more information.", oldLocale.constData(), oldEncoding.data());
        } else if (warnOnOverride) {
            // Let the user know we over-rode their configuration.
            qWarning("Detected locale \"%s\" with character encoding \"%s\", which is not UTF-8.\n"
                     "Qt depends on a UTF-8 locale, and has switched to \"%s\" instead.\n"
                     "If this causes problems, reconfigure your locale. See the locale(1) manual\n"
                     "for more information.",
                     oldLocale.constData(), oldEncoding.data(), newLocale.constData());
        }
    }
#  endif // Platform choice
#endif // Unix
}


/*!
    \class QCoreApplication
    \inmodule QtCore
    \brief The QCoreApplication class provides an event loop for Qt
    applications without UI.

    This class is used by non-GUI applications to provide their event
    loop. For non-GUI application that uses Qt, there should be exactly
    one QCoreApplication object. For GUI applications, see
    QGuiApplication. For applications that use the Qt Widgets module,
    see QApplication.

    QCoreApplication contains the main event loop, where all events
    from the operating system (e.g., timer and network events) and
    other sources are processed and dispatched. It also handles the
    application's initialization and finalization, as well as
    system-wide and application-wide settings.

    \section1 The Event Loop and Event Handling

    The event loop is started with a call to exec(). Long-running
    operations can call processEvents() to keep the application
    responsive.

    In general, we recommend that you create a QCoreApplication,
    QGuiApplication or a QApplication object in your \c main()
    function as early as possible. exec() will not return until
    the event loop exits; e.g., when quit() is called.

    Several static convenience functions are also provided. The
    QCoreApplication object is available from instance(). Events can
    be sent with sendEvent() or posted to an event queue with postEvent().
    Pending events can be removed with removePostedEvents() or dispatched
    with sendPostedEvents().

    The class provides a quit() slot and an aboutToQuit() signal.

    \section1 Application and Library Paths

    An application has an applicationDirPath() and an
    applicationFilePath(). Library paths (see QLibrary) can be retrieved
    with libraryPaths() and manipulated by setLibraryPaths(), addLibraryPath(),
    and removeLibraryPath().

    \section1 Internationalization and Translations

    Translation files can be added or removed
    using installTranslator() and removeTranslator(). Application
    strings can be translated using translate(). The QObject::tr()
    function is implemented in terms of translate().

    \section1 Accessing Command Line Arguments

    The command line arguments which are passed to QCoreApplication's
    constructor should be accessed using the arguments() function.

    \note QCoreApplication removes option \c -qmljsdebugger="...". It parses the
    argument of \c qmljsdebugger, and then removes this option plus its argument.

    For more advanced command line option handling, create a QCommandLineParser.

    \section1 Locale Settings

    On Unix/Linux Qt is configured to use the system locale settings by
    default. This can cause a conflict when using POSIX functions, for
    instance, when converting between data types such as floats and
    strings, since the notation may differ between locales. To get
    around this problem, call the POSIX function \c{setlocale(LC_NUMERIC,"C")}
    right after initializing QApplication, QGuiApplication or QCoreApplication
    to reset the locale that is used for number formatting to "C"-locale.

    \sa QGuiApplication, QAbstractEventDispatcher, QEventLoop,
    {Producer and Consumer using Semaphores},
    {Producer and Consumer using Wait Conditions}
*/

/*!
    \fn static QCoreApplication *QCoreApplication::instance()

    Returns a pointer to the application's QCoreApplication (or
    QGuiApplication/QApplication) instance.

    If no instance has been allocated, \nullptr is returned.
*/

/*!
    \internal
 */
QCoreApplication::QCoreApplication(QCoreApplicationPrivate &p)
#ifdef QT_NO_QOBJECT
    : d_ptr(&p)
#else
    : QObject(p, nullptr)
#endif
{
    d_func()->q_ptr = this;
    // note: it is the subclasses' job to call
    // QCoreApplicationPrivate::eventDispatcher->startingUp();
}

/*!
    Constructs a Qt core application. Core applications are applications without
    a graphical user interface. Such applications are used at the console or as
    server processes.

    The \a argc and \a argv arguments are processed by the application,
    and made available in a more convenient form by the arguments()
    function.

    \warning The data referred to by \a argc and \a argv must stay valid
    for the entire lifetime of the QCoreApplication object. In addition,
    \a argc must be greater than zero and \a argv must contain at least
    one valid character string.
*/
QCoreApplication::QCoreApplication(int &argc, char **argv
#ifndef Q_QDOC
                                   , int
#endif
                                   )
#ifdef QT_NO_QOBJECT
    : d_ptr(new QCoreApplicationPrivate(argc, argv))
#else
    : QObject(*new QCoreApplicationPrivate(argc, argv))
#endif
{
    d_func()->q_ptr = this;
    d_func()->init();
#ifndef QT_NO_QOBJECT
    QCoreApplicationPrivate::eventDispatcher->startingUp();
#endif
}

/*!
  \enum QCoreApplication::anonymous
  \internal

  \value ApplicationFlags QT_VERSION
*/

void Q_TRACE_INSTRUMENT(qtcore) QCoreApplicationPrivate::init()
{
    Q_TRACE_SCOPE(QCoreApplicationPrivate_init);

#if defined(Q_OS_MACOS)
    QMacAutoReleasePool pool;
#endif

    Q_Q(QCoreApplication);

    initConsole();

    initLocale();

    Q_ASSERT_X(!QCoreApplication::self, "QCoreApplication", "there should be only one application object");
    QCoreApplication::self = q;

#if QT_CONFIG(thread)
#ifdef Q_OS_WASM
    emscripten::val hardwareConcurrency = emscripten::val::global("navigator")["hardwareConcurrency"];
    if (hardwareConcurrency.isUndefined())
        QThreadPrivate::idealThreadCount = 2;
    else
        QThreadPrivate::idealThreadCount = hardwareConcurrency.as<int>();
#endif
#endif

    // Store app name/version (so they're still available after QCoreApplication is destroyed)
    if (!coreappdata()->applicationNameSet)
        coreappdata()->application = appName();

    if (!coreappdata()->applicationVersionSet)
        coreappdata()->applicationVersion = appVersion();

#if defined(Q_OS_ANDROID)
    // We've deferred initializing the logging registry due to not being
    // able to guarantee that logging happened on the same thread as the
    // Qt main thread, but now that the Qt main thread is set up, we can
    // enable categorized logging.
    QLoggingRegistry::instance()->initializeRules();
#endif

#if QT_CONFIG(library)
    // Reset the lib paths, so that they will be recomputed, taking the availability of argv[0]
    // into account. If necessary, recompute right away and replay the manual changes on top of the
    // new lib paths.
    QStringList *appPaths = coreappdata()->app_libpaths.release();
    QStringList *manualPaths = coreappdata()->manual_libpaths.release();
    if (appPaths) {
        if (manualPaths) {
            // Replay the delta. As paths can only be prepended to the front or removed from
            // anywhere in the list, we can just linearly scan the lists and find the items that
            // have been removed. Once the original list is exhausted we know all the remaining
            // items have been added.
            QStringList newPaths(q->libraryPaths());
            for (qsizetype i = manualPaths->size(), j = appPaths->size(); i > 0 || j > 0; qt_noop()) {
                if (--j < 0) {
                    newPaths.prepend((*manualPaths)[--i]);
                } else if (--i < 0) {
                    newPaths.removeAll((*appPaths)[j]);
                } else if ((*manualPaths)[i] != (*appPaths)[j]) {
                    newPaths.removeAll((*appPaths)[j]);
                    ++i; // try again with next item.
                }
            }
            delete manualPaths;
            coreappdata()->manual_libpaths.reset(new QStringList(newPaths));
        }
        delete appPaths;
    }
#endif

#ifndef QT_NO_QOBJECT
    // use the event dispatcher created by the app programmer (if any)
    Q_ASSERT(!eventDispatcher);
    auto thisThreadData = threadData.loadRelaxed();
    eventDispatcher = thisThreadData->eventDispatcher.loadRelaxed();

    // otherwise we create one
    if (!eventDispatcher)
        createEventDispatcher();
    Q_ASSERT(eventDispatcher);

    if (!eventDispatcher->parent()) {
        eventDispatcher->moveToThread(thisThreadData->thread.loadAcquire());
        eventDispatcher->setParent(q);
    }

    thisThreadData->eventDispatcher = eventDispatcher;
    eventDispatcherReady();
#endif

    processCommandLineArguments();

    qt_call_pre_routines();
    qt_startup_hook();
#ifndef QT_BOOTSTRAPPED
    QtPrivate::initBindingStatusThreadId();
    if (Q_UNLIKELY(qtHookData[QHooks::Startup]))
        reinterpret_cast<QHooks::StartupCallback>(qtHookData[QHooks::Startup])();
#endif

#ifndef QT_NO_QOBJECT
    is_app_running = true; // No longer starting up.
#endif
}

/*!
    Destroys the QCoreApplication object.
*/
QCoreApplication::~QCoreApplication()
{
    preRoutinesCalled = false;

    qt_call_post_routines();

    self = nullptr;
#ifndef QT_NO_QOBJECT
    QCoreApplicationPrivate::is_app_closing = true;
    QCoreApplicationPrivate::is_app_running = false;
#endif

#if QT_CONFIG(thread)
    // Synchronize and stop the global thread pool threads.
    QThreadPool *globalThreadPool = nullptr;
    QThreadPool *guiThreadPool = nullptr;
    QT_TRY {
        globalThreadPool = QThreadPool::globalInstance();
        guiThreadPool = QThreadPoolPrivate::qtGuiInstance();
    } QT_CATCH (...) {
        // swallow the exception, since destructors shouldn't throw
    }
    if (globalThreadPool) {
        globalThreadPool->waitForDone();
        delete globalThreadPool;
    }
    if (guiThreadPool) {
        guiThreadPool->waitForDone();
        delete guiThreadPool;
    }
#endif

#ifndef QT_NO_QOBJECT
    d_func()->threadData.loadRelaxed()->eventDispatcher = nullptr;
    if (QCoreApplicationPrivate::eventDispatcher)
        QCoreApplicationPrivate::eventDispatcher->closingDown();
    QCoreApplicationPrivate::eventDispatcher = nullptr;
#endif

#if QT_CONFIG(library)
    coreappdata()->app_libpaths.reset();
    coreappdata()->manual_libpaths.reset();
#endif
}

/*!
    \since 5.3

    Allows the application to run setuid on UNIX platforms if \a allow
    is true.

    If \a allow is false (the default) and Qt detects the application is
    running with an effective user id different than the real user id,
    the application will be aborted when a QCoreApplication instance is
    created.

    Qt is not an appropriate solution for setuid programs due to its
    large attack surface. However some applications may be required
    to run in this manner for historical reasons. This flag will
    prevent Qt from aborting the application when this is detected,
    and must be set before a QCoreApplication instance is created.

    \note It is strongly recommended not to enable this option since
    it introduces security risks.
*/
void QCoreApplication::setSetuidAllowed(bool allow)
{
    QCoreApplicationPrivate::setuidAllowed = allow;
}

/*!
    \since 5.3

    Returns true if the application is allowed to run setuid on UNIX
    platforms.

    \sa QCoreApplication::setSetuidAllowed()
*/
bool QCoreApplication::isSetuidAllowed()
{
    return QCoreApplicationPrivate::setuidAllowed;
}


/*!
    Sets the attribute \a attribute if \a on is true;
    otherwise clears the attribute.

    \note Some application attributes must be set \b before creating a
    QCoreApplication instance. Refer to the Qt::ApplicationAttribute
    documentation for more information.

    \sa testAttribute()
*/
void QCoreApplication::setAttribute(Qt::ApplicationAttribute attribute, bool on)
{
    if (on)
        QCoreApplicationPrivate::attribs |= 1 << attribute;
    else
        QCoreApplicationPrivate::attribs &= ~(1 << attribute);
#if defined(QT_NO_QOBJECT)
    if (Q_UNLIKELY(qApp)) {
#else
    if (Q_UNLIKELY(QCoreApplicationPrivate::is_app_running)) {
#endif
        switch (attribute) {
            case Qt::AA_PluginApplication:
            case Qt::AA_UseDesktopOpenGL:
            case Qt::AA_UseOpenGLES:
            case Qt::AA_UseSoftwareOpenGL:
            case Qt::AA_ShareOpenGLContexts:
#ifdef QT_BOOTSTRAPPED
                qWarning("Attribute %d must be set before QCoreApplication is created.",
                         attribute);
#else
                qWarning("Attribute Qt::%s must be set before QCoreApplication is created.",
                         QMetaEnum::fromType<Qt::ApplicationAttribute>().valueToKey(attribute));
#endif
                break;
            default:
                break;
        }
    }
}

/*!
  Returns \c true if attribute \a attribute is set;
  otherwise returns \c false.

  \sa setAttribute()
 */
bool QCoreApplication::testAttribute(Qt::ApplicationAttribute attribute)
{
    return QCoreApplicationPrivate::testAttribute(attribute);
}

#ifndef QT_NO_QOBJECT

/*!
    \property QCoreApplication::quitLockEnabled

    \brief Whether the use of the QEventLoopLocker feature can cause the
    application to quit.

    The default is \c true.

    \sa QEventLoopLocker
*/

bool QCoreApplication::isQuitLockEnabled()
{
    return quitLockEnabled;
}

static bool doNotify(QObject *, QEvent *);

void QCoreApplication::setQuitLockEnabled(bool enabled)
{
    quitLockEnabled = enabled;
}

/*!
  \internal
  \since 5.6

  This function is here to make it possible for Qt extensions to
  hook into event notification without subclassing QApplication.
*/
bool QCoreApplication::notifyInternal2(QObject *receiver, QEvent *event)
{
    bool selfRequired = QCoreApplicationPrivate::threadRequiresCoreApplication();
    if (!self && selfRequired)
        return false;

    // Make it possible for Qt Script to hook into events even
    // though QApplication is subclassed...
    bool result = false;
    void *cbdata[] = { receiver, event, &result };
    if (QInternal::activateCallbacks(QInternal::EventNotifyCallback, cbdata)) {
        return result;
    }

    // Qt enforces the rule that events can only be sent to objects in
    // the current thread, so receiver->d_func()->threadData is
    // equivalent to QThreadData::current(), just without the function
    // call overhead.
    QObjectPrivate *d = receiver->d_func();
    QThreadData *threadData = d->threadData.loadAcquire();
    QScopedScopeLevelCounter scopeLevelCounter(threadData);
    if (!selfRequired)
        return doNotify(receiver, event);
    return self->notify(receiver, event);
}

/*!
    \internal
    \since 5.10

    Forwards the \a event to the \a receiver, using the spontaneous
    state of the \a originatingEvent if specified.
*/
bool QCoreApplication::forwardEvent(QObject *receiver, QEvent *event, QEvent *originatingEvent)
{
    if (event && originatingEvent)
        event->m_spont = originatingEvent->m_spont;

    return notifyInternal2(receiver, event);
}

/*!
  Sends \a event to \a receiver: \a {receiver}->event(\a event).
  Returns the value that is returned from the receiver's event
  handler. Note that this function is called for all events sent to
  any object in any thread.

  For certain types of events (e.g. mouse and key events),
  the event will be propagated to the receiver's parent and so on up to
  the top-level object if the receiver is not interested in the event
  (i.e., it returns \c false).

  There are five different ways that events can be processed;
  reimplementing this virtual function is just one of them. All five
  approaches are listed below:
  \list 1
  \li Reimplementing \l {QWidget::}{paintEvent()}, \l {QWidget::}{mousePressEvent()} and so
  on. This is the most common, easiest, and least powerful way.

  \li Reimplementing this function. This is very powerful, providing
  complete control; but only one subclass can be active at a time.

  \li Installing an event filter on QCoreApplication::instance(). Such
  an event filter is able to process all events for all widgets, so
  it's just as powerful as reimplementing notify(); furthermore, it's
  possible to have more than one application-global event filter.
  Global event filters even see mouse events for
  \l{QWidget::isEnabled()}{disabled widgets}. Note that application
  event filters are only called for objects that live in the main
  thread.

  \li Reimplementing QObject::event() (as QWidget does). If you do
  this you get Tab key presses, and you get to see the events before
  any widget-specific event filters.

  \li Installing an event filter on the object. Such an event filter gets all
  the events, including Tab and Shift+Tab key press events, as long as they
  do not change the focus widget.
  \endlist

  \b{Future direction:} This function will not be called for objects that live
  outside the main thread in Qt 6. Applications that need that functionality
  should find other solutions for their event inspection needs in the meantime.
  The change may be extended to the main thread, causing this function to be
  deprecated.

  \warning If you override this function, you must ensure all threads that
  process events stop doing so before your application object begins
  destruction. This includes threads started by other libraries that you may be
  using, but does not apply to Qt's own threads.

  \sa QObject::event(), installNativeEventFilter()
*/

bool QCoreApplication::notify(QObject *receiver, QEvent *event)
{
    Q_ASSERT(receiver);
    Q_ASSERT(event);

    // no events are delivered after ~QCoreApplication() has started
    if (QCoreApplicationPrivate::is_app_closing)
        return true;
    return doNotify(receiver, event);
}

static bool doNotify(QObject *receiver, QEvent *event)
{
    Q_ASSERT(event);

    // ### Qt 7: turn into an assert
    if (receiver == nullptr) {                        // serious error
        qWarning("QCoreApplication::notify: Unexpected null receiver");
        return true;
    }

#ifndef QT_NO_DEBUG
    QCoreApplicationPrivate::checkReceiverThread(receiver);
#endif

    return receiver->isWidgetType() ? false : QCoreApplicationPrivate::notify_helper(receiver, event);
}

bool QCoreApplicationPrivate::sendThroughApplicationEventFilters(QObject *receiver, QEvent *event)
{
    // We can't access the application event filters outside of the main thread (race conditions)
    Q_ASSERT(receiver->d_func()->threadData.loadAcquire()->thread.loadRelaxed() == mainThread());

    if (extraData) {
        // application event filters are only called for objects in the GUI thread
        for (qsizetype i = 0; i < extraData->eventFilters.size(); ++i) {
            QObject *obj = extraData->eventFilters.at(i);
            if (!obj)
                continue;
            if (obj->d_func()->threadData.loadRelaxed() != threadData.loadRelaxed()) {
                qWarning("QCoreApplication: Application event filter cannot be in a different thread.");
                continue;
            }
            if (obj->eventFilter(receiver, event))
                return true;
        }
    }
    return false;
}

bool QCoreApplicationPrivate::sendThroughObjectEventFilters(QObject *receiver, QEvent *event)
{
    if (receiver != QCoreApplication::instance() && receiver->d_func()->extraData) {
        for (qsizetype i = 0; i < receiver->d_func()->extraData->eventFilters.size(); ++i) {
            QObject *obj = receiver->d_func()->extraData->eventFilters.at(i);
            if (!obj)
                continue;
            if (obj->d_func()->threadData.loadRelaxed() != receiver->d_func()->threadData.loadRelaxed()) {
                qWarning("QCoreApplication: Object event filter cannot be in a different thread.");
                continue;
            }
            if (obj->eventFilter(receiver, event))
                return true;
        }
    }
    return false;
}

/*!
  \internal

  Helper function called by QCoreApplicationPrivate::notify() and qapplication.cpp
 */
bool QCoreApplicationPrivate::notify_helper(QObject *receiver, QEvent * event)
{
    // Note: when adjusting the tracepoints in here
    // consider adjusting QApplicationPrivate::notify_helper too.
    Q_TRACE(QCoreApplication_notify_entry, receiver, event, event->type());
    bool consumed = false;
    bool filtered = false;
    Q_TRACE_EXIT(QCoreApplication_notify_exit, consumed, filtered);

    // send to all application event filters (only does anything in the main thread)
    if (QCoreApplication::self
            && receiver->d_func()->threadData.loadRelaxed()->thread.loadAcquire() == mainThread()
            && QCoreApplication::self->d_func()->sendThroughApplicationEventFilters(receiver, event)) {
        filtered = true;
        return filtered;
    }
    // send to all receiver event filters
    if (sendThroughObjectEventFilters(receiver, event)) {
        filtered = true;
        return filtered;
    }

    // deliver the event
    consumed = receiver->event(event);
    return consumed;
}

/*!
  Returns \c true if an application object has not been created yet;
  otherwise returns \c false.

  \sa closingDown()
*/

bool QCoreApplication::startingUp()
{
    return !QCoreApplicationPrivate::is_app_running;
}

/*!
  Returns \c true if the application objects are being destroyed;
  otherwise returns \c false.

  \sa startingUp()
*/

bool QCoreApplication::closingDown()
{
    return QCoreApplicationPrivate::is_app_closing;
}


/*!
    Processes some pending events for the calling thread according to
    the specified \a flags.

    Use of this function is discouraged. Instead, prefer to move long
    operations out of the GUI thread into an auxiliary one and to completely
    avoid nested event loop processing. If event processing is really
    necessary, consider using \l QEventLoop instead.

    In the event that you are running a local loop which calls this function
    continuously, without an event loop, the
    \l{QEvent::DeferredDelete}{DeferredDelete} events will
    not be processed. This can affect the behaviour of widgets,
    e.g. QToolTip, that rely on \l{QEvent::DeferredDelete}{DeferredDelete}
    events to function properly. An alternative would be to call
    \l{QCoreApplication::sendPostedEvents()}{sendPostedEvents()} from
    within that local loop.

    Calling this function processes events only for the calling thread,
    and returns after all available events have been processed. Available
    events are events queued before the function call. This means that
    events that are posted while the function runs will be queued until
    a later round of event processing.

    \threadsafe

    \sa exec(), QTimer, QEventLoop::processEvents(), sendPostedEvents()
*/
void QCoreApplication::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    QThreadData *data = QThreadData::current();
    if (!data->hasEventDispatcher())
        return;
    data->eventDispatcher.loadRelaxed()->processEvents(flags);
}

/*!
    \overload processEvents()

    Processes pending events for the calling thread for \a ms
    milliseconds or until there are no more events to process,
    whichever is shorter.

    Use of this function is discouraged. Instead, prefer to move long
    operations out of the GUI thread into an auxiliary one and to completely
    avoid nested event loop processing. If event processing is really
    necessary, consider using \l QEventLoop instead.

    Calling this function processes events only for the calling thread.

    \note Unlike the \l{QCoreApplication::processEvents(QEventLoop::ProcessEventsFlags flags)}{processEvents()}
    overload, this function also processes events that are posted while the function runs.

    \note All events that were queued before the timeout will be processed,
    however long it takes.

    \threadsafe

    \sa exec(), QTimer, QEventLoop::processEvents()
*/
void QCoreApplication::processEvents(QEventLoop::ProcessEventsFlags flags, int ms)
{
    // ### TODO: consider splitting this method into a public and a private
    //           one, so that a user-invoked processEvents can be detected
    //           and handled properly.
    QThreadData *data = QThreadData::current();
    if (!data->hasEventDispatcher())
        return;
    QElapsedTimer start;
    start.start();
    while (data->eventDispatcher.loadRelaxed()->processEvents(flags & ~QEventLoop::WaitForMoreEvents)) {
        if (start.elapsed() > ms)
            break;
    }
}

/*****************************************************************************
  Main event loop wrappers
 *****************************************************************************/

/*!
    Enters the main event loop and waits until exit() is called.  Returns
    the value that was passed to exit() (which is 0 if exit() is called via
    quit()).

    It is necessary to call this function to start event handling. The
    main event loop receives events from the window system and
    dispatches these to the application widgets.

    To make your application perform idle processing (by executing a
    special function whenever there are no pending events), use a
    QTimer with 0 timeout. More advanced idle processing schemes can
    be achieved using processEvents().

    We recommend that you connect clean-up code to the
    \l{QCoreApplication::}{aboutToQuit()} signal, instead of putting it in
    your application's \c{main()} function because on some platforms the
    exec() call may not return. For example, on Windows
    when the user logs off, the system terminates the process after Qt
    closes all top-level windows. Hence, there is no guarantee that the
    application will have time to exit its event loop and execute code at
    the end of the \c{main()} function after the exec()
    call.

    \sa quit(), exit(), processEvents(), QApplication::exec()
*/
int QCoreApplication::exec()
{
    if (!QCoreApplicationPrivate::checkInstance("exec"))
        return -1;

    QThreadData *threadData = self->d_func()->threadData.loadAcquire();
    if (threadData != QThreadData::current()) {
        qWarning("%s::exec: Must be called from the main thread", self->metaObject()->className());
        return -1;
    }
    if (!threadData->eventLoops.isEmpty()) {
        qWarning("QCoreApplication::exec: The event loop is already running");
        return -1;
    }

    threadData->quitNow = false;
    QEventLoop eventLoop;
    self->d_func()->in_exec = true;
    self->d_func()->aboutToQuitEmitted = false;
    int returnCode = eventLoop.exec(QEventLoop::ApplicationExec);
    threadData->quitNow = false;

    if (self)
        self->d_func()->execCleanup();

    return returnCode;
}


// Cleanup after eventLoop is done executing in QCoreApplication::exec().
// This is for use cases in which QCoreApplication is instantiated by a
// library and not by an application executable, for example, Active X
// servers.

void QCoreApplicationPrivate::execCleanup()
{
    threadData.loadRelaxed()->quitNow = false;
    in_exec = false;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}


/*!
  Tells the application to exit with a return code.

    After this function has been called, the application leaves the
    main event loop and returns from the call to exec(). The exec()
    function returns \a returnCode. If the event loop is not running,
    this function does nothing.

  By convention, a \a returnCode of 0 means success, and any non-zero
  value indicates an error.

  It's good practice to always connect signals to this slot using a
  \l{Qt::}{QueuedConnection}. If a signal connected (non-queued) to this slot
  is emitted before control enters the main event loop (such as before
  "int main" calls \l{QCoreApplication::}{exec()}), the slot has no effect
  and the application never exits. Using a queued connection ensures that the
  slot will not be invoked until after control enters the main event loop.

  Note that unlike the C library function of the same name, this
  function \e does return to the caller -- it is event processing that
  stops.

  Note also that this function is not thread-safe. It should be called only
  from the main thread (the thread that the QCoreApplication object is
  processing events on). To ask the application to exit from another thread,
  either use QCoreApplication::quit() or instead call this function from the
  main thread with QMetaMethod::invokeMethod().

  \sa quit(), exec()
*/
void QCoreApplication::exit(int returnCode)
{
    if (!self)
        return;
    QCoreApplicationPrivate *d = self->d_func();
    if (!d->aboutToQuitEmitted) {
        emit self->aboutToQuit(QCoreApplication::QPrivateSignal());
        d->aboutToQuitEmitted = true;
    }
    QThreadData *data = d->threadData.loadRelaxed();
    data->quitNow = true;
    for (qsizetype i = 0; i < data->eventLoops.size(); ++i) {
        QEventLoop *eventLoop = data->eventLoops.at(i);
        eventLoop->exit(returnCode);
    }
}

/*****************************************************************************
  QCoreApplication management of posted events
 *****************************************************************************/

#ifndef QT_NO_QOBJECT
/*!
    \fn bool QCoreApplication::sendEvent(QObject *receiver, QEvent *event)

    Sends event \a event directly to receiver \a receiver, using the
    notify() function. Returns the value that was returned from the
    event handler.

    The event is \e not deleted when the event has been sent. The normal
    approach is to create the event on the stack, for example:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 0

    \sa postEvent(), notify()
*/
bool QCoreApplication::sendEvent(QObject *receiver, QEvent *event)
{
    Q_ASSERT_X(receiver, "QCoreApplication::sendEvent", "Unexpected null receiver");
    Q_ASSERT_X(event, "QCoreApplication::sendEvent", "Unexpected null event");

    Q_TRACE(QCoreApplication_sendEvent, receiver, event, event->type());

    event->m_spont = false;
    return notifyInternal2(receiver, event);
}

/*!
    \internal
*/
bool QCoreApplication::sendSpontaneousEvent(QObject *receiver, QEvent *event)
{
    Q_ASSERT_X(receiver, "QCoreApplication::sendSpontaneousEvent", "Unexpected null receiver");
    Q_ASSERT_X(event, "QCoreApplication::sendSpontaneousEvent", "Unexpected null event");

    Q_TRACE(QCoreApplication_sendSpontaneousEvent, receiver, event, event->type());

    event->m_spont = true;
    return notifyInternal2(receiver, event);
}

#endif // QT_NO_QOBJECT

QCoreApplicationPrivate::QPostEventListLocker QCoreApplicationPrivate::lockThreadPostEventList(QObject *object)
{
    QPostEventListLocker locker;

    if (!object) {
        locker.threadData = QThreadData::current();
        locker.locker = qt_unique_lock(locker.threadData->postEventList.mutex);
        return locker;
    }

    auto &threadData = QObjectPrivate::get(object)->threadData;

    // if object has moved to another thread, follow it
    for (;;) {
        // synchronizes with the storeRelease in QObject::moveToThread
        locker.threadData = threadData.loadAcquire();
        if (!locker.threadData) {
            // destruction in progress
            return locker;
        }

        auto temporaryLocker = qt_unique_lock(locker.threadData->postEventList.mutex);
        if (locker.threadData == threadData.loadAcquire()) {
            locker.locker = std::move(temporaryLocker);
            break;
        }
    }

    Q_ASSERT(locker.threadData);
    return locker;
}

/*!
    \since 4.3

    Adds the event \a event, with the object \a receiver as the
    receiver of the event, to an event queue and returns immediately.

    The event must be allocated on the heap since the post event queue
    will take ownership of the event and delete it once it has been
    posted.  It is \e {not safe} to access the event after
    it has been posted.

    When control returns to the main event loop, all events that are
    stored in the queue will be sent using the notify() function.

    Events are sorted in descending \a priority order, i.e. events
    with a high \a priority are queued before events with a lower \a
    priority. The \a priority can be any integer value, i.e. between
    INT_MAX and INT_MIN, inclusive; see Qt::EventPriority for more
    details. Events with equal \a priority will be processed in the
    order posted.

    \threadsafe

    \sa sendEvent(), notify(), sendPostedEvents(), Qt::EventPriority
*/
void QCoreApplication::postEvent(QObject *receiver, QEvent *event, int priority)
{
    Q_ASSERT_X(event, "QCoreApplication::postEvent", "Unexpected null event");

    Q_TRACE_SCOPE(QCoreApplication_postEvent, receiver, event, event->type());

    // ### Qt 7: turn into an assert
    if (receiver == nullptr) {
        qWarning("QCoreApplication::postEvent: Unexpected null receiver");
        delete event;
        return;
    }

    auto locker = QCoreApplicationPrivate::lockThreadPostEventList(receiver);
    if (!locker.threadData) {
        // posting during destruction? just delete the event to prevent a leak
        delete event;
        return;
    }

    QThreadData *data = locker.threadData;

    // if this is one of the compressible events, do compression
    if (receiver->d_func()->postedEvents
        && self && self->compressEvent(event, receiver, &data->postEventList)) {
        Q_TRACE(QCoreApplication_postEvent_event_compressed, receiver, event);
        return;
    }

    if (event->type() == QEvent::DeferredDelete)
        receiver->d_ptr->deleteLaterCalled = true;

    if (event->type() == QEvent::DeferredDelete && data == QThreadData::current()) {
        // remember the current running eventloop for DeferredDelete
        // events posted in the receiver's thread.

        // Events sent by non-Qt event handlers (such as glib) may not
        // have the scopeLevel set correctly. The scope level makes sure that
        // code like this:
        //     foo->deleteLater();
        //     qApp->processEvents(); // without passing QEvent::DeferredDelete
        // will not cause "foo" to be deleted before returning to the event loop.

        // If the scope level is 0 while loopLevel != 0, we are called from a
        // non-conformant code path, and our best guess is that the scope level
        // should be 1. (Loop level 0 is special: it means that no event loops
        // are running.)
        int loopLevel = data->loopLevel;
        int scopeLevel = data->scopeLevel;
        if (scopeLevel == 0 && loopLevel != 0)
            scopeLevel = 1;
        static_cast<QDeferredDeleteEvent *>(event)->level = loopLevel + scopeLevel;
    }

    // delete the event on exceptions to protect against memory leaks till the event is
    // properly owned in the postEventList
    std::unique_ptr<QEvent> eventDeleter(event);
    Q_TRACE(QCoreApplication_postEvent_event_posted, receiver, event, event->type());
    data->postEventList.addEvent(QPostEvent(receiver, event, priority));
    Q_UNUSED(eventDeleter.release());
    event->m_posted = true;
    ++receiver->d_func()->postedEvents;
    data->canWait = false;
    locker.unlock();

    QAbstractEventDispatcher* dispatcher = data->eventDispatcher.loadAcquire();
    if (dispatcher)
        dispatcher->wakeUp();
}

/*!
  \internal
  Returns \c true if \a event was compressed away (possibly deleted) and should not be added to the list.
*/
bool QCoreApplication::compressEvent(QEvent *event, QObject *receiver, QPostEventList *postedEvents)
{
    Q_ASSERT(event);
    Q_ASSERT(receiver);
    Q_ASSERT(postedEvents);

#ifdef Q_OS_WIN
    // compress posted timers to this object.
    if (event->type() == QEvent::Timer && receiver->d_func()->postedEvents > 0) {
        int timerId = ((QTimerEvent *) event)->timerId();
        for (const QPostEvent &e : std::as_const(*postedEvents)) {
            if (e.receiver == receiver && e.event && e.event->type() == QEvent::Timer
                && ((QTimerEvent *) e.event)->timerId() == timerId) {
                delete event;
                return true;
            }
        }
        return false;
    }
#endif

    if (event->type() == QEvent::DeferredDelete) {
        if (receiver->d_ptr->deleteLaterCalled) {
            // there was a previous DeferredDelete event, so we can drop the new one
            delete event;
            return true;
        }
        // deleteLaterCalled is set to true in postedEvents when queueing the very first
        // deferred deletion event.
        return false;
    }

    if (event->type() == QEvent::Quit && receiver->d_func()->postedEvents > 0) {
        for (const QPostEvent &cur : std::as_const(*postedEvents)) {
            if (cur.receiver != receiver
                    || cur.event == nullptr
                    || cur.event->type() != event->type())
                continue;
            // found an event for this receiver
            delete event;
            return true;
        }
    }

    return false;
}

/*!
  Immediately dispatches all events which have been previously queued
  with QCoreApplication::postEvent() and which are for the object \a
  receiver and have the event type \a event_type.

  Events from the window system are \e not dispatched by this
  function, but by processEvents().

  If \a receiver is \nullptr, the events of \a event_type are sent for
  all objects. If \a event_type is 0, all the events are sent for
  \a receiver.

  \note This method must be called from the thread in which its QObject
  parameter, \a receiver, lives.

  \sa postEvent()
*/
void QCoreApplication::sendPostedEvents(QObject *receiver, int event_type)
{
    // ### TODO: consider splitting this method into a public and a private
    //           one, so that a user-invoked sendPostedEvents can be detected
    //           and handled properly.
    QThreadData *data = QThreadData::current();

    QCoreApplicationPrivate::sendPostedEvents(receiver, event_type, data);
}

void QCoreApplicationPrivate::sendPostedEvents(QObject *receiver, int event_type,
                                               QThreadData *data)
{
    if (event_type == -1) {
        // we were called by an obsolete event dispatcher.
        event_type = 0;
    }

    if (receiver && receiver->d_func()->threadData.loadRelaxed() != data) {
        qWarning("QCoreApplication::sendPostedEvents: Cannot send "
                 "posted events for objects in another thread");
        return;
    }

    ++data->postEventList.recursion;

    auto locker = qt_unique_lock(data->postEventList.mutex);

    // by default, we assume that the event dispatcher can go to sleep after
    // processing all events. if any new events are posted while we send
    // events, canWait will be set to false.
    data->canWait = (data->postEventList.size() == 0);

    if (data->postEventList.size() == 0 || (receiver && !receiver->d_func()->postedEvents)) {
        --data->postEventList.recursion;
        return;
    }

    data->canWait = true;

    // okay. here is the tricky loop. be careful about optimizing
    // this, it looks the way it does for good reasons.
    qsizetype startOffset = data->postEventList.startOffset;
    qsizetype &i = (!event_type && !receiver) ? data->postEventList.startOffset : startOffset;
    data->postEventList.insertionOffset = data->postEventList.size();

    // Exception-safe cleaning up without the need for a try/catch block
    struct CleanUp {
        QObject *receiver;
        int event_type;
        QThreadData *data;
        bool exceptionCaught;

        inline CleanUp(QObject *receiver, int event_type, QThreadData *data) :
            receiver(receiver), event_type(event_type), data(data), exceptionCaught(true)
        {}
        inline ~CleanUp()
        {
            if (exceptionCaught) {
                // since we were interrupted, we need another pass to make sure we clean everything up
                data->canWait = false;
            }

            --data->postEventList.recursion;
            if (!data->postEventList.recursion && !data->canWait && data->hasEventDispatcher())
                data->eventDispatcher.loadRelaxed()->wakeUp();

            // clear the global list, i.e. remove everything that was
            // delivered.
            if (!event_type && !receiver && data->postEventList.startOffset >= 0) {
                const QPostEventList::iterator it = data->postEventList.begin();
                data->postEventList.erase(it, it + data->postEventList.startOffset);
                data->postEventList.insertionOffset -= data->postEventList.startOffset;
                Q_ASSERT(data->postEventList.insertionOffset >= 0);
                data->postEventList.startOffset = 0;
            }
        }
    };
    CleanUp cleanup(receiver, event_type, data);

    while (i < data->postEventList.size()) {
        // avoid live-lock
        if (i >= data->postEventList.insertionOffset)
            break;

        const QPostEvent &pe = data->postEventList.at(i);
        ++i;

        if (!pe.event)
            continue;
        if ((receiver && receiver != pe.receiver) || (event_type && event_type != pe.event->type())) {
            data->canWait = false;
            continue;
        }

        if (pe.event->type() == QEvent::DeferredDelete) {
            // DeferredDelete events are sent either
            // 1) when the event loop that posted the event has returned; or
            // 2) if explicitly requested (with QEvent::DeferredDelete) for
            //    events posted by the current event loop; or
            // 3) if the event was posted before the outermost event loop.

            int eventLevel = static_cast<QDeferredDeleteEvent *>(pe.event)->loopLevel();
            int loopLevel = data->loopLevel + data->scopeLevel;
            const bool allowDeferredDelete =
                (eventLevel > loopLevel
                 || (!eventLevel && loopLevel > 0)
                 || (event_type == QEvent::DeferredDelete
                     && eventLevel == loopLevel));
            if (!allowDeferredDelete) {
                // cannot send deferred delete
                if (!event_type && !receiver) {
                    // we must copy it first; we want to re-post the event
                    // with the event pointer intact, but we can't delay
                    // nulling the event ptr until after re-posting, as
                    // addEvent may invalidate pe.
                    QPostEvent pe_copy = pe;

                    // null out the event so if sendPostedEvents recurses, it
                    // will ignore this one, as it's been re-posted.
                    const_cast<QPostEvent &>(pe).event = nullptr;

                    // re-post the copied event so it isn't lost
                    data->postEventList.addEvent(pe_copy);
                }
                continue;
            }
        }

        // first, we diddle the event so that we can deliver
        // it, and that no one will try to touch it later.
        pe.event->m_posted = false;
        QEvent *e = pe.event;
        QObject * r = pe.receiver;

        --r->d_func()->postedEvents;
        Q_ASSERT(r->d_func()->postedEvents >= 0);

        // next, update the data structure so that we're ready
        // for the next event.
        const_cast<QPostEvent &>(pe).event = nullptr;

        locker.unlock();
        const auto relocker = qScopeGuard([&locker] { locker.lock(); });

        QScopedPointer<QEvent> event_deleter(e); // will delete the event (with the mutex unlocked)

        // after all that work, it's time to deliver the event.
        QCoreApplication::sendEvent(r, e);

        // careful when adding anything below this point - the
        // sendEvent() call might invalidate any invariants this
        // function depends on.
    }

    cleanup.exceptionCaught = false;
}

/*!
    \since 4.3

    Removes all events of the given \a eventType that were posted
    using postEvent() for \a receiver.

    The events are \e not dispatched, instead they are removed from
    the queue. You should never need to call this function. If you do
    call it, be aware that killing events may cause \a receiver to
    break one or more invariants.

    If \a receiver is \nullptr, the events of \a eventType are removed
    for all objects. If \a eventType is 0, all the events are removed
    for \a receiver. You should never call this function with \a
    eventType of 0.

    \threadsafe
*/

void QCoreApplication::removePostedEvents(QObject *receiver, int eventType)
{
    auto locker = QCoreApplicationPrivate::lockThreadPostEventList(receiver);
    QThreadData *data = locker.threadData;

    // the QObject destructor calls this function directly.  this can
    // happen while the event loop is in the middle of posting events,
    // and when we get here, we may not have any more posted events
    // for this object.
    if (receiver && !receiver->d_func()->postedEvents)
        return;

    //we will collect all the posted events for the QObject
    //and we'll delete after the mutex was unlocked
    QVarLengthArray<QEvent*> events;
    qsizetype n = data->postEventList.size();
    qsizetype j = 0;

    for (qsizetype i = 0; i < n; ++i) {
        const QPostEvent &pe = data->postEventList.at(i);

        if ((!receiver || pe.receiver == receiver)
            && (pe.event && (eventType == 0 || pe.event->type() == eventType))) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->m_posted = false;
            events.append(pe.event);
            const_cast<QPostEvent &>(pe).event = nullptr;
        } else if (!data->postEventList.recursion) {
            if (i != j)
                qSwap(data->postEventList[i], data->postEventList[j]);
            ++j;
        }
    }

#ifdef QT_DEBUG
    if (receiver && eventType == 0) {
        Q_ASSERT(!receiver->d_func()->postedEvents);
    }
#endif

    if (!data->postEventList.recursion) {
        // truncate list
        data->postEventList.erase(data->postEventList.begin() + j, data->postEventList.end());
    }

    locker.unlock();
    qDeleteAll(events);
}

/*!
  Removes \a event from the queue of posted events, and emits a
  warning message if appropriate.

  \warning This function can be \e really slow. Avoid using it, if
  possible.

  \threadsafe
*/

void QCoreApplicationPrivate::removePostedEvent(QEvent * event)
{
    if (!event || !event->m_posted)
        return;

    QThreadData *data = QThreadData::current();

    const auto locker = qt_scoped_lock(data->postEventList.mutex);

    if (data->postEventList.size() == 0) {
#if defined(QT_DEBUG)
        qDebug("QCoreApplication::removePostedEvent: Internal error: %p %d is posted",
                (void*)event, event->type());
        return;
#endif
    }

    for (const QPostEvent &pe : std::as_const(data->postEventList)) {
        if (pe.event == event) {
#ifndef QT_NO_DEBUG
            qWarning("QCoreApplication::removePostedEvent: Event of type %d deleted while posted to %s %s",
                     event->type(),
                     pe.receiver->metaObject()->className(),
                     pe.receiver->objectName().toLocal8Bit().data());
#endif
            --pe.receiver->d_func()->postedEvents;
            pe.event->m_posted = false;
            delete pe.event;
            const_cast<QPostEvent &>(pe).event = nullptr;
            return;
        }
    }
}

/*!\reimp

*/
bool QCoreApplication::event(QEvent *e)
{
    if (e->type() == QEvent::Quit) {
        exit(0);
        return true;
    }
    return QObject::event(e);
}

void QCoreApplicationPrivate::ref()
{
    quitLockRef.ref();
}

void QCoreApplicationPrivate::deref()
{
    quitLockRef.deref();

    if (quitLockEnabled && canQuitAutomatically())
        quitAutomatically();
}

bool QCoreApplicationPrivate::canQuitAutomatically()
{
    if (!in_exec)
        return false;

    if (quitLockEnabled && quitLockRef.loadRelaxed())
        return false;

    return true;
}

void QCoreApplicationPrivate::quitAutomatically()
{
    Q_Q(QCoreApplication);

    // Explicit requests by the user to quit() is plumbed via the platform
    // if possible, and delivers the quit event synchronously. For automatic
    // quits we implicitly support cancelling the quit by showing another
    // window, which currently relies on removing any posted quit events
    // from the event queue. As a result, we can't use the normal quit()
    // code path, and need to post manually.
    QCoreApplication::postEvent(q, new QEvent(QEvent::Quit));
}

/*!
    \threadsafe

    Asks the application to quit.

    The request may be ignored if the application prevents the quit,
    for example if one of its windows can't be closed. The application
    can affect this by handling the QEvent::Quit event on the application
    level, or QEvent::Close events for the individual windows.

    If the quit is not interrupted the application will exit with return
    code 0 (success).

    To exit the application without a chance of being interrupted, call
    exit() directly. Note that method is not thread-safe.

    It's good practice to always connect signals to this slot using a
    \l{Qt::}{QueuedConnection}. If a signal connected (non-queued) to this slot
    is emitted before control enters the main event loop (such as before
    "int main" calls \l{QCoreApplication::}{exec()}), the slot has no effect
    and the application never exits. Using a queued connection ensures that the
    slot will not be invoked until after control enters the main event loop.

    Example:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 1

    \b{Thread-safety note}: this function may be called from any thread to
    thread-safely cause the currently-running main application loop to exit.
    However, thread-safety is not guaranteed if the QCoreApplication object is
    being destroyed at the same time.

    \sa exit(), aboutToQuit()
*/
void QCoreApplication::quit()
{
    if (!self)
        return;

    if (!self->d_func()->in_exec)
        return;

    self->d_func()->quit();
}

void QCoreApplicationPrivate::quit()
{
    Q_Q(QCoreApplication);

    if (QThread::currentThread() == mainThread()) {
        QEvent quitEvent(QEvent::Quit);
        QCoreApplication::sendEvent(q, &quitEvent);
    } else {
        QCoreApplication::postEvent(q, new QEvent(QEvent::Quit));
    }
}

/*!
  \fn void QCoreApplication::aboutToQuit()

  This signal is emitted when the application is about to quit the
  main event loop, e.g. when the event loop level drops to zero.
  This may happen either after a call to quit() from inside the
  application or when the user shuts down the entire desktop session.

  The signal is particularly useful if your application has to do some
  last-second cleanup. Note that no user interaction is possible in
  this state.

  \note At this point the main event loop is still running, but will
  not process further events on return except QEvent::DeferredDelete
  events for objects deleted via deleteLater(). If event processing is
  needed, use a nested event loop or call QCoreApplication::processEvents()
  manually.

  \sa quit()
*/

#endif // QT_NO_QOBJECT

#ifndef QT_NO_TRANSLATION
/*!
    Adds the translation file \a translationFile to the list of
    translation files to be used for translations.

    Multiple translation files can be installed. Translations are
    searched for in the reverse order in which they were installed,
    so the most recently installed translation file is searched first
    and the first translation file installed is searched last.
    The search stops as soon as a translation containing a matching
    string is found.

    Installing or removing a QTranslator, or changing an installed QTranslator
    generates a \l{QEvent::LanguageChange}{LanguageChange} event for the
    QCoreApplication instance. A QApplication instance will propagate the event
    to all toplevel widgets, where a reimplementation of changeEvent can
    re-translate the user interface by passing user-visible strings via the
    tr() function to the respective property setters. User-interface classes
    generated by Qt Designer provide a \c retranslateUi() function that can be
    called.

    The function returns \c true on success and false on failure.

    \note QCoreApplication does \e not take ownership of \a translationFile.

    \sa removeTranslator(), translate(), QTranslator::load(),
        {Writing Source Code for Translation#Prepare for Dynamic Language Changes}{Prepare for Dynamic Language Changes}
*/

bool QCoreApplication::installTranslator(QTranslator *translationFile)
{
    if (!translationFile)
        return false;

    if (!QCoreApplicationPrivate::checkInstance("installTranslator"))
        return false;
    QCoreApplicationPrivate *d = self->d_func();
    {
        QWriteLocker locker(&d->translateMutex);
        d->translators.prepend(translationFile);
    }

#ifndef QT_NO_TRANSLATION_BUILDER
    if (translationFile->isEmpty())
        return false;
#endif

#ifndef QT_NO_QOBJECT
    QEvent ev(QEvent::LanguageChange);
    QCoreApplication::sendEvent(self, &ev);
#endif

    return true;
}

/*!
    Removes the translation file \a translationFile from the list of
    translation files used by this application. (It does not delete the
    translation file from the file system.)

    The function returns \c true on success and false on failure.

    \sa installTranslator(), translate(), QObject::tr()
*/

bool QCoreApplication::removeTranslator(QTranslator *translationFile)
{
    if (!translationFile)
        return false;
    if (!QCoreApplicationPrivate::checkInstance("removeTranslator"))
        return false;
    QCoreApplicationPrivate *d = self->d_func();
    QWriteLocker locker(&d->translateMutex);
    if (d->translators.removeAll(translationFile)) {
#ifndef QT_NO_QOBJECT
        locker.unlock();
        if (!self->closingDown()) {
            QEvent ev(QEvent::LanguageChange);
            QCoreApplication::sendEvent(self, &ev);
        }
#endif
        return true;
    }
    return false;
}

static void replacePercentN(QString *result, int n)
{
    if (n >= 0) {
        qsizetype percentPos = 0;
        qsizetype len = 0;
        while ((percentPos = result->indexOf(u'%', percentPos + len)) != -1) {
            len = 1;
            if (percentPos + len == result->size())
                break;
            QString fmt;
            if (result->at(percentPos + len) == u'L') {
                ++len;
                if (percentPos + len == result->size())
                    break;
                fmt = "%L1"_L1;
            } else {
                fmt = "%1"_L1;
            }
            if (result->at(percentPos + len) == u'n') {
                fmt = fmt.arg(n);
                ++len;
                result->replace(percentPos, len, fmt);
                len = fmt.size();
            }
        }
    }
}

/*!
    \threadsafe

    Returns the translation text for \a sourceText, by querying the
    installed translation files. The translation files are searched
    from the most recently installed file back to the first
    installed file.

    QObject::tr() provides this functionality more conveniently.

    \a context is typically a class name (e.g., "MyDialog") and \a
    sourceText is either English text or a short identifying text.

    \a disambiguation is an identifying string, for when the same \a
    sourceText is used in different roles within the same context. By
    default, it is \nullptr.

    See the \l QTranslator and \l QObject::tr() documentation for
    more information about contexts, disambiguations and comments.

    \a n is used in conjunction with \c %n to support plural forms.
    See QObject::tr() for details.

    If none of the translation files contain a translation for \a
    sourceText in \a context, this function returns a QString
    equivalent of \a sourceText.

    This function is not virtual. You can use alternative translation
    techniques by subclassing \l QTranslator.

    \sa QObject::tr(), installTranslator(), removeTranslator(),
        {Internationalization and Translations}
*/
QString QCoreApplication::translate(const char *context, const char *sourceText,
                                    const char *disambiguation, int n)
{
    QString result;

    if (!sourceText)
        return result;

    if (self) {
        QCoreApplicationPrivate *d = self->d_func();
        QReadLocker locker(&d->translateMutex);
        if (!d->translators.isEmpty()) {
            QList<QTranslator*>::ConstIterator it;
            QTranslator *translationFile;
            for (it = d->translators.constBegin(); it != d->translators.constEnd(); ++it) {
                translationFile = *it;
                result = translationFile->translate(context, sourceText, disambiguation, n);
                if (!result.isNull())
                    break;
            }
        }
    }

    if (result.isNull())
        result = QString::fromUtf8(sourceText);

    replacePercentN(&result, n);
    return result;
}

// Declared in qglobal.h
QString qtTrId(const char *id, int n)
{
    return QCoreApplication::translate(nullptr, id, nullptr, n);
}

bool QCoreApplicationPrivate::isTranslatorInstalled(QTranslator *translator)
{
    if (!QCoreApplication::self)
        return false;
    QCoreApplicationPrivate *d = QCoreApplication::self->d_func();
    QReadLocker locker(&d->translateMutex);
    return d->translators.contains(translator);
}

#else

QString QCoreApplication::translate(const char *context, const char *sourceText,
                                    const char *disambiguation, int n)
{
    Q_UNUSED(context);
    Q_UNUSED(disambiguation);
    QString ret = QString::fromUtf8(sourceText);
    if (n >= 0)
        ret.replace("%n"_L1, QString::number(n));
    return ret;
}

#endif //QT_NO_TRANSLATION

// Makes it possible to point QCoreApplication to a custom location to ensure
// the directory is added to the patch, and qt.conf and deployed plugins are
// found from there. This is for use cases in which QGuiApplication is
// instantiated by a library and not by an application executable, for example,
// Active X servers.

void QCoreApplicationPrivate::setApplicationFilePath(const QString &path)
{
    if (QCoreApplicationPrivate::cachedApplicationFilePath)
        *QCoreApplicationPrivate::cachedApplicationFilePath = path;
    else
        QCoreApplicationPrivate::cachedApplicationFilePath = new QString(path);
}

/*!
    Returns the directory that contains the application executable.

    For example, if you have installed Qt in the \c{C:\Qt}
    directory, and you run the \c{regexp} example, this function will
    return "C:/Qt/examples/tools/regexp".

    On \macos and iOS this will point to the directory actually containing
    the executable, which may be inside an application bundle (if the
    application is bundled).

    \warning On Linux, this function will try to get the path from the
    \c {/proc} file system. If that fails, it assumes that \c
    {argv[0]} contains the absolute file name of the executable. The
    function also assumes that the current directory has not been
    changed by the application.

    \sa applicationFilePath()
*/
QString QCoreApplication::applicationDirPath()
{
    if (!self) {
        qWarning("QCoreApplication::applicationDirPath: Please instantiate the QApplication object first");
        return QString();
    }

    QCoreApplicationPrivate *d = self->d_func();
    if (d->cachedApplicationDirPath.isNull())
        d->cachedApplicationDirPath = QFileInfo(applicationFilePath()).path();
    return d->cachedApplicationDirPath;
}

#if !defined(Q_OS_WIN) && !defined(Q_OS_DARWIN)     // qcoreapplication_win.cpp or qcoreapplication_mac.cpp
static QString qAppFileName()
{
#  if defined(Q_OS_ANDROID)
    // the actual process on Android is the Java VM, so this doesn't help us
    return QString();
#  elif defined(Q_OS_LINUX)
    // this includes the Embedded Android builds
    return QFile::decodeName(qt_readlink("/proc/self/exe"));
#  elif defined(AT_EXECPATH)
    // seen on FreeBSD, but I suppose the other BSDs could adopt this API
    char execfn[PATH_MAX];
    if (elf_aux_info(AT_EXECPATH, execfn, sizeof(execfn)) != 0)
        execfn[0] = '\0';

    qsizetype len = qstrlen(execfn);
    return QFile::decodeName(QByteArray::fromRawData(execfn, len));
#  else
    // other OS or something
    return QString();
#endif
}
#endif

/*!
    Returns the file path of the application executable.

    For example, if you have installed Qt in the \c{/usr/local/qt}
    directory, and you run the \c{regexp} example, this function will
    return "/usr/local/qt/examples/tools/regexp/regexp".

    \warning On Linux, this function will try to get the path from the
    \c {/proc} file system. If that fails, it assumes that \c
    {argv[0]} contains the absolute file name of the executable. The
    function also assumes that the current directory has not been
    changed by the application.

    \sa applicationDirPath()
*/
QString QCoreApplication::applicationFilePath()
{
    if (!self) {
        qWarning("QCoreApplication::applicationFilePath: Please instantiate the QApplication object first");
        return QString();
    }

    QCoreApplicationPrivate *d = self->d_func();

    if (d->argc) {
        static QByteArray procName = QByteArray(d->argv[0]);
        if (procName != d->argv[0]) {
            // clear the cache if the procname changes, so we reprocess it.
            QCoreApplicationPrivate::clearApplicationFilePath();
            procName.assign(d->argv[0]);
        }
    }

    if (QCoreApplicationPrivate::cachedApplicationFilePath)
        return *QCoreApplicationPrivate::cachedApplicationFilePath;

    QString absPath = qAppFileName();
    if (absPath.isEmpty() && !arguments().isEmpty()) {
        QString argv0 = QFile::decodeName(arguments().at(0).toLocal8Bit());

        if (!argv0.isEmpty() && argv0.at(0) == u'/') {
            /*
              If argv0 starts with a slash, it is already an absolute
              file path.
            */
            absPath = argv0;
        } else if (argv0.contains(u'/')) {
            /*
              If argv0 contains one or more slashes, it is a file path
              relative to the current directory.
            */
            absPath = QDir::current().absoluteFilePath(argv0);
        } else {
            /*
              Otherwise, the file path has to be determined using the
              PATH environment variable.
            */
            absPath = QStandardPaths::findExecutable(argv0);
        }
    }

    absPath = QFileInfo(absPath).canonicalFilePath();
    if (!absPath.isEmpty()) {
        QCoreApplicationPrivate::setApplicationFilePath(absPath);
        return *QCoreApplicationPrivate::cachedApplicationFilePath;
    }
    return QString();
}

/*!
    \since 4.4

    Returns the current process ID for the application.
*/
qint64 QCoreApplication::applicationPid()
{
#if defined(Q_OS_WIN)
    return GetCurrentProcessId();
#elif defined(Q_OS_VXWORKS)
    return (pid_t) taskIdCurrent;
#else
    return getpid();
#endif
}

/*!
    \since 4.1

    Returns the list of command-line arguments.

    Usually arguments().at(0) is the program name, arguments().at(1)
    is the first argument, and arguments().last() is the last
    argument. See the note below about Windows.

    Calling this function is slow - you should store the result in a variable
    when parsing the command line.

    \warning On Unix, this list is built from the argc and argv parameters passed
    to the constructor in the main() function. The string-data in argv is
    interpreted using QString::fromLocal8Bit(); hence it is not possible to
    pass, for example, Japanese command line arguments on a system that runs in a
    Latin1 locale. Most modern Unix systems do not have this limitation, as they are
    Unicode-based.

    On Windows, the list is built from the argc and argv parameters only if
    modified argv/argc parameters are passed to the constructor. In that case,
    encoding problems might occur.

    Otherwise, the arguments() are constructed from the return value of
    \l{https://docs.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getcommandlinea}{GetCommandLine()}.
    As a result of this, the string given by arguments().at(0) might not be
    the program name on Windows, depending on how the application was started.

    \sa applicationFilePath(), QCommandLineParser
*/

QStringList QCoreApplication::arguments()
{
    QStringList list;

    if (!self) {
        qWarning("QCoreApplication::arguments: Please instantiate the QApplication object first");
        return list;
    }

    const QCoreApplicationPrivate *d = self->d_func();

    const int argc = d->argc;
    char ** const argv = d->argv;
    list.reserve(argc);

#if defined(Q_OS_WIN)
    const bool argsModifiedByUser = d->origArgv == nullptr;
    if (!argsModifiedByUser) {
        // On Windows, it is possible to pass Unicode arguments on
        // the command line, but we don't implement any of the wide
        // entry-points (wmain/wWinMain), so get the arguments from
        // the Windows API instead of using argv. Note that we only
        // do this when argv were not modified by the user in main().
        QString cmdline = QString::fromWCharArray(GetCommandLine());
        QStringList commandLineArguments = qWinCmdArgs(cmdline);

        // Even if the user didn't modify argv before passing them
        // on to QCoreApplication, derived QApplications might have.
        // If that's the case argc will differ from origArgc.
        if (argc != d->origArgc) {
            // Note: On MingGW the arguments from GetCommandLine are
            // not wildcard expanded (if wildcard expansion is enabled),
            // as opposed to the arguments in argv. This means we can't
            // compare commandLineArguments to argv/origArgc, but
            // must remove elements by value, based on whether they
            // were filtered out from argc.
            for (int i = 0; i < d->origArgc; ++i) {
                if (!contains(argc, argv, d->origArgv[i]))
                    commandLineArguments.removeAll(QString::fromLocal8Bit(d->origArgv[i]));
            }
        }

        return commandLineArguments;
    } // Fall back to rebuilding from argv/argc when a modified argv was passed.
#endif // defined(Q_OS_WIN)

    for (int a = 0; a < argc; ++a)
        list << QString::fromLocal8Bit(argv[a]);

    return list;
}

/*!
    \property QCoreApplication::organizationName
    \brief the name of the organization that wrote this application

    The value is used by the QSettings class when it is constructed
    using the default constructor. This saves having to repeat this
    information each time a QSettings object is created.

    On Mac, QSettings uses \l {QCoreApplication::}{organizationDomain()} as the organization
    if it's not an empty string; otherwise it uses
    organizationName(). On all other platforms, QSettings uses
    organizationName() as the organization.

    \sa organizationDomain, applicationName
*/

/*!
  \fn void QCoreApplication::organizationNameChanged()
  \internal

  While not useful from C++ due to how organizationName is normally set once on
  startup, this is still needed for QML so that bindings are reevaluated after
  that initial change.
*/
void QCoreApplication::setOrganizationName(const QString &orgName)
{
    if (coreappdata()->orgName == orgName)
        return;
    coreappdata()->orgName = orgName;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->organizationNameChanged();
#endif
}

QString QCoreApplication::organizationName()
{
    return coreappdata()->orgName;
}

/*!
    \property QCoreApplication::organizationDomain
    \brief the Internet domain of the organization that wrote this application

    The value is used by the QSettings class when it is constructed
    using the default constructor. This saves having to repeat this
    information each time a QSettings object is created.

    On Mac, QSettings uses organizationDomain() as the organization
    if it's not an empty string; otherwise it uses organizationName().
    On all other platforms, QSettings uses organizationName() as the
    organization.

    \sa organizationName, applicationName, applicationVersion
*/
/*!
  \fn void QCoreApplication::organizationDomainChanged()
  \internal

  Primarily for QML, see organizationNameChanged.
*/
void QCoreApplication::setOrganizationDomain(const QString &orgDomain)
{
    if (coreappdata()->orgDomain == orgDomain)
        return;
    coreappdata()->orgDomain = orgDomain;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->organizationDomainChanged();
#endif
}

QString QCoreApplication::organizationDomain()
{
    return coreappdata()->orgDomain;
}

/*!
    \property QCoreApplication::applicationName
    \brief the name of this application

    The application name is used in various Qt classes and modules,
    most prominently in \l{QSettings} when it is constructed using the default constructor.
    Other uses are in formatted logging output (see \l{qSetMessagePattern()}),
    in output by \l{QCommandLineParser}, in \l{QTemporaryDir} and \l{QTemporaryFile}
    default paths, and in some file locations of \l{QStandardPaths}.
    \l{Qt D-Bus}, \l{Accessibility}, and the XCB platform integration make use
    of the application name, too.

    If not set, the application name defaults to the executable name.

    \sa organizationName, organizationDomain, applicationVersion, applicationFilePath()
*/
/*!
  \fn void QCoreApplication::applicationNameChanged()
  \internal

  Primarily for QML, see organizationNameChanged.
*/
void QCoreApplication::setApplicationName(const QString &application)
{
    coreappdata()->applicationNameSet = !application.isEmpty();
    QString newAppName = application;
    if (newAppName.isEmpty() && QCoreApplication::self)
        newAppName = QCoreApplication::self->d_func()->appName();
    if (coreappdata()->application == newAppName)
        return;
    coreappdata()->application = newAppName;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->applicationNameChanged();
#endif
}

QString QCoreApplication::applicationName()
{
    return coreappdata() ? coreappdata()->application : QString();
}

/*!
    \property QCoreApplication::applicationVersion
    \since 4.4
    \brief the version of this application

    If not set, the application version defaults to a platform-specific value
    determined from the main application executable or package (since Qt 5.9):

    \table
    \header
        \li Platform
        \li Source
    \row
        \li Windows (classic desktop)
        \li PRODUCTVERSION parameter of the VERSIONINFO resource
    \row
        \li macOS, iOS, tvOS, watchOS
        \li CFBundleVersion property of the information property list
    \row
        \li Android
        \li android:versionName property of the AndroidManifest.xml manifest element
    \endtable

    On other platforms, the default is the empty string.

    \sa applicationName, organizationName, organizationDomain
*/
/*!
  \fn void QCoreApplication::applicationVersionChanged()
  \internal

  Primarily for QML, see organizationNameChanged.
*/
void QCoreApplication::setApplicationVersion(const QString &version)
{
    coreappdata()->applicationVersionSet = !version.isEmpty();
    QString newVersion = version;
    if (newVersion.isEmpty() && QCoreApplication::self)
        newVersion = QCoreApplication::self->d_func()->appVersion();
    if (coreappdata()->applicationVersion == newVersion)
        return;
    coreappdata()->applicationVersion = newVersion;
#ifndef QT_NO_QOBJECT
    if (QCoreApplication::self)
        emit QCoreApplication::self->applicationVersionChanged();
#endif
}

QString QCoreApplication::applicationVersion()
{
    return coreappdata() ? coreappdata()->applicationVersion : QString();
}

#if QT_CONFIG(permissions) || defined(Q_QDOC)

/*!
    Checks the status of the given \a permission

    If the result is Qt::PermissionStatus::Undetermined then permission should be
    requested via requestPermission() to determine the user's intent.

    \since 6.5
    \sa requestPermission(), {Application Permissions}
*/
Qt::PermissionStatus QCoreApplication::checkPermission(const QPermission &permission)
{
    return QPermissions::Private::checkPermission(permission);
}

/*!
    \fn template<typename Functor> void QCoreApplication::requestPermission(
        const QPermission &permission, Functor &&functor)

    Requests the given \a permission.

    \include permissions.qdocinc requestPermission-functor

    The \a functor can be a free-standing or static member function:

    \code
    qApp->requestPermission(QCameraPermission{}, &permissionUpdated);
    \endcode

    or a lambda:

    \code
    qApp->requestPermission(QCameraPermission{}, [](const QPermission &permission) {
    });
    \endcode

    \include permissions.qdocinc requestPermission-postamble

    \since 6.5
    \sa checkPermission(), {Application Permissions}
*/

/*!
    \fn template<typename Functor> void QCoreApplication::requestPermission(
        const QPermission &permission, const QObject *context,
        Functor functor)

    Requests the given \a permission, in the context of \a context.

    \include permissions.qdocinc requestPermission-functor

    The \a functor can be a free-standing or static member function:

    \code
    qApp->requestPermission(QCameraPermission{}, context, &permissionUpdated);
    \endcode

    a lambda:

    \code
    qApp->requestPermission(QCameraPermission{}, context, [](const QPermission &permission) {
    });
    \endcode

    or a slot in the \a context object:

    \code
    qApp->requestPermission(QCameraPermission{}, this, &CamerWidget::permissionUpdated);
    \endcode

    The \a functor will be called in the thread of the \a context object. If
    \a context is destroyed before the request completes, the \a functor will
    not be called.

    \include permissions.qdocinc requestPermission-postamble

    \since 6.5
    \overload
    \sa checkPermission(), {Application Permissions}
*/

/*!
    \internal

    Called by the various requestPermission overloads to perform the request.

    Calls the functor encapsulated in the \a slotObjRaw in the given \a context
    (which may be \c nullptr).
*/
void QCoreApplication::requestPermission(const QPermission &requestedPermission,
    QtPrivate::QSlotObjectBase *slotObjRaw, const QObject *context)
{
    QtPrivate::SlotObjSharedPtr slotObj(QtPrivate::SlotObjUniquePtr{slotObjRaw}); // adopts
    if (QThread::currentThread() != QCoreApplicationPrivate::mainThread()) {
        qWarning(lcPermissions, "Permissions can only be requested from the GUI (main) thread");
        return;
    }

    Q_ASSERT(slotObj);

    // Used as the signalID in the metacall event and only used to
    // verify that we are not processing an unrelated event, not to
    // emit the right signal. So using a value that can never clash
    // with any signal index. Clang doesn't like this to be a static
    // member of the PermissionReceiver.
    static constexpr ushort PermissionReceivedID = 0xffff;

    // If we have a context object, then we dispatch the permission response
    // asynchronously through a received object that lives in the same thread
    // as the context object. Otherwise we call the functor synchronously when
    // we get a response (which might still be asynchronous for the caller).
    class PermissionReceiver : public QObject
    {
    public:
        explicit PermissionReceiver(const QtPrivate::SlotObjSharedPtr &slotObject, const QObject *context)
            : slotObject(slotObject), context(context)
        {}

    protected:
        bool event(QEvent *event) override {
            if (event->type() == QEvent::MetaCall) {
                auto metaCallEvent = static_cast<QMetaCallEvent *>(event);
                if (metaCallEvent->id() == PermissionReceivedID) {
                    Q_ASSERT(slotObject);
                    // only execute if context object is still alive
                    if (context)
                        slotObject->call(const_cast<QObject*>(context.data()), metaCallEvent->args());
                    deleteLater();

                    return true;
                }
            }
            return QObject::event(event);
        }
    private:
        QtPrivate::SlotObjSharedPtr slotObject;
        QPointer<const QObject> context;
    };
    PermissionReceiver *receiver = nullptr;
    if (context) {
        receiver = new PermissionReceiver(slotObj, context);
        receiver->moveToThread(context->thread());
    }

    QPermissions::Private::requestPermission(requestedPermission, [=](Qt::PermissionStatus status) {
        Q_ASSERT_X(status != Qt::PermissionStatus::Undetermined, "QPermission",
            "QCoreApplication::requestPermission() should never return Undetermined");
        if (status == Qt::PermissionStatus::Undetermined)
            status = Qt::PermissionStatus::Denied;

        if (QCoreApplication::self) {
            QPermission permission = requestedPermission;
            permission.m_status = status;

            if (receiver) {
                auto metaCallEvent = QMetaCallEvent::create(slotObj.get(), qApp,
                                                            PermissionReceivedID, permission);
                qApp->postEvent(receiver, metaCallEvent);
            } else {
                void *argv[] = { nullptr, &permission };
                slotObj->call(const_cast<QObject*>(context), argv);
            }
        }
    });
}

#endif // QT_CONFIG(permissions)

#if QT_CONFIG(library)

Q_GLOBAL_STATIC(QRecursiveMutex, libraryPathMutex)

/*!
    Returns a list of paths that the application will search when
    dynamically loading libraries.

    The return value of this function may change when a QCoreApplication
    is created. It is not recommended to call it before creating a
    QCoreApplication. The directory of the application executable (\b not
    the working directory) is part of the list if it is known. In order
    to make it known a QCoreApplication has to be constructed as it will
    use \c {argv[0]} to find it.

    Qt provides default library paths, but they can also be set using
    a \l{Using qt.conf}{qt.conf} file. Paths specified in this file
    will override default values. Note that if the qt.conf file is in
    the directory of the application executable, it may not be found
    until a QCoreApplication is created. If it is not found when calling
    this function, the default library paths will be used.

    The list will include the installation directory for plugins if
    it exists (the default installation directory for plugins is \c
    INSTALL/plugins, where \c INSTALL is the directory where Qt was
    installed). The colon separated entries of the \c QT_PLUGIN_PATH
    environment variable are always added. The plugin installation
    directory (and its existence) may change when the directory of
    the application executable becomes known.

    If you want to iterate over the list, you can use the \l foreach
    pseudo-keyword:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 2

    \sa setLibraryPaths(), addLibraryPath(), removeLibraryPath(), QLibrary,
        {How to Create Qt Plugins}
*/
QStringList QCoreApplication::libraryPaths()
{
    QMutexLocker locker(libraryPathMutex());
    return libraryPathsLocked();
}

/*!
    \internal
*/
QStringList QCoreApplication::libraryPathsLocked()
{
    if (coreappdata()->manual_libpaths)
        return *(coreappdata()->manual_libpaths);

    if (!coreappdata()->app_libpaths) {
        QStringList *app_libpaths = new QStringList;
        coreappdata()->app_libpaths.reset(app_libpaths);

        auto setPathsFromEnv = [&](QString libPathEnv) {
            if (!libPathEnv.isEmpty()) {
                QStringList paths = libPathEnv.split(QDir::listSeparator(), Qt::SkipEmptyParts);
                for (QStringList::const_iterator it = paths.constBegin(); it != paths.constEnd(); ++it) {
                    QString canonicalPath = QDir(*it).canonicalPath();
                    if (!canonicalPath.isEmpty()
                        && !app_libpaths->contains(canonicalPath)) {
                        app_libpaths->append(canonicalPath);
                    }
                }
            }
        };
        setPathsFromEnv(qEnvironmentVariable("QT_PLUGIN_PATH"));
#ifdef Q_OS_DARWIN
        // Check the main bundle's PlugIns directory as this is a standard location for Apple OSes.
        // Note that the QLibraryInfo::PluginsPath below will coincidentally be the same as this value
        // but with a different casing, so it can't be relied upon when the underlying filesystem
        // is case sensitive (and this is always the case on newer OSes like iOS).
        if (CFBundleRef bundleRef = CFBundleGetMainBundle()) {
            if (QCFType<CFURLRef> urlRef = CFBundleCopyBuiltInPlugInsURL(bundleRef)) {
                if (QCFType<CFURLRef> absoluteUrlRef = CFURLCopyAbsoluteURL(urlRef)) {
                    if (QCFString path = CFURLCopyFileSystemPath(absoluteUrlRef, kCFURLPOSIXPathStyle)) {
                        if (QFile::exists(path)) {
                            path = QDir(path).canonicalPath();
                            if (!app_libpaths->contains(path))
                                app_libpaths->append(path);
                        }
                    }
                }
            }
        }
#endif // Q_OS_DARWIN

        QString installPathPlugins =  QLibraryInfo::path(QLibraryInfo::PluginsPath);
        if (QFile::exists(installPathPlugins)) {
            // Make sure we convert from backslashes to slashes.
            installPathPlugins = QDir(installPathPlugins).canonicalPath();
            if (!app_libpaths->contains(installPathPlugins))
                app_libpaths->append(installPathPlugins);
        }

        // If QCoreApplication is not yet instantiated,
        // make sure we add the application path when we construct the QCoreApplication
        if (self) self->d_func()->appendApplicationPathToLibraryPaths();
    }
    return *(coreappdata()->app_libpaths);
}



/*!

    Sets the list of directories to search when loading plugins with
    QLibrary to \a paths. All existing paths will be deleted and the
    path list will consist of the paths given in \a paths and the path
    to the application.

    The library paths are reset to the default when an instance of
    QCoreApplication is destructed.

    \sa libraryPaths(), addLibraryPath(), removeLibraryPath(), QLibrary
 */
void QCoreApplication::setLibraryPaths(const QStringList &paths)
{
    QMutexLocker locker(libraryPathMutex());

    // setLibraryPaths() is considered a "remove everything and then add some new ones" operation.
    // When the application is constructed it should still amend the paths. So we keep the originals
    // around, and even create them if they don't exist, yet.
    if (!coreappdata()->app_libpaths)
        libraryPathsLocked();

    if (coreappdata()->manual_libpaths)
        *(coreappdata()->manual_libpaths) = paths;
    else
        coreappdata()->manual_libpaths.reset(new QStringList(paths));

    locker.unlock();
    QFactoryLoader::refreshAll();
}

/*!
  Prepends \a path to the beginning of the library path list, ensuring that
  it is searched for libraries first. If \a path is empty or already in the
  path list, the path list is not changed.

  The default path list consists of one or two entries. The first is the
  installation directory for plugins, which is \c INSTALL/plugins, where \c
  INSTALL is the directory where Qt was installed. The second is the
  application's own directory (\b not the current directory), but only after
  the QCoreApplication object is instantiated.

  The library paths are reset to the default when an instance of
  QCoreApplication is destructed.

  \sa removeLibraryPath(), libraryPaths(), setLibraryPaths()
 */
void QCoreApplication::addLibraryPath(const QString &path)
{
    if (path.isEmpty())
        return;

    QString canonicalPath = QDir(path).canonicalPath();
    if (canonicalPath.isEmpty())
        return;

    QMutexLocker locker(libraryPathMutex());

    QStringList *libpaths = coreappdata()->manual_libpaths.get();
    if (libpaths) {
        if (libpaths->contains(canonicalPath))
            return;
    } else {
        // make sure that library paths are initialized
        libraryPathsLocked();
        QStringList *app_libpaths = coreappdata()->app_libpaths.get();
        if (app_libpaths->contains(canonicalPath))
            return;

        coreappdata()->manual_libpaths.reset(libpaths = new QStringList(*app_libpaths));
    }

    libpaths->prepend(canonicalPath);
    locker.unlock();
    QFactoryLoader::refreshAll();
}

/*!
    Removes \a path from the library path list. If \a path is empty or not
    in the path list, the list is not changed.

    The library paths are reset to the default when an instance of
    QCoreApplication is destructed.

    \sa addLibraryPath(), libraryPaths(), setLibraryPaths()
*/
void QCoreApplication::removeLibraryPath(const QString &path)
{
    if (path.isEmpty())
        return;

    QString canonicalPath = QDir(path).canonicalPath();
    if (canonicalPath.isEmpty())
        return;

    QMutexLocker locker(libraryPathMutex());

    QStringList *libpaths = coreappdata()->manual_libpaths.get();
    if (libpaths) {
        if (libpaths->removeAll(canonicalPath) == 0)
            return;
    } else {
        // make sure that library paths is initialized
        libraryPathsLocked();
        QStringList *app_libpaths = coreappdata()->app_libpaths.get();
        if (!app_libpaths->contains(canonicalPath))
            return;

        coreappdata()->manual_libpaths.reset(libpaths = new QStringList(*app_libpaths));
        libpaths->removeAll(canonicalPath);
    }

    locker.unlock();
    QFactoryLoader::refreshAll();
}

#endif // QT_CONFIG(library)

#ifndef QT_NO_QOBJECT

/*!
    Installs an event filter \a filterObj for all native events
    received by the application in the main thread.

    The event filter \a filterObj receives events via its \l {QAbstractNativeEventFilter::}{nativeEventFilter()}
    function, which is called for all native events received in the main thread.

    The QAbstractNativeEventFilter::nativeEventFilter() function should
    return true if the event should be filtered, i.e. stopped. It should
    return false to allow normal Qt processing to continue: the native
    event can then be translated into a QEvent and handled by the standard
    Qt \l{QEvent} {event} filtering, e.g. QObject::installEventFilter().

    If multiple event filters are installed, the filter that was
    installed last is activated first.

    \note The filter function set here receives native messages,
    i.e. MSG or XCB event structs.

    \note Native event filters will be disabled in the application when the
    Qt::AA_PluginApplication attribute is set.

    For maximum portability, you should always try to use QEvent
    and QObject::installEventFilter() whenever possible.

    \sa QObject::installEventFilter()

    \since 5.0
*/
void QCoreApplication::installNativeEventFilter(QAbstractNativeEventFilter *filterObj)
{
    if (QCoreApplication::testAttribute(Qt::AA_PluginApplication)) {
        qWarning("Native event filters are not applied when the Qt::AA_PluginApplication attribute is set");
        return;
    }

    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance(QCoreApplicationPrivate::theMainThread.loadAcquire());
    if (!filterObj || !eventDispatcher)
        return;
    eventDispatcher->installNativeEventFilter(filterObj);
}

/*!
    Removes an event \a filterObject from this object. The
    request is ignored if such an event filter has not been installed.

    All event filters for this object are automatically removed when
    this object is destroyed.

    It is always safe to remove an event filter, even during event
    filter activation (i.e. from the nativeEventFilter() function).

    \sa installNativeEventFilter()
    \since 5.0
*/
void QCoreApplication::removeNativeEventFilter(QAbstractNativeEventFilter *filterObject)
{
    QAbstractEventDispatcher *eventDispatcher = QAbstractEventDispatcher::instance();
    if (!filterObject || !eventDispatcher)
        return;
    eventDispatcher->removeNativeEventFilter(filterObject);
}

/*!
    Returns a pointer to the event dispatcher object for the main thread. If no
    event dispatcher exists for the thread, this function returns \nullptr.
*/
QAbstractEventDispatcher *QCoreApplication::eventDispatcher()
{
    if (QCoreApplicationPrivate::theMainThread.loadAcquire())
        return QCoreApplicationPrivate::theMainThread.loadRelaxed()->eventDispatcher();
    return nullptr;
}

/*!
    Sets the event dispatcher for the main thread to \a eventDispatcher. This
    is only possible as long as there is no event dispatcher installed yet. That
    is, before QCoreApplication has been instantiated. This method takes
    ownership of the object.
*/
void QCoreApplication::setEventDispatcher(QAbstractEventDispatcher *eventDispatcher)
{
    QThread *mainThread = QCoreApplicationPrivate::theMainThread.loadAcquire();
    if (!mainThread)
        mainThread = QThread::currentThread(); // will also setup theMainThread
    mainThread->setEventDispatcher(eventDispatcher);
}

#endif // QT_NO_QOBJECT

/*!
    \macro Q_COREAPP_STARTUP_FUNCTION(QtStartUpFunction ptr)
    \since 5.1
    \relates QCoreApplication
    \reentrant

    Adds a global function that will be called from the QCoreApplication
    constructor. This macro is normally used to initialize libraries
    for program-wide functionality, without requiring the application to
    call into the library for initialization.

    The function specified by \a ptr should take no arguments and should
    return nothing. For example:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 3

    Note that the startup function will run at the end of the QCoreApplication constructor,
    before any GUI initialization. If GUI code is required in the function,
    use a timer (or a queued invocation) to perform the initialization later on,
    from the event loop.

    If QCoreApplication is deleted and another QCoreApplication is created,
    the startup function will be invoked again.

    \note This macro is not suitable for use in library code that is then
    statically linked into an application since the function may not be called
    at all due to being eliminated by the linker.
*/

/*!
    \fn void qAddPostRoutine(QtCleanUpFunction ptr)
    \threadsafe
    \relates QCoreApplication

    Adds a global routine that will be called from the QCoreApplication
    destructor. This function is normally used to add cleanup routines
    for program-wide functionality.

    The cleanup routines are called in the reverse order of their addition.

    The function specified by \a ptr should take no arguments and should
    return nothing. For example:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 4

    Note that for an application- or module-wide cleanup, qAddPostRoutine()
    is often not suitable. For example, if the program is split into dynamically
    loaded modules, the relevant module may be unloaded long before the
    QCoreApplication destructor is called. In such cases, if using qAddPostRoutine()
    is still desirable, qRemovePostRoutine() can be used to prevent a routine
    from being called by the QCoreApplication destructor. For example, if that
    routine was called before the module was unloaded.

    For modules and libraries, using a reference-counted
    initialization manager or Qt's parent-child deletion mechanism may
    be better. Here is an example of a private class that uses the
    parent-child mechanism to call a cleanup function at the right
    time:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 5

    By selecting the right parent object, this can often be made to
    clean up the module's data at the right moment.

    \note This function has been thread-safe since Qt 5.10.

    \sa qRemovePostRoutine()
*/

/*!
    \fn void qRemovePostRoutine(QtCleanUpFunction ptr)
    \threadsafe
    \relates QCoreApplication
    \since 5.3

    Removes the cleanup routine specified by \a ptr from the list of
    routines called by the QCoreApplication destructor. The routine
    must have been previously added to the list by a call to
    qAddPostRoutine(), otherwise this function has no effect.

    \note This function has been thread-safe since Qt 5.10.

    \sa qAddPostRoutine()
*/

/*!
    \macro Q_DECLARE_TR_FUNCTIONS(context)
    \relates QCoreApplication

    The Q_DECLARE_TR_FUNCTIONS() macro declares and implements the
    translation function \c tr() with this signature:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 6

    This macro is useful if you want to use QObject::tr()  in classes
    that don't inherit from QObject.

    Q_DECLARE_TR_FUNCTIONS() must appear at the very top of the
    class definition (before the first \c{public:} or \c{protected:}).
    For example:

    \snippet code/src_corelib_kernel_qcoreapplication.cpp 7

    The \a context parameter is normally the class name, but it can
    be any text.

    \sa Q_OBJECT, QObject::tr()
*/

void *QCoreApplication::resolveInterface(const char *name, int revision) const
{
    Q_UNUSED(name); Q_UNUSED(revision);
    return nullptr;
}

QT_END_NAMESPACE

#ifndef QT_NO_QOBJECT
#include "moc_qcoreapplication.cpp"
#endif
