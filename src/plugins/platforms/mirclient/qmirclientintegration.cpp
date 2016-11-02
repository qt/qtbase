/****************************************************************************
**
** Copyright (C) 2014-2016 Canonical, Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


// Local
#include "qmirclientintegration.h"
#include "qmirclientbackingstore.h"
#include "qmirclientclipboard.h"
#include "qmirclientdebugextension.h"
#include "qmirclientdesktopwindow.h"
#include "qmirclientglcontext.h"
#include "qmirclientinput.h"
#include "qmirclientlogging.h"
#include "qmirclientnativeinterface.h"
#include "qmirclientscreen.h"
#include "qmirclienttheme.h"
#include "qmirclientwindow.h"

// Qt
#include <QFileInfo>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatforminputcontext.h>
#include <QtEglSupport/private/qeglconvenience_p.h>
#include <QtEglSupport/private/qeglpbuffer_p.h>
#include <QtFontDatabaseSupport/private/qgenericunixfontdatabase_p.h>
#include <QtEventDispatcherSupport/private/qgenericunixeventdispatcher_p.h>
#ifndef QT_NO_ACCESSIBILITY
#include <qpa/qplatformaccessibility.h>
#ifndef QT_NO_ACCESSIBILITY_ATSPI_BRIDGE
#include <QtLinuxAccessibilitySupport/private/bridge_p.h>
#endif
#endif

#include <QOpenGLContext>
#include <QOffscreenSurface>

// platform-api
#include <ubuntu/application/lifecycle_delegate.h>
#include <ubuntu/application/id.h>
#include <ubuntu/application/options.h>

static void resumedCallback(const UApplicationOptions */*options*/, void* context)
{
    auto integration = static_cast<QMirClientClientIntegration*>(context);
    integration->appStateController()->setResumed();
}

static void aboutToStopCallback(UApplicationArchive */*archive*/, void* context)
{
    auto integration = static_cast<QMirClientClientIntegration*>(context);
    auto inputContext = integration->inputContext();
    if (inputContext) {
        inputContext->hideInputPanel();
    } else {
        qCWarning(mirclient) << "aboutToStopCallback(): no input context";
    }
    integration->appStateController()->setSuspended();
}

QMirClientClientIntegration::QMirClientClientIntegration(int argc, char **argv)
    : QPlatformIntegration()
    , mNativeInterface(new QMirClientNativeInterface(this))
    , mFontDb(new QGenericUnixFontDatabase)
    , mServices(new QMirClientPlatformServices)
    , mAppStateController(new QMirClientAppStateController)
    , mScaleFactor(1.0)
{
    {
        QStringList args = QCoreApplication::arguments();
        setupOptions(args);
        QByteArray sessionName = generateSessionName(args);
        setupDescription(sessionName);
    }

    // Create new application instance
    mInstance = u_application_instance_new_from_description_with_options(mDesc, mOptions);

    if (Q_UNLIKELY(!mInstance))
        qFatal("QMirClientClientIntegration: connection to Mir server failed. Check that a Mir server is\n"
               "running, and the correct socket is being used and is accessible. The shell may have\n"
               "rejected the incoming connection, so check its log file");

    mMirConnection = u_application_instance_get_mir_connection(mInstance);

    // Choose the default surface format suited to the Mir platform
    QSurfaceFormat defaultFormat;
    defaultFormat.setRedBufferSize(8);
    defaultFormat.setGreenBufferSize(8);
    defaultFormat.setBlueBufferSize(8);
    QSurfaceFormat::setDefaultFormat(defaultFormat);

    // Initialize EGL.
    mEglNativeDisplay = mir_connection_get_egl_native_display(mMirConnection);
    ASSERT((mEglDisplay = eglGetDisplay(mEglNativeDisplay)) != EGL_NO_DISPLAY);
    ASSERT(eglInitialize(mEglDisplay, nullptr, nullptr) == EGL_TRUE);

    // Has debug mode been requsted, either with "-testability" switch or QT_LOAD_TESTABILITY env var
    bool testability = qEnvironmentVariableIsSet("QT_LOAD_TESTABILITY");
    for (int i=1; !testability && i<argc; i++) {
        if (strcmp(argv[i], "-testability") == 0) {
            testability = true;
        }
    }
    if (testability) {
        mDebugExtension.reset(new QMirClientDebugExtension);
    }
}

void QMirClientClientIntegration::initialize()
{
    // Init the ScreenObserver
    mScreenObserver.reset(new QMirClientScreenObserver(mMirConnection));
    connect(mScreenObserver.data(), &QMirClientScreenObserver::screenAdded,
            [this](QMirClientScreen *screen) { this->screenAdded(screen); });
    connect(mScreenObserver.data(), &QMirClientScreenObserver::screenRemoved,
                     this, &QMirClientClientIntegration::destroyScreen);

    Q_FOREACH (auto screen, mScreenObserver->screens()) {
        screenAdded(screen);
    }

    // Initialize input.
    mInput = new QMirClientInput(this);
    mInputContext = QPlatformInputContextFactory::create();

    // compute the scale factor
    const int defaultGridUnit = 8;
    int gridUnit = defaultGridUnit;
    QByteArray gridUnitString = qgetenv("GRID_UNIT_PX");
    if (!gridUnitString.isEmpty()) {
        bool ok;
        gridUnit = gridUnitString.toInt(&ok);
        if (!ok) {
            gridUnit = defaultGridUnit;
        }
    }
    mScaleFactor = static_cast<qreal>(gridUnit) / defaultGridUnit;
}

QMirClientClientIntegration::~QMirClientClientIntegration()
{
    eglTerminate(mEglDisplay);
    delete mInput;
    delete mInputContext;
    delete mServices;
}

QPlatformServices *QMirClientClientIntegration::services() const
{
    return mServices;
}

void QMirClientClientIntegration::setupOptions(QStringList &args)
{
    int argc = args.size() + 1;
    char **argv = new char*[argc];
    for (int i = 0; i < argc - 1; i++)
        argv[i] = qstrdup(args.at(i).toLocal8Bit());
    argv[argc - 1] = nullptr;

    mOptions = u_application_options_new_from_cmd_line(argc - 1, argv);

    for (int i = 0; i < argc; i++)
        delete [] argv[i];
    delete [] argv;
}

void QMirClientClientIntegration::setupDescription(QByteArray &sessionName)
{
    mDesc = u_application_description_new();

    UApplicationId* id = u_application_id_new_from_stringn(sessionName.data(), sessionName.count());
    u_application_description_set_application_id(mDesc, id);

    UApplicationLifecycleDelegate* delegate = u_application_lifecycle_delegate_new();
    u_application_lifecycle_delegate_set_application_resumed_cb(delegate, &resumedCallback);
    u_application_lifecycle_delegate_set_application_about_to_stop_cb(delegate, &aboutToStopCallback);
    u_application_lifecycle_delegate_set_context(delegate, this);
    u_application_description_set_application_lifecycle_delegate(mDesc, delegate);
}

QByteArray QMirClientClientIntegration::generateSessionName(QStringList &args)
{
    // Try to come up with some meaningful session name to uniquely identify this session,
    // helping with shell debugging

    if (args.count() == 0) {
        return QByteArray("QtUbuntu");
    } if (args[0].contains("qmlscene")) {
        return generateSessionNameFromQmlFile(args);
    } else {
        // use the executable name
        QFileInfo fileInfo(args[0]);
        return fileInfo.fileName().toLocal8Bit();
    }
}

QByteArray QMirClientClientIntegration::generateSessionNameFromQmlFile(QStringList &args)
{
    Q_FOREACH (QString arg, args) {
        if (arg.endsWith(".qml")) {
            QFileInfo fileInfo(arg);
            return fileInfo.fileName().toLocal8Bit();
        }
    }

    // give up
    return "qmlscene";
}

QPlatformWindow* QMirClientClientIntegration::createPlatformWindow(QWindow* window) const
{
    if (window->type() == Qt::Desktop) {
        // Desktop windows should not be backed up by a mir surface as they don't draw anything (nor should).
        return new QMirClientDesktopWindow(window);
    } else {
        return new QMirClientWindow(window, mInput, mNativeInterface, mAppStateController.data(),
                                    mEglDisplay, mMirConnection, mDebugExtension.data());
    }
}

bool QMirClientClientIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedOpenGL:
        if (qEnvironmentVariableIsEmpty("QTUBUNTU_NO_THREADED_OPENGL")) {
            return true;
        } else {
            qCDebug(mirclient, "disabled threaded OpenGL");
            return false;
        }

    case ThreadedPixmaps:
    case OpenGL:
    case ApplicationState:
    case MultipleWindows:
    case NonFullScreenWindows:
#if QT_VERSION > QT_VERSION_CHECK(5, 5, 0)
    case SwitchableWidgetComposition:
#endif
    case RasterGLSurface: // needed for QQuickWidget
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QAbstractEventDispatcher *QMirClientClientIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformBackingStore* QMirClientClientIntegration::createPlatformBackingStore(QWindow* window) const
{
    return new QMirClientBackingStore(window);
}

QPlatformOpenGLContext* QMirClientClientIntegration::createPlatformOpenGLContext(
        QOpenGLContext* context) const
{
    QSurfaceFormat format(context->format());

    auto platformContext = new QMirClientOpenGLContext(format, context->shareHandle(), mEglDisplay);
    if (!platformContext->isValid()) {
        // Older Intel Atom-based devices only support OpenGL 1.4 compatibility profile but by default
        // QML asks for at least OpenGL 2.0. The XCB GLX backend ignores this request and returns a
        // 1.4 context, but the XCB EGL backend tries to honor it, and fails. The 1.4 context appears to
        // have sufficient capabilities on MESA (i915) to render correctly however. So reduce the default
        // requested OpenGL version to 1.0 to ensure EGL will give us a working context (lp:1549455).
        static const bool isMesa = QString(eglQueryString(mEglDisplay, EGL_VENDOR)).contains(QStringLiteral("Mesa"));
        if (isMesa) {
            qCDebug(mirclientGraphics, "Attempting to choose OpenGL 1.4 context which may suit Mesa");
            format.setMajorVersion(1);
            format.setMinorVersion(4);
            delete platformContext;
            platformContext = new QMirClientOpenGLContext(format, context->shareHandle(), mEglDisplay);
        }
    }
    return platformContext;
}

QStringList QMirClientClientIntegration::themeNames() const
{
    return QStringList(QMirClientTheme::name);
}

QPlatformTheme* QMirClientClientIntegration::createPlatformTheme(const QString& name) const
{
    Q_UNUSED(name);
    return new QMirClientTheme;
}

QVariant QMirClientClientIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
        case QPlatformIntegration::StartDragDistance: {
            // default is 10 pixels (see QPlatformTheme::defaultThemeHint)
            return 10.0 * mScaleFactor;
        }
        case QPlatformIntegration::PasswordMaskDelay: {
            // return time in milliseconds - 1 second
            return QVariant(1000);
        }
        default:
            break;
    }
    return QPlatformIntegration::styleHint(hint);
}

QPlatformClipboard* QMirClientClientIntegration::clipboard() const
{
    static QPlatformClipboard *clipboard = nullptr;
    if (!clipboard) {
        clipboard = new QMirClientClipboard;
    }
    return clipboard;
}

QPlatformNativeInterface* QMirClientClientIntegration::nativeInterface() const
{
    return mNativeInterface;
}

QPlatformOffscreenSurface *QMirClientClientIntegration::createPlatformOffscreenSurface(
        QOffscreenSurface *surface) const
{
    return new QEGLPbuffer(mEglDisplay, surface->requestedFormat(), surface);
}

void QMirClientClientIntegration::destroyScreen(QMirClientScreen *screen)
{
    // FIXME: on deleting a screen while a Window is on it, Qt will automatically
    // move the window to the primaryScreen(). This will trigger a screenChanged
    // signal, causing things like QQuickScreenAttached to re-fetch screen properties
    // like DPI and physical size. However this is crashing, as Qt is calling virtual
    // functions on QPlatformScreen, for reasons unclear. As workaround, move window
    // to primaryScreen() before deleting the screen. Might be QTBUG-38650

    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    if (screen != primaryScreen->handle()) {
        uint32_t movedWindowCount = 0;
        Q_FOREACH (QWindow *w, QGuiApplication::topLevelWindows()) {
            if (w->screen()->handle() == screen) {
                QWindowSystemInterface::handleWindowScreenChanged(w, primaryScreen);
                ++movedWindowCount;
            }
        }
        if (movedWindowCount > 0) {
            QWindowSystemInterface::flushWindowSystemEvents();
        }
    }

    qCDebug(mirclient) << "Removing Screen with id" << screen->mirOutputId() << "and geometry" << screen->geometry();
#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
    delete screen;
#else
    QPlatformIntegration::destroyScreen(screen);
#endif
}

#ifndef QT_NO_ACCESSIBILITY
QPlatformAccessibility *QMirClientClientIntegration::accessibility() const
{
#if !defined(QT_NO_ACCESSIBILITY_ATSPI_BRIDGE)
    if (!mAccessibility) {
        Q_ASSERT_X(QCoreApplication::eventDispatcher(), "QMirClientIntegration",
            "Initializing accessibility without event-dispatcher!");
        mAccessibility.reset(new QSpiAccessibleBridge());
    }
#endif
    return mAccessibility.data();
}
#endif
