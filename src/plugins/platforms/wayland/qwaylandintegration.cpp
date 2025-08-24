// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwaylandintegration_p.h"

#include "qwaylanddisplay_p.h"
#include "qwaylandshmwindow_p.h"
#include "qwaylandinputdevice_p.h"
#include "qwaylandinputcontext_p.h"
#include "qwaylandinputmethodcontext_p.h"
#include "qwaylandshmbackingstore_p.h"
#include "qwaylandnativeinterface_p.h"
#if QT_CONFIG(clipboard)
#include "qwaylandclipboard_p.h"
#endif
#include "qwaylanddnd_p.h"
#include "qwaylandwindowmanagerintegration_p.h"
#include "qwaylandplatformservices_p.h"
#include "qwaylandscreen_p.h"
#include "qwaylandcursor_p.h"
#include "qwaylandeventdispatcher_p.h"

#if defined(Q_OS_MACOS)
#  include <QtGui/private/qcoretextfontdatabase_p.h>
#  include <QtGui/private/qfontengine_coretext_p.h>
#else
#  include <QtGui/private/qgenericunixfontdatabase_p.h>
#endif
#include <QtGui/private/qgenericunixtheme_p.h>

#include <QtGui/private/qguiapplication_p.h>

#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformcursor.h>
#include <QtGui/QSurfaceFormat>
#if QT_CONFIG(opengl)
#include <QtGui/QOpenGLContext>
#endif // QT_CONFIG(opengl)
#include <QSocketNotifier>

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatforminputcontext.h>

#include "qwaylandhardwareintegration_p.h"
#include "qwaylandclientbufferintegration_p.h"
#include "qwaylandclientbufferintegrationfactory_p.h"

#include "qwaylandserverbufferintegration_p.h"
#include "qwaylandserverbufferintegrationfactory_p.h"
#include "qwaylandshellsurface_p.h"

#include "qwaylandshellintegration_p.h"
#include "qwaylandshellintegrationfactory_p.h"

#include "qwaylandinputdeviceintegration_p.h"
#include "qwaylandinputdeviceintegrationfactory_p.h"
#include "qwaylandwindow_p.h"

#include <QtWaylandClient/private/qwayland-xdg-system-bell-v1.h>

#if QT_CONFIG(accessibility_atspi_bridge)
#include <QtGui/private/qspiaccessiblebridge_p.h>
#endif

#if QT_CONFIG(xkbcommon)
#include <QtGui/private/qxkbcommon_p.h>
#endif

#if QT_CONFIG(vulkan)
#include "qwaylandvulkaninstance_p.h"
#include "qwaylandvulkanwindow_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtWaylandClient {

QWaylandIntegration *QWaylandIntegration::sInstance = nullptr;

QWaylandIntegration::QWaylandIntegration(const QString &platformName)
#if defined(Q_OS_MACOS)
    : mFontDb(new QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>)
#else
    : mPlatformName(platformName), mFontDb(new QGenericUnixFontDatabase())
#endif
{
    mDisplay.reset(new QWaylandDisplay(this));
    mPlatformServices.reset(new QWaylandPlatformServices(mDisplay.data()));

    QWaylandWindow::fixedToplevelPositions =
            !qEnvironmentVariableIsSet("QT_WAYLAND_DISABLE_FIXED_POSITIONS");

    sInstance = this;
    if (platformName != "wayland"_L1)
        initializeClientBufferIntegration();
}

QWaylandIntegration::~QWaylandIntegration()
{
    sInstance = nullptr;
}

bool QWaylandIntegration::init()
{
    return mDisplay->initialize();
}

QPlatformNativeInterface * QWaylandIntegration::nativeInterface() const
{
    return mNativeInterface.data();
}

bool QWaylandIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL:
        return mDisplay->clientBufferIntegration();
    case ThreadedOpenGL:
        return mDisplay->clientBufferIntegration() && mDisplay->clientBufferIntegration()->supportsThreadedOpenGL();
    case BufferQueueingOpenGL:
        return true;
    case MultipleWindows:
    case NonFullScreenWindows:
        return true;
    case RasterGLSurface:
        return true;
    case WindowActivation:
        return true;
    case ScreenWindowGrabbing: // whether QScreen::grabWindow() is supported
        return false;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QWaylandIntegration::createPlatformWindow(QWindow *window) const
{
    if ((window->surfaceType() == QWindow::OpenGLSurface || window->surfaceType() == QWindow::RasterGLSurface)
        && mDisplay->clientBufferIntegration())
        return mDisplay->clientBufferIntegration()->createEglWindow(window);

#if QT_CONFIG(vulkan)
    if (window->surfaceType() == QSurface::VulkanSurface)
        return new QWaylandVulkanWindow(window, mDisplay.data());
#endif // QT_CONFIG(vulkan)

    return new QWaylandShmWindow(window, mDisplay.data());
}

#if QT_CONFIG(opengl)
QPlatformOpenGLContext *QWaylandIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    if (mDisplay->clientBufferIntegration())
        return mDisplay->clientBufferIntegration()->createPlatformOpenGLContext(context->format(), context->shareHandle());
    return nullptr;
}

QOpenGLContext *QWaylandIntegration::createOpenGLContext(EGLContext context, EGLDisplay contextDisplay, QOpenGLContext *shareContext) const
{
    return mClientBufferIntegration->createOpenGLContext(context, contextDisplay, shareContext);
}
#endif  // opengl

QPlatformBackingStore *QWaylandIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWaylandShmBackingStore(window, mDisplay.data());
}

QAbstractEventDispatcher *QWaylandIntegration::createEventDispatcher() const
{
    return QWaylandEventDispatcher::createEventDispatcher();
}

QPlatformNativeInterface *QWaylandIntegration::createPlatformNativeInterface()
{
    return new QWaylandNativeInterface(this);
}

// Support platform specific initialization
void QWaylandIntegration::initializePlatform()
{
    mDisplay->initEventThread();

    mNativeInterface.reset(createPlatformNativeInterface());
    initializeInputDeviceIntegration();
#if QT_CONFIG(clipboard)
    mClipboard.reset(new QWaylandClipboard(mDisplay.data()));
#endif
#if QT_CONFIG(draganddrop)
    mDrag.reset(new QWaylandDrag(mDisplay.data()));
#endif

    reconfigureInputContext();
}

void QWaylandIntegration::initialize()
{
    initializePlatform();

    // Call this after initializing event thread for QWaylandDisplay::flushRequests()
    QAbstractEventDispatcher *dispatcher = QGuiApplicationPrivate::eventDispatcher;
    QObject::connect(dispatcher, SIGNAL(aboutToBlock()), mDisplay.data(), SLOT(flushRequests()));
    QObject::connect(dispatcher, SIGNAL(awake()), mDisplay.data(), SLOT(flushRequests()));

    // Qt does not support running with no screens
    mDisplay->ensureScreen();
}

QPlatformFontDatabase *QWaylandIntegration::fontDatabase() const
{
    return mFontDb.data();
}

#if QT_CONFIG(clipboard)
QPlatformClipboard *QWaylandIntegration::clipboard() const
{
    return mClipboard.data();
}
#endif

#if QT_CONFIG(draganddrop)
QPlatformDrag *QWaylandIntegration::drag() const
{
    return mDrag.data();
}
#endif  // draganddrop

QPlatformInputContext *QWaylandIntegration::inputContext() const
{
    return mInputContext.data();
}

QVariant QWaylandIntegration::styleHint(StyleHint hint) const
{
    if (hint == ShowIsFullScreen && mDisplay->windowManagerIntegration())
        return mDisplay->windowManagerIntegration()->showIsFullScreen();

    return QPlatformIntegration::styleHint(hint);
}

#if QT_CONFIG(accessibility)
QPlatformAccessibility *QWaylandIntegration::accessibility() const
{
    if (!mAccessibility) {
#if QT_CONFIG(accessibility_atspi_bridge)
        Q_ASSERT_X(QCoreApplication::eventDispatcher(), "QWaylandIntegration",
            "Initializing accessibility without event-dispatcher!");
        mAccessibility.reset(new QSpiAccessibleBridge());
#else
        mAccessibility.reset(new QPlatformAccessibility());
#endif
    }
    return mAccessibility.data();
}
#endif

QPlatformServices *QWaylandIntegration::services() const
{
    return mPlatformServices.data();
}

QWaylandDisplay *QWaylandIntegration::display() const
{
    return mDisplay.data();
}

Qt::KeyboardModifiers QWaylandIntegration::queryKeyboardModifiers() const
{
    if (auto *seat = mDisplay->currentInputDevice(); seat && seat->keyboardFocus()) {
        return seat->modifiers();
    }
    return Qt::NoModifier;
}

QList<int> QWaylandIntegration::possibleKeys(const QKeyEvent *event) const
{
    if (auto *seat = mDisplay->currentInputDevice())
        return seat->possibleKeys(event);
    return {};
}

QStringList QWaylandIntegration::themeNames() const
{
    return QGenericUnixTheme::themeNames();
}

QPlatformTheme *QWaylandIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

QWaylandScreen *QWaylandIntegration::createPlatformScreen(QWaylandDisplay *waylandDisplay, int version, uint32_t id) const
{
   return new QWaylandScreen(waylandDisplay, version, id);
}

QWaylandCursor *QWaylandIntegration::createPlatformCursor(QWaylandDisplay *display) const
{
   return new QWaylandCursor(display);
}

#if QT_CONFIG(vulkan)
QPlatformVulkanInstance *QWaylandIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QWaylandVulkanInstance(instance);
}
#endif // QT_CONFIG(vulkan)

// May be called from non-GUI threads
QWaylandClientBufferIntegration *QWaylandIntegration::clientBufferIntegration() const
{
    // Do an inexpensive check first to avoid locking whenever possible
    if (Q_UNLIKELY(!mClientBufferIntegrationInitialized))
        const_cast<QWaylandIntegration *>(this)->initializeClientBufferIntegration();

    Q_ASSERT(mClientBufferIntegrationInitialized);
    return mClientBufferIntegration && mClientBufferIntegration->isValid() ? mClientBufferIntegration.data() : nullptr;
}

QWaylandServerBufferIntegration *QWaylandIntegration::serverBufferIntegration() const
{
    if (!mServerBufferIntegrationInitialized)
        const_cast<QWaylandIntegration *>(this)->initializeServerBufferIntegration();

    return mServerBufferIntegration.data();
}

QWaylandShellIntegration *QWaylandIntegration::shellIntegration() const
{
    if (!mShellIntegrationInitialized)
        const_cast<QWaylandIntegration *>(this)->initializeShellIntegration();

    return mShellIntegration.data();
}

// May be called from non-GUI threads
void QWaylandIntegration::initializeClientBufferIntegration()
{
    QMutexLocker lock(&mClientBufferInitLock);
    if (mClientBufferIntegrationInitialized)
        return;

    QString targetKey = QString::fromLocal8Bit(qgetenv("QT_WAYLAND_CLIENT_BUFFER_INTEGRATION"));
    if (mPlatformName == "wayland-egl"_L1)
        targetKey = "wayland-egl"_L1;
    else if (mPlatformName == "wayland-brcm"_L1)
        targetKey = "brcm"_L1;

    if (targetKey.isEmpty()) {
        if (mDisplay->hardwareIntegration()
                && mDisplay->hardwareIntegration()->clientBufferIntegration() != QLatin1String("wayland-eglstream-controller")
                && mDisplay->hardwareIntegration()->clientBufferIntegration() != QLatin1String("linux-dmabuf-unstable-v1")) {
            targetKey = mDisplay->hardwareIntegration()->clientBufferIntegration();
        } else {
            targetKey = QLatin1String("wayland-egl");
        }
    }

    if (targetKey.isEmpty()) {
        qWarning("Failed to determine what client buffer integration to use");
    } else {
        QStringList keys = QWaylandClientBufferIntegrationFactory::keys();
        qCDebug(lcQpaWayland) << "Available client buffer integrations:" << keys;

        if (keys.contains(targetKey))
            mClientBufferIntegration.reset(QWaylandClientBufferIntegrationFactory::create(targetKey, QStringList()));

        if (mClientBufferIntegration) {
            qCDebug(lcQpaWayland) << "Initializing client buffer integration" << targetKey;
            mClientBufferIntegration->initialize(mDisplay.data());
        } else {
            qCWarning(lcQpaWayland) << "Failed to load client buffer integration:" << targetKey;
            qCWarning(lcQpaWayland) << "Available client buffer integrations:" << keys;
        }
    }

    // This must be set last to make sure other threads don't use the
    // integration before initialization is complete.
    mClientBufferIntegrationInitialized = true;
}

void QWaylandIntegration::initializeServerBufferIntegration()
{
    mServerBufferIntegrationInitialized = true;

    QString targetKey = QString::fromLocal8Bit(qgetenv("QT_WAYLAND_SERVER_BUFFER_INTEGRATION"));

    if (targetKey.isEmpty() && mDisplay->hardwareIntegration())
        targetKey = mDisplay->hardwareIntegration()->serverBufferIntegration();

    if (targetKey.isEmpty()) {
        qWarning("Failed to determine what server buffer integration to use");
        return;
    }

    QStringList keys = QWaylandServerBufferIntegrationFactory::keys();
    qCDebug(lcQpaWayland) << "Available server buffer integrations:" << keys;

    if (keys.contains(targetKey))
        mServerBufferIntegration.reset(QWaylandServerBufferIntegrationFactory::create(targetKey, QStringList()));

    if (mServerBufferIntegration) {
        qCDebug(lcQpaWayland) << "Initializing server buffer integration" << targetKey;
        mServerBufferIntegration->initialize(mDisplay.data());
    } else {
        qCWarning(lcQpaWayland) << "Failed to load server buffer integration: " <<  targetKey;
        qCWarning(lcQpaWayland) << "Available server buffer integrations:" << keys;
    }
}

void QWaylandIntegration::initializeShellIntegration()
{
    mShellIntegrationInitialized = true;

    QByteArray integrationNames = qgetenv("QT_WAYLAND_SHELL_INTEGRATION");
    QString targetKeys = QString::fromLocal8Bit(integrationNames);

    QStringList preferredShells;
    if (!targetKeys.isEmpty()) {
        preferredShells = targetKeys.split(QLatin1Char(';'));
    } else {
        preferredShells << QLatin1String("xdg-shell");
        preferredShells << QLatin1String("wl-shell") << QLatin1String("ivi-shell");
        preferredShells << QLatin1String("qt-shell");
    }

    for (const QString &preferredShell : std::as_const(preferredShells)) {
        mShellIntegration.reset(createShellIntegration(preferredShell));
        if (mShellIntegration) {
            qCDebug(lcQpaWayland, "Using the '%s' shell integration", qPrintable(preferredShell));
            break;
        }
    }

    if (!mShellIntegration) {
        qCWarning(lcQpaWayland) << "Loading shell integration failed.";
        qCWarning(lcQpaWayland) << "Attempted to load the following shells" << preferredShells;
    }

    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
}

QWaylandInputDevice *QWaylandIntegration::createInputDevice(QWaylandDisplay *display, int version, uint32_t id) const
{
    if (mInputDeviceIntegration) {
        return mInputDeviceIntegration->createInputDevice(display, version, id);
    }
    return new QWaylandInputDevice(display, version, id);
}

void QWaylandIntegration::initializeInputDeviceIntegration()
{
    QByteArray integrationName = qgetenv("QT_WAYLAND_INPUTDEVICE_INTEGRATION");
    QString targetKey = QString::fromLocal8Bit(integrationName);

    if (targetKey.isEmpty()) {
        return;
    }

    QStringList keys = QWaylandInputDeviceIntegrationFactory::keys();
    if (keys.contains(targetKey)) {
        mInputDeviceIntegration.reset(QWaylandInputDeviceIntegrationFactory::create(targetKey, QStringList()));
        qDebug("Using the '%s' input device integration", qPrintable(targetKey));
    } else {
        qWarning("Wayland inputdevice integration '%s' not found, using default", qPrintable(targetKey));
    }
}

void QWaylandIntegration::reconfigureInputContext()
{
    if (!mDisplay) {
        // This function can be called from QWaylandDisplay::registry_global() when we
        // are in process of constructing QWaylandDisplay. Configuring input context
        // in that case is done by calling reconfigureInputContext() from QWaylandIntegration
        // constructor, after QWaylandDisplay has been constructed.
        return;
    }

    auto requested = QPlatformInputContextFactory::requested();
    if (requested.contains(QLatin1String("qtvirtualkeyboard")))
        qCWarning(lcQpaWayland) << "qtvirtualkeyboard currently is not supported at client-side,"
                                   " use QT_IM_MODULES=qtvirtualkeyboard at compositor-side.";

    if (mDisplay->isWaylandInputContextRequested()
        && !requested.contains(QLatin1String(WAYLAND_IM_KEY)))
        requested.append(QLatin1String(WAYLAND_IM_KEY));

    const QString defaultInputContext(QStringLiteral("compose"));
    if (!requested.contains(defaultInputContext))
        requested.append(defaultInputContext);

    for (const QString &imKey : requested) {
        if (imKey == QLatin1String(WAYLAND_IM_KEY)) {
            Q_ASSERT(mDisplay->isWaylandInputContextRequested());
            if (mDisplay->textInputMethodManager() != nullptr)
                mInputContext.reset(new QWaylandInputMethodContext(mDisplay.data()));
            else if (mDisplay->textInputManagerv1() != nullptr
                     || mDisplay->textInputManagerv2() != nullptr
                     || mDisplay->textInputManagerv3() != nullptr)
                mInputContext.reset(new QWaylandInputContext(mDisplay.data()));
        } else {
            mInputContext.reset(QPlatformInputContextFactory::create(imKey));
        }

        if (mInputContext && mInputContext->isValid())
            break;
    }

#if QT_CONFIG(xkbcommon)
    QXkbCommon::setXkbContext(mInputContext.data(), mDisplay->xkbContext());
    if (QWaylandInputContext* waylandInput = qobject_cast<QWaylandInputContext*>(mInputContext.get())) {
        waylandInput->setXkbContext(mDisplay->xkbContext());
    }
#endif

    qCDebug(lcQpaWayland) << "using input method:" << (inputContext() ? inputContext()->metaObject()->className() : "<none>");
}

QWaylandShellIntegration *QWaylandIntegration::createShellIntegration(const QString &integrationName)
{
    if (QWaylandShellIntegrationFactory::keys().contains(integrationName)) {
        return QWaylandShellIntegrationFactory::create(integrationName, mDisplay.data());
    } else {
        qCWarning(lcQpaWayland) << "No shell integration named" << integrationName << "found";
        return nullptr;
    }
}

void QWaylandIntegration::reset()
{
    mServerBufferIntegration.reset();
    mServerBufferIntegrationInitialized = false;

    mInputDeviceIntegration.reset();

    mClientBufferIntegration.reset();
    mClientBufferIntegrationInitialized = false;
}

void QWaylandIntegration::setApplicationBadge(qint64 number)
{
    mPlatformServices->setApplicationBadge(number);
}

void QWaylandIntegration::beep() const
{
    if (auto bell = mDisplay->systemBell()) {
        bell->ring(nullptr);
    }
}

}

QT_END_NAMESPACE
