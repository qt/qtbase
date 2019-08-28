/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatforminputcontextfactory_p.h>
#include <qpa/qplatformoffscreensurface.h>
#include <QtCore/qcoreapplication.h>

#include <QtPlatformHeaders/qcocoanativecontext.h>

#include <QtGui/private/qcoregraphics_p.h>

#include <QtFontDatabaseSupport/private/qfontengine_coretext_p.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qtwidgetsglobal.h>
#if QT_CONFIG(filedialog)
#include "qcocoafiledialoghelper.h"
#endif
#endif

#include <IOKit/graphics/IOGraphicsLib.h>

static void initResources()
{
    Q_INIT_RESOURCE(qcocoaresources);
}

QT_BEGIN_NAMESPACE

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
        if (param == QLatin1String("fontengine=freetype"))
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
#ifndef QT_NO_ACCESSIBILITY
    , mAccessibility(new QCocoaAccessibility)
#endif
#ifndef QT_NO_CLIPBOARD
    , mCocoaClipboard(new QCocoaClipboard)
#endif
    , mCocoaDrag(new QCocoaDrag)
    , mNativeInterface(new QCocoaNativeInterface)
    , mServices(new QCocoaServices)
    , mKeyboardMapper(new QCocoaKeyMapper)
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
        // bundle) are "background" applications that does not take keybaord
        // focus or have a dock icon or task switcher entry. Qt Gui apps generally
        // wants to be foreground applications so change the process type. (But
        // see the function implementation for exceptions.)
        qt_mac_transformProccessToForegroundApplication();

        // Move the application window to front to make it take focus, also when launching
        // from the terminal. On 10.12+ this call has been moved to applicationDidFinishLauching
        // to work around issues with loss of focus at startup.
        if (QOperatingSystemVersion::current() < QOperatingSystemVersion::MacOSSierra) {
            // Ignoring other apps is necessary (we must ignore the terminal), but makes
            // Qt apps play slightly less nice with other apps when lanching from Finder
            // (See the activateIgnoringOtherApps docs.)
            [cocoaApplication activateIgnoringOtherApps : YES];
        }
    }

    // ### For AA_MacPluginApplication we don't want to load the menu nib.
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

    QMacInternalPasteboardMime::initializeMimeTypes();
    QCocoaMimeTypes::initializeMimeTypes();
    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);

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
    QMacInternalPasteboardMime::destroyMimeTypes();
#endif

    QCocoaScreen::cleanupScreens();

    clearToolbars();
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
        // building with SDK 10.14 or higher which enbles view layer-backing.
        return QMacVersion::buildSDK() < QOperatingSystemVersion(QOperatingSystemVersion::MacOSMojave);
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
#endif

QPlatformBackingStore *QCocoaIntegration::createPlatformBackingStore(QWindow *window) const
{
    QCocoaWindow *platformWindow = static_cast<QCocoaWindow*>(window->handle());
    if (!platformWindow) {
        qWarning() << window << "must be created before being used with a backingstore";
        return nullptr;
    }

    if (platformWindow->view().layer)
        return new QCALayerBackingStore(window);
    else
        return new QNSWindowBackingStore(window);
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

#ifndef QT_NO_ACCESSIBILITY
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
    return QStringList(QLatin1String(QCocoaTheme::name));
}

QPlatformTheme *QCocoaIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1String(QCocoaTheme::name))
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
    default: break;
    }

    return QPlatformIntegration::styleHint(hint);
}

Qt::KeyboardModifiers QCocoaIntegration::queryKeyboardModifiers() const
{
    return QCocoaKeyMapper::queryKeyboardModifiers();
}

QList<int> QCocoaIntegration::possibleKeys(const QKeyEvent *event) const
{
    return mKeyboardMapper->possibleKeys(event);
}

void QCocoaIntegration::setToolbar(QWindow *window, NSToolbar *toolbar)
{
    if (NSToolbar *prevToolbar = mToolbars.value(window))
        [prevToolbar release];

    [toolbar retain];
    mToolbars.insert(window, toolbar);
}

NSToolbar *QCocoaIntegration::toolbar(QWindow *window) const
{
    return mToolbars.value(window);
}

void QCocoaIntegration::clearToolbars()
{
    QHash<QWindow *, NSToolbar *>::const_iterator it = mToolbars.constBegin();
    while (it != mToolbars.constEnd()) {
        [it.value() release];
        ++it;
    }
    mToolbars.clear();
}

void QCocoaIntegration::pushPopupWindow(QCocoaWindow *window)
{
    m_popupWindowStack.append(window);
}

QCocoaWindow *QCocoaIntegration::popPopupWindow()
{
    if (m_popupWindowStack.isEmpty())
        return nullptr;
    return m_popupWindowStack.takeLast();
}

QCocoaWindow *QCocoaIntegration::activePopupWindow() const
{
    if (m_popupWindowStack.isEmpty())
        return nullptr;
    return m_popupWindowStack.front();
}

QList<QCocoaWindow *> *QCocoaIntegration::popupWindowStack()
{
    return &m_popupWindowStack;
}

void QCocoaIntegration::setApplicationIcon(const QIcon &icon) const
{
    // Fall back to a size that looks good on the highest resolution screen available
    auto fallbackSize = NSApp.dockTile.size.width * qGuiApp->devicePixelRatio();
    NSApp.applicationIconImage = [NSImage imageFromQIcon:icon withSize:fallbackSize];
}

void QCocoaIntegration::beep() const
{
    NSBeep();
}

void QCocoaIntegration::closePopups(QWindow *forWindow)
{
    for (auto it = m_popupWindowStack.begin(); it != m_popupWindowStack.end();) {
        auto *popup = *it;
        if (!forWindow || popup->window()->transientParent() == forWindow) {
            it = m_popupWindowStack.erase(it);
            QWindowSystemInterface::handleCloseEvent<QWindowSystemInterface::SynchronousDelivery>(popup->window());
        } else {
            ++it;
        }
    }
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

#include "moc_qcocoaintegration.cpp"

QT_END_NAMESPACE
