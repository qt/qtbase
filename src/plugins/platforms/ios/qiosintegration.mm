// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qiosintegration.h"
#include "qioseventdispatcher.h"
#include "qiosglobal.h"
#include "qioswindow.h"
#include "qiosscreen.h"
#include "qiosplatformaccessibility.h"
#ifndef Q_OS_TVOS
#include "qiosclipboard.h"
#endif
#include "qiosinputcontext.h"
#include "qiostheme.h"
#include "qiosservices.h"
#include "qiosoptionalplugininterface.h"

#include <QtGui/qpointingdevice.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qrhibackingstore_p.h>

#include <qoffscreensurface.h>
#include <qpa/qplatformoffscreensurface.h>

#include <QtGui/private/qcoretextfontdatabase_p.h>
#include <QtGui/private/qmacmimeregistry_p.h>
#include <QtGui/qutimimeconverter.h>
#include <QDir>
#include <QOperatingSystemVersion>

#if QT_CONFIG(opengl)
#include "qioscontext.h"
#endif

#import <AudioToolbox/AudioServices.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QCoreTextFontEngine;

QIOSIntegration *QIOSIntegration::instance()
{
    return static_cast<QIOSIntegration *>(QGuiApplicationPrivate::platformIntegration());
}

QIOSIntegration::QIOSIntegration()
    : m_fontDatabase(new QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>)
#if !defined(Q_OS_TVOS) && !defined(QT_NO_CLIPBOARD)
    , m_clipboard(new QIOSClipboard)
#endif
    , m_inputContext(0)
    , m_platformServices(new QIOSServices)
    , m_accessibility(0)
    , m_optionalPlugins(new QFactoryLoader(QIosOptionalPluginInterface_iid, "/platforms/darwin"_L1))
{
    if (Q_UNLIKELY(!qt_apple_isApplicationExtension() && !qt_apple_sharedApplication())) {
        qFatal("Error: You are creating QApplication before calling UIApplicationMain.\n" \
               "If you are writing a native iOS application, and only want to use Qt for\n" \
               "parts of the application, a good place to create QApplication is from within\n" \
               "'applicationDidFinishLaunching' inside your UIApplication delegate.\n");
    }

    // Set current directory to app bundle folder
    QDir::setCurrent(QString::fromUtf8([[[NSBundle mainBundle] bundlePath] UTF8String]));
}

void QIOSIntegration::initialize()
{
    UIScreen *mainScreen = [UIScreen mainScreen];
    NSMutableArray<UIScreen *> *screens = [[[UIScreen screens] mutableCopy] autorelease];
    if (![screens containsObject:mainScreen]) {
        // Fallback for iOS 7.1 (QTBUG-42345)
        [screens insertObject:mainScreen atIndex:0];
    }

    for (UIScreen *screen in screens)
        QWindowSystemInterface::handleScreenAdded(new QIOSScreen(screen));

    // Depends on a primary screen being present
    m_inputContext = new QIOSInputContext;

    m_touchDevice = new QPointingDevice;
    m_touchDevice->setType(QInputDevice::DeviceType::TouchScreen);
    QPointingDevice::Capabilities touchCapabilities = QPointingDevice::Capability::Position | QPointingDevice::Capability::NormalizedPosition;
    if (mainScreen.traitCollection.forceTouchCapability == UIForceTouchCapabilityAvailable)
        touchCapabilities |= QPointingDevice::Capability::Pressure;
    m_touchDevice->setCapabilities(touchCapabilities);
    QWindowSystemInterface::registerInputDevice(m_touchDevice);
#if QT_CONFIG(tabletevent)
    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);
#endif
    QMacMimeRegistry::initializeMimeTypes();

    qsizetype size = QList<QPluginParsedMetaData>(m_optionalPlugins->metaData()).size();
    for (qsizetype i = 0; i < size; ++i)
        qobject_cast<QIosOptionalPluginInterface *>(m_optionalPlugins->instance(i))->initPlugin();
}

QIOSIntegration::~QIOSIntegration()
{
    delete m_fontDatabase;
    m_fontDatabase = 0;

#if !defined(Q_OS_TVOS) && !defined(QT_NO_CLIPBOARD)
    delete m_clipboard;
    m_clipboard = 0;
#endif

    QMacMimeRegistry::destroyMimeTypes();

    delete m_inputContext;
    m_inputContext = 0;

    foreach (QScreen *screen, QGuiApplication::screens())
        QWindowSystemInterface::handleScreenRemoved(screen->handle());

    delete m_platformServices;
    m_platformServices = 0;

    delete m_accessibility;
    m_accessibility = 0;

    delete m_optionalPlugins;
    m_optionalPlugins = 0;
}

bool QIOSIntegration::hasCapability(Capability cap) const
{
    switch (cap) {
#if QT_CONFIG(opengl)
    case BufferQueueingOpenGL:
        return true;
    case OpenGL:
    case ThreadedOpenGL:
        return true;
    case RasterGLSurface:
        return true;
#endif
    case ThreadedPixmaps:
        return true;
    case MultipleWindows:
        return true;
    case WindowManagement:
        return false;
    case ApplicationState:
        return true;
    case ForeignWindows:
        return true;
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformWindow *QIOSIntegration::createPlatformWindow(QWindow *window) const
{
    return new QIOSWindow(window);
}

QPlatformWindow *QIOSIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    return new QIOSWindow(window, nativeHandle);
}

QPlatformBackingStore *QIOSIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QRhiBackingStore(window);
}

#if QT_CONFIG(opengl)
// Used when the QWindow's surface type is set by the client to QSurface::OpenGLSurface
QPlatformOpenGLContext *QIOSIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    return new QIOSContext(context);
}
#endif

class QIOSOffscreenSurface : public QPlatformOffscreenSurface
{
public:
    QIOSOffscreenSurface(QOffscreenSurface *offscreenSurface) : QPlatformOffscreenSurface(offscreenSurface) {}

    QSurfaceFormat format() const override
    {
        Q_ASSERT(offscreenSurface());
        return offscreenSurface()->requestedFormat();
    }
    bool isValid() const override { return true; }
};

QPlatformOffscreenSurface *QIOSIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    return new QIOSOffscreenSurface(surface);
}

QAbstractEventDispatcher *QIOSIntegration::createEventDispatcher() const
{
    return QIOSEventDispatcher::create();
}

QPlatformFontDatabase * QIOSIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QIOSIntegration::clipboard() const
{
#ifndef Q_OS_TVOS
    return m_clipboard;
#else
    return QPlatformIntegration::clipboard();
#endif
}
#endif

QPlatformInputContext *QIOSIntegration::inputContext() const
{
    return m_inputContext;
}

QPlatformServices *QIOSIntegration::services() const
{
    return m_platformServices;
}

QVariant QIOSIntegration::styleHint(StyleHint hint) const
{
    switch (hint) {
    case StartDragTime:
        return 300;
    case PasswordMaskDelay:
        // this number is based on timing the native delay
        // since there is no API to get it
        return 2000;
    case ShowIsMaximized:
        return true;
    case SetFocusOnTouchRelease:
        return true;
    default:
        return QPlatformIntegration::styleHint(hint);
    }
}

QStringList QIOSIntegration::themeNames() const
{
    return QStringList(QLatin1StringView(QIOSTheme::name));
}

QPlatformTheme *QIOSIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1StringView(QIOSTheme::name))
        return new QIOSTheme;

    return QPlatformIntegration::createPlatformTheme(name);
}

QPointingDevice *QIOSIntegration::touchDevice()
{
    return m_touchDevice;
}

#if QT_CONFIG(accessibility)
QPlatformAccessibility *QIOSIntegration::accessibility() const
{
    if (!m_accessibility)
        m_accessibility = new QIOSPlatformAccessibility;
    return m_accessibility;
}
#endif

QPlatformNativeInterface *QIOSIntegration::nativeInterface() const
{
    return const_cast<QIOSIntegration *>(this);
}

void QIOSIntegration::beep() const
{
#if !TARGET_IPHONE_SIMULATOR
    AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
#endif
}

void QIOSIntegration::setApplicationBadge(qint64 number)
{
    UIApplication.sharedApplication.applicationIconBadgeNumber = number;
}

// ---------------------------------------------------------

void *QIOSIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle())
        return 0;

    QByteArray lowerCaseResource = resource.toLower();

    QIOSWindow *platformWindow = static_cast<QIOSWindow *>(window->handle());

    if (lowerCaseResource == "uiview")
        return reinterpret_cast<void *>(platformWindow->winId());

    return 0;
}

// ---------------------------------------------------------

QT_END_NAMESPACE

#include "moc_qiosintegration.cpp"
