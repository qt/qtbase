// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <AppKit/AppKit.h>

#include "qcocoaintegration.h"

#include "qcocoawindow.h"
#include "qcocoabackingstore.h"
#include "qcocoanativeinterface.h"
#include "qcocoamenuloader.h"
#include "qcocoaeventdispatcher.h"
#include "qcocoahelpers.h"
#include "qcocoaapplication.h"
#include "qcocoaapplicationdelegate.h"
#include "qcocoatheme.h"
#include "qcocoainputcontext.h"
#include "qcocoamimetypes.h"
#include "qcocoaaccessibility.h"
#include "qcocoascreen.h"
#if QT_CONFIG(sessionmanager)
#  include "qcocoasessionmanager.h"
#endif
#include "qcocoawindowmanager.h"

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatformoffscreensurface.h>
#include <QtCore/qcoreapplication.h>
#include <QtGui/qpointingdevice.h>

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qcoregraphics_p.h>
#include <QtGui/private/qmacmimeregistry_p.h>
#ifndef QT_NO_OPENGL
#  include <QtGui/private/qopenglcontext_p.h>
#endif
#include <QtGui/private/qrhibackingstore_p.h>
#include <QtGui/private/qfontengine_coretext_p.h>

#include <IOKit/graphics/IOGraphicsLib.h>

#include <inttypes.h>

static void initResources()
{
    Q_INIT_RESOURCE(qcocoaresources);
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQpa, "qt.qpa", QtWarningMsg);

static void logVersionInformation()
{
    if (!lcQpa().isInfoEnabled())
        return;

    auto osVersion = QMacVersion::currentRuntime();
    auto qtBuildSDK = QMacVersion::buildSDK(QMacVersion::QtLibraries);
    auto qtDeploymentTarget = QMacVersion::deploymentTarget(QMacVersion::QtLibraries);
    auto appBuildSDK = QMacVersion::buildSDK(QMacVersion::ApplicationBinary);
    auto appDeploymentTarget = QMacVersion::deploymentTarget(QMacVersion::ApplicationBinary);

    qCInfo(lcQpa, "Loading macOS (Cocoa) platform plugin for Qt " QT_VERSION_STR ", running on macOS %d.%d.%d\n\n" \
        "  Component     SDK version   Deployment target  \n" \
        " ------------- ------------- -------------------\n" \
        "  Qt " QT_VERSION_STR "       %d.%d.%d          %d.%d.%d\n" \
        "  Application     %d.%d.%d          %d.%d.%d\n",
            osVersion.majorVersion(), osVersion.minorVersion(), osVersion.microVersion(),
            qtBuildSDK.majorVersion(), qtBuildSDK.minorVersion(), qtBuildSDK.microVersion(),
            qtDeploymentTarget.majorVersion(), qtDeploymentTarget.minorVersion(), qtDeploymentTarget.microVersion(),
            appBuildSDK.majorVersion(), appBuildSDK.minorVersion(), appBuildSDK.microVersion(),
            appDeploymentTarget.majorVersion(), appDeploymentTarget.minorVersion(), appDeploymentTarget.microVersion());
}


class QCoreTextFontEngine;
class QFontEngineFT;

static QCocoaIntegration::Options parseOptions(const QStringList &paramList)
{
    QCocoaIntegration::Options options;
    for (const QString &param : paramList) {
#ifndef QT_NO_FREETYPE
        if (param == "fontengine=freetype"_L1)
            options |= QCocoaIntegration::UseFreeTypeFontEngine;
        else
#endif
            qWarning() << "Unknown option" << param;
    }
    return options;
}

QCocoaIntegration *QCocoaIntegration::mInstance = nullptr;

QCocoaIntegration::QCocoaIntegration(const QStringList &paramList)
    : mOptions(parseOptions(paramList))
    , mFontDb(nullptr)
#if QT_CONFIG(accessibility)
    , mAccessibility(new QCocoaAccessibility)
#endif
#ifndef QT_NO_CLIPBOARD
    , mCocoaClipboard(new QCocoaClipboard)
#endif
    , mCocoaDrag(new QCocoaDrag)
    , mNativeInterface(new QCocoaNativeInterface)
    , mServices(new QCocoaServices)
    , mKeyboardMapper(new QAppleKeyMapper)
{
    logVersionInformation();

    if (mInstance)
        qWarning("Creating multiple Cocoa platform integrations is not supported");
    mInstance = this;

#ifndef QT_NO_FREETYPE
    if (mOptions.testFlag(UseFreeTypeFontEngine))
        mFontDb.reset(new QCoreTextFontDatabaseEngineFactory<QFontEngineFT>);
    else
#endif
        mFontDb.reset(new QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>);

    QString icStr = QPlatformInputContextFactory::requested();
    icStr.isNull() ? mInputContext.reset(new QCocoaInputContext)
                   : mInputContext.reset(QPlatformInputContextFactory::create(icStr));

    initResources();
    QMacAutoReleasePool pool;

    NSApplication *cocoaApplication = [QNSApplication sharedApplication];
    qt_redirectNSApplicationSendEvent();

    if (qEnvironmentVariableIsEmpty("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM")) {
        // Applications launched from plain executables (without an app
        // bundle) are "background" applications that does not take keyboard
        // focus or have a dock icon or task switcher entry. Qt Gui apps generally
        // wants to be foreground applications so change the process type. (But
        // see the function implementation for exceptions.)
        qt_mac_transformProccessToForegroundApplication();
    }

    // Qt 4 also does not set the application delegate, so that behavior
    // is matched here.
    if (!QCoreApplication::testAttribute(Qt::AA_PluginApplication)) {

        // Set app delegate, link to the current delegate (if any)
        QCocoaApplicationDelegate *newDelegate = [QCocoaApplicationDelegate sharedDelegate];
        [newDelegate setReflectionDelegate:[cocoaApplication delegate]];
        [cocoaApplication setDelegate:newDelegate];

        // Load the application menu. This menu contains Preferences, Hide, Quit.
        QCocoaMenuLoader *qtMenuLoader = [QCocoaMenuLoader sharedMenuLoader];
        [cocoaApplication setMenu:[qtMenuLoader menu]];
    }

    QCocoaScreen::initializeScreens();

    QMacMimeRegistry::initializeMimeTypes();
    QCocoaMimeTypes::initializeMimeTypes();
    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
    QWindowSystemInterface::registerInputDevice(new QInputDevice(QString("keyboard"), 0,
                                                                 QInputDevice::DeviceType::Keyboard, QString(), this));

    connect(qGuiApp, &QGuiApplication::focusWindowChanged,
        this, &QCocoaIntegration::focusWindowChanged);
}

QCocoaIntegration::~QCocoaIntegration()
{
    mInstance = nullptr;

    qt_resetNSApplicationSendEvent();

    QMacAutoReleasePool pool;
    if (!QCoreApplication::testAttribute(Qt::AA_PluginApplication)) {
        // remove the apple event handlers installed by QCocoaApplicationDelegate
        QCocoaApplicationDelegate *delegate = [QCocoaApplicationDelegate sharedDelegate];
        [delegate removeAppleEventHandlers];
        // reset the application delegate
        [[NSApplication sharedApplication] setDelegate:nil];
    }

#ifndef QT_NO_CLIPBOARD
    // Delete the clipboard integration and destroy mime type converters.
    // Deleting the clipboard integration flushes promised pastes using
    // the mime converters - the ordering here is important.
    delete mCocoaClipboard;
    QMacMimeRegistry::destroyMimeTypes();
#endif

    QCocoaScreen::cleanupScreens();
}

QCocoaIntegration *QCocoaIntegration::instance()
{
    return mInstance;
}

QCocoaIntegration::Options QCocoaIntegration::options() const
{
    return mOptions;
}

#if QT_CONFIG(sessionmanager)
QPlatformSessionManager *QCocoaIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QCocoaSessionManager(id, key);
}
#endif

bool QCocoaIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
#ifndef QT_NO_OPENGL
    case ThreadedOpenGL:
        // AppKit expects rendering to happen on the main thread, and we can
        // easily end up in situations where rendering on secondary threads
        // will result in visual artifacts, bugs, or even deadlocks, when
        // layer-backed.
        return false;
    case OpenGL:
    case BufferQueueingOpenGL:
#endif
    case ThreadedPixmaps:
    case WindowMasks:
    case MultipleWindows:
    case ForeignWindows:
    case RasterGLSurface:
    case ApplicationState:
    case ApplicationIcon:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QCocoaIntegration::createPlatformWindow(QWindow *window) const
{
    return new QCocoaWindow(window);
}

QPlatformWindow *QCocoaIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    return new QCocoaWindow(window, nativeHandle);
}

class QCocoaOffscreenSurface : public QPlatformOffscreenSurface
{
public:
    QCocoaOffscreenSurface(QOffscreenSurface *offscreenSurface) : QPlatformOffscreenSurface(offscreenSurface) {}

    QSurfaceFormat format() const override
    {
        Q_ASSERT(offscreenSurface());
        return offscreenSurface()->requestedFormat();
    }
    bool isValid() const override { return true; }
};

QPlatformOffscreenSurface *QCocoaIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QCocoaOffscreenSurface(surface);
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QCocoaIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QCocoaGLContext(context);
}

QOpenGLContext *QCocoaIntegration::createOpenGLContext(NSOpenGLContext *nativeContext, QOpenGLContext *shareContext) const
{
    if (!nativeContext)
        return nullptr;

    auto *context = new QOpenGLContext;
    context->setShareContext(shareContext);
    auto *contextPrivate = QOpenGLContextPrivate::get(context);
    contextPrivate->adopt(new QCocoaGLContext(nativeContext));
    return context;
}

#endif

QPlatformBackingStore *QCocoaIntegration::createPlatformBackingStore(QWindow *window) const
{
    QCocoaWindow *platformWindow = static_cast<QCocoaWindow*>(window->handle());
    if (!platformWindow) {
        qWarning() << window << "must be created before being used with a backingstore";
        return nullptr;
    }

    switch (window->surfaceType()) {
    case QSurface::RasterSurface:
        return new QCALayerBackingStore(window);
    case QSurface::MetalSurface:
    case QSurface::OpenGLSurface:
    case QSurface::VulkanSurface:
        return new QRhiBackingStore(window);
    default:
        return nullptr;
    }
}

QAbstractEventDispatcher *QCocoaIntegration::createEventDispatcher() const
{
    return new QCocoaEventDispatcher;
}

#if QT_CONFIG(vulkan)
QPlatformVulkanInstance *QCocoaIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    mCocoaVulkanInstance = new QCocoaVulkanInstance(instance);
    return mCocoaVulkanInstance;
}

QCocoaVulkanInstance *QCocoaIntegration::getCocoaVulkanInstance() const
{
    return mCocoaVulkanInstance;
}
#endif

QCoreTextFontDatabase *QCocoaIntegration::fontDatabase() const
{
    return mFontDb.data();
}

QCocoaNativeInterface *QCocoaIntegration::nativeInterface() const
{
    return mNativeInterface.data();
}

QPlatformInputContext *QCocoaIntegration::inputContext() const
{
    return mInputContext.data();
}

#if QT_CONFIG(accessibility)
QCocoaAccessibility *QCocoaIntegration::accessibility() const
{
    return mAccessibility.data();
}
#endif

#ifndef QT_NO_CLIPBOARD
QCocoaClipboard *QCocoaIntegration::clipboard() const
{
    return mCocoaClipboard;
}
#endif

QCocoaDrag *QCocoaIntegration::drag() const
{
    return mCocoaDrag.data();
}

QStringList QCocoaIntegration::themeNames() const
{
    return QStringList(QLatin1StringView(QCocoaTheme::name));
}

QPlatformTheme *QCocoaIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1StringView(QCocoaTheme::name))
        return new QCocoaTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QCocoaServices *QCocoaIntegration::services() const
{
    return mServices.data();
}

QVariant QCocoaIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case FontSmoothingGamma:
        return QCoreTextFontEngine::fontSmoothingGamma();
    case ShowShortcutsInContextMenus:
        return QVariant(false);
    case ReplayMousePressOutsidePopup:
        return QVariant(false);
    default: break;
    }

    return QPlatformIntegration::styleHint(hint);
}

Qt::KeyboardModifiers QCocoaIntegration::queryKeyboardModifiers() const
{
    return QAppleKeyMapper::queryKeyboardModifiers();
}

QList<int> QCocoaIntegration::possibleKeys(const QKeyEvent *event) const
{
    return mKeyboardMapper->possibleKeys(event);
}

void QCocoaIntegration::setApplicationIcon(const QIcon &icon) const
{
    // Fall back to a size that looks good on the highest resolution screen available
    auto fallbackSize = NSApp.dockTile.size.width * qGuiApp->devicePixelRatio();
    NSApp.applicationIconImage = [NSImage imageFromQIcon:icon withSize:fallbackSize];
}

void QCocoaIntegration::setApplicationBadge(qint64 number)
{
    NSApp.dockTile.badgeLabel = number ? [NSString stringWithFormat:@"%" PRId64, number] : nil;
}

void QCocoaIntegration::beep() const
{
    NSBeep();
}

void QCocoaIntegration::quit() const
{
    qCDebug(lcQpaApplication) << "Terminating application";
    [NSApp terminate:nil];
}

void QCocoaIntegration::focusWindowChanged(QWindow *focusWindow)
{
    // Don't revert icon just because we lost focus
    if (!focusWindow)
        return;

    static bool hasDefaultApplicationIcon = [](){
        NSImage *genericApplicationIcon = [[NSWorkspace sharedWorkspace]
            iconForFileType:NSFileTypeForHFSTypeCode(kGenericApplicationIcon)];
        NSImage *applicationIcon = [NSImage imageNamed:NSImageNameApplicationIcon];

        NSRect rect = NSMakeRect(0, 0, 32, 32);
        return [applicationIcon CGImageForProposedRect:&rect context:nil hints:nil]
            == [genericApplicationIcon CGImageForProposedRect:&rect context:nil hints:nil];
    }();

    // Don't let the window icon override an explicit application icon set in the Info.plist
    if (!hasDefaultApplicationIcon)
        return;

    // Or an explicit application icon set on QGuiApplication
    if (!qGuiApp->windowIcon().isNull())
        return;

    setApplicationIcon(focusWindow->icon());
}

QT_END_NAMESPACE

#include "moc_qcocoaintegration.cpp"
