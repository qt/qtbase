// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbcursor.h"
#include "qxcbkeyboard.h"
#include "qxcbbackingstore.h"
#include "qxcbnativeinterface.h"
#include "qxcbclipboard.h"
#include "qxcbeventqueue.h"
#include "qxcbeventdispatcher.h"
#if QT_CONFIG(draganddrop)
#include "qxcbdrag.h"
#endif
#include "qxcbglintegration.h"

#ifndef QT_NO_SESSIONMANAGER
#include "qxcbsessionmanager.h"
#endif
#include "qxcbxsettings.h"

#include <xcb/xcb.h>

#include <QtGui/private/qgenericunixfontdatabase_p.h>
#include <QtGui/private/qgenericunixservices_p.h>

#include <stdio.h>

#include <QtGui/private/qguiapplication_p.h>

#if QT_CONFIG(xcb_xlib)
#define register        /* C++17 deprecated register */
#include <X11/Xlib.h>
#undef register
#endif
#if QT_CONFIG(xcb_native_painting)
#include "qxcbnativepainting.h"
#include "qpixmap_x11_p.h"
#include "qbackingstore_x11_p.h"
#endif

#include <qpa/qplatforminputcontextfactory_p.h>
#include <private/qgenericunixthemes_p.h>
#include <qpa/qplatforminputcontext.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QOffscreenSurface>
#if QT_CONFIG(accessibility)
#include <qpa/qplatformaccessibility.h>
#if QT_CONFIG(accessibility_atspi_bridge)
#include <QtGui/private/qspiaccessiblebridge_p.h>
#endif
#endif

#include <QtCore/QFileInfo>

#if QT_CONFIG(vulkan)
#include "qxcbvulkaninstance.h"
#include "qxcbvulkanwindow.h"
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// Find out if our parent process is gdb by looking at the 'exe' symlink under /proc,.
// or, for older Linuxes, read out 'cmdline'.
static bool runningUnderDebugger()
{
#if defined(QT_DEBUG) && defined(Q_OS_LINUX)
    const QString parentProc = "/proc/"_L1 + QString::number(getppid());
    const QFileInfo parentProcExe(parentProc + "/exe"_L1);
    if (parentProcExe.isSymLink())
        return parentProcExe.symLinkTarget().endsWith("/gdb"_L1);
    QFile f(parentProc + "/cmdline"_L1);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    QByteArray s;
    char c;
    while (f.getChar(&c) && c) {
        if (c == '/')
            s.clear();
        else
            s += c;
    }
    return s == "gdb";
#else
    return false;
#endif
}

QXcbIntegration *QXcbIntegration::m_instance = nullptr;

QXcbIntegration::QXcbIntegration(const QStringList &parameters, int &argc, char **argv)
    : m_services(new QGenericUnixServices)
    , m_instanceName(nullptr)
    , m_canGrab(true)
    , m_defaultVisualId(UINT_MAX)
{
    Q_UNUSED(parameters);

    m_instance = this;
    qApp->setAttribute(Qt::AA_CompressHighFrequencyEvents, true);

    qRegisterMetaType<QXcbWindow*>();
#if QT_CONFIG(xcb_xlib)
    XInitThreads();
#endif
    m_nativeInterface.reset(new QXcbNativeInterface);

    // Parse arguments
    const char *displayName = nullptr;
    bool noGrabArg = false;
    bool doGrabArg = false;
    if (argc) {
        int j = 1;
        for (int i = 1; i < argc; i++) {
            QByteArray arg(argv[i]);
            if (arg.startsWith("--"))
                arg.remove(0, 1);
            if (arg == "-display" && i < argc - 1)
                displayName = argv[++i];
            else if (arg == "-name" && i < argc - 1)
                m_instanceName = argv[++i];
            else if (arg == "-nograb")
                noGrabArg = true;
            else if (arg == "-dograb")
                doGrabArg = true;
            else if (arg == "-visual" && i < argc - 1) {
                bool ok = false;
                m_defaultVisualId = QByteArray(argv[++i]).toUInt(&ok, 0);
                if (!ok)
                    m_defaultVisualId = UINT_MAX;
            }
            else
                argv[j++] = argv[i];
        }
        argc = j;
    } // argc

    bool underDebugger = runningUnderDebugger();
    if (noGrabArg && doGrabArg && underDebugger) {
        qWarning("Both -nograb and -dograb command line arguments specified. Please pick one. -nograb takes precedence");
        doGrabArg = false;
    }

#if defined(QT_DEBUG)
    if (!noGrabArg && !doGrabArg && underDebugger) {
        qCDebug(lcQpaXcb, "Qt: gdb: -nograb added to command-line options.\n"
                "\t Use the -dograb option to enforce grabbing.");
    }
#endif
    m_canGrab = (!underDebugger && !noGrabArg) || (underDebugger && doGrabArg);

    static bool canNotGrabEnv = qEnvironmentVariableIsSet("QT_XCB_NO_GRAB_SERVER");
    if (canNotGrabEnv)
        m_canGrab = false;

    m_connection = new QXcbConnection(m_nativeInterface.data(), m_canGrab, m_defaultVisualId, displayName);
    if (!m_connection->isConnected()) {
        delete m_connection;
        m_connection = nullptr;
        return;
    }

    m_fontDatabase.reset(new QGenericUnixFontDatabase());

#if QT_CONFIG(xcb_native_painting)
    if (nativePaintingEnabled()) {
        qCDebug(lcQpaXcb, "QXCB USING NATIVE PAINTING");
        qt_xcb_native_x11_info_init(connection());
    }
#endif
}

QXcbIntegration::~QXcbIntegration()
{
    delete m_connection;
    m_connection = nullptr;
    m_instance = nullptr;
}

QPlatformPixmap *QXcbIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
#if QT_CONFIG(xcb_native_painting)
    if (nativePaintingEnabled())
        return new QX11PlatformPixmap(type);
#endif

    return QPlatformIntegration::createPlatformPixmap(type);
}

QPlatformWindow *QXcbIntegration::createPlatformWindow(QWindow *window) const
{
    QXcbGlIntegration *glIntegration = nullptr;
    const bool isTrayIconWindow = QXcbWindow::isTrayIconWindow(window);;
    if (window->type() != Qt::Desktop && !isTrayIconWindow) {
        if (window->supportsOpenGL()) {
            glIntegration = connection()->glIntegration();
            if (glIntegration) {
                QXcbWindow *xcbWindow = glIntegration->createWindow(window);
                xcbWindow->create();
                return xcbWindow;
            }
#if QT_CONFIG(vulkan)
        } else if (window->surfaceType() == QSurface::VulkanSurface) {
            QXcbWindow *xcbWindow = new QXcbVulkanWindow(window);
            xcbWindow->create();
            return xcbWindow;
#endif
        }
    }

    Q_ASSERT(window->type() == Qt::Desktop || isTrayIconWindow || !window->supportsOpenGL()
             || (!glIntegration && window->surfaceType() == QSurface::RasterGLSurface)); // for VNC
    QXcbWindow *xcbWindow = new QXcbWindow(window);
    xcbWindow->create();
    return xcbWindow;
}

QPlatformWindow *QXcbIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    return new QXcbForeignWindow(window, nativeHandle);
}

#ifndef QT_NO_OPENGL
QPlatformOpenGLContext *QXcbIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QXcbGlIntegration *glIntegration = m_connection->glIntegration();
    if (!glIntegration) {
        qWarning("QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled");
        return nullptr;
    }
    return glIntegration->createPlatformOpenGLContext(context);
}

# if QT_CONFIG(xcb_glx_plugin)
QOpenGLContext *QXcbIntegration::createOpenGLContext(GLXContext context, void *visualInfo, QOpenGLContext *shareContext) const
{
    using namespace QNativeInterface::Private;
    if (auto *glxIntegration = dynamic_cast<QGLXIntegration*>(m_connection->glIntegration()))
        return glxIntegration->createOpenGLContext(context, visualInfo, shareContext);
    else
        return nullptr;
}
# endif

#if QT_CONFIG(egl)
QOpenGLContext *QXcbIntegration::createOpenGLContext(EGLContext context, EGLDisplay display, QOpenGLContext *shareContext) const
{
    using namespace QNativeInterface::Private;
    if (auto *eglIntegration = dynamic_cast<QEGLIntegration*>(m_connection->glIntegration()))
        return eglIntegration->createOpenGLContext(context, display, shareContext);
    else
        return nullptr;
}
#endif

#endif // QT_NO_OPENGL

QPlatformBackingStore *QXcbIntegration::createPlatformBackingStore(QWindow *window) const
{
    QPlatformBackingStore *backingStore = nullptr;

    const bool isTrayIconWindow = QXcbWindow::isTrayIconWindow(window);
    if (isTrayIconWindow) {
        backingStore = new QXcbSystemTrayBackingStore(window);
#if QT_CONFIG(xcb_native_painting)
    } else if (nativePaintingEnabled()) {
        backingStore = new QXcbNativeBackingStore(window);
#endif
    } else {
        backingStore = new QXcbBackingStore(window);
    }
    Q_ASSERT(backingStore);
    return backingStore;
}

QPlatformOffscreenSurface *QXcbIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QXcbScreen *screen = static_cast<QXcbScreen *>(surface->screen()->handle());
    QXcbGlIntegration *glIntegration = screen->connection()->glIntegration();
    if (!glIntegration) {
        qWarning("QXcbIntegration: Cannot create platform offscreen surface, neither GLX nor EGL are enabled");
        return nullptr;
    }
    return glIntegration->createPlatformOffscreenSurface(surface);
}

bool QXcbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case OpenGL:
    case ThreadedOpenGL:
    {
        if (const auto *integration = connection()->glIntegration())
            return cap != ThreadedOpenGL || integration->supportsThreadedOpenGL();
        return false;
    }

    case ThreadedPixmaps:
    case WindowMasks:
    case MultipleWindows:
    case ForeignWindows:
    case SyncState:
    case RasterGLSurface:
        return true;

    case SwitchableWidgetComposition:
    {
        return m_connection->glIntegration()
            && m_connection->glIntegration()->supportsSwitchableWidgetComposition();
    }

    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QAbstractEventDispatcher *QXcbIntegration::createEventDispatcher() const
{
    return QXcbEventDispatcher::createEventDispatcher(connection());
}

using namespace Qt::Literals::StringLiterals;
static const auto xsNetCursorBlink = "Net/CursorBlink"_ba;
static const auto xsNetCursorBlinkTime = "Net/CursorBlinkTime"_ba;
static const auto xsNetDoubleClickTime = "Net/DoubleClickTime"_ba;
static const auto xsNetDoubleClickDistance = "Net/DoubleClickDistance"_ba;
static const auto xsNetDndDragThreshold = "Net/DndDragThreshold"_ba;

void QXcbIntegration::initialize()
{
    const auto defaultInputContext = "compose"_L1;
    // Perform everything that may potentially need the event dispatcher (timers, socket
    // notifiers) here instead of the constructor.
    QString icStr = QPlatformInputContextFactory::requested();
    if (icStr.isNull())
        icStr = defaultInputContext;
    m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
    if (!m_inputContext && icStr != defaultInputContext && icStr != "none"_L1)
        m_inputContext.reset(QPlatformInputContextFactory::create(defaultInputContext));

    connection()->keyboard()->initialize();

    auto notifyThemeChanged = [](QXcbVirtualDesktop *, const QByteArray &, const QVariant &, void *) {
        QWindowSystemInterface::handleThemeChange();
    };

    auto *xsettings = connection()->primaryScreen()->xSettings();
    xsettings->registerCallbackForProperty(xsNetCursorBlink, notifyThemeChanged, this);
    xsettings->registerCallbackForProperty(xsNetCursorBlinkTime, notifyThemeChanged, this);
    xsettings->registerCallbackForProperty(xsNetDoubleClickTime, notifyThemeChanged, this);
    xsettings->registerCallbackForProperty(xsNetDoubleClickDistance, notifyThemeChanged, this);
    xsettings->registerCallbackForProperty(xsNetDndDragThreshold, notifyThemeChanged, this);
}

void QXcbIntegration::moveToScreen(QWindow *window, int screen)
{
    Q_UNUSED(window);
    Q_UNUSED(screen);
}

QPlatformFontDatabase *QXcbIntegration::fontDatabase() const
{
    return m_fontDatabase.data();
}

QPlatformNativeInterface * QXcbIntegration::nativeInterface() const
{
    return m_nativeInterface.data();
}

#ifndef QT_NO_CLIPBOARD
QPlatformClipboard *QXcbIntegration::clipboard() const
{
    return m_connection->clipboard();
}
#endif

#if QT_CONFIG(draganddrop)
#include <private/qsimpledrag_p.h>
QPlatformDrag *QXcbIntegration::drag() const
{
    static const bool useSimpleDrag = qEnvironmentVariableIsSet("QT_XCB_USE_SIMPLE_DRAG");
    if (Q_UNLIKELY(useSimpleDrag)) { // This is useful for testing purposes
        static QSimpleDrag *simpleDrag = nullptr;
        if (!simpleDrag)
            simpleDrag = new QSimpleDrag();
        return simpleDrag;
    }

    return m_connection->drag();
}
#endif

QPlatformInputContext *QXcbIntegration::inputContext() const
{
    return m_inputContext.data();
}

#if QT_CONFIG(accessibility)
QPlatformAccessibility *QXcbIntegration::accessibility() const
{
#if !defined(QT_NO_ACCESSIBILITY_ATSPI_BRIDGE)
    if (!m_accessibility) {
        Q_ASSERT_X(QCoreApplication::eventDispatcher(), "QXcbIntegration",
            "Initializing accessibility without event-dispatcher!");
        m_accessibility.reset(new QSpiAccessibleBridge());
    }
#endif

    return m_accessibility.data();
}
#endif

QPlatformServices *QXcbIntegration::services() const
{
    return m_services.data();
}

Qt::KeyboardModifiers QXcbIntegration::queryKeyboardModifiers() const
{
    return m_connection->queryKeyboardModifiers();
}

QList<int> QXcbIntegration::possibleKeys(const QKeyEvent *e) const
{
    return m_connection->keyboard()->possibleKeys(e);
}

QStringList QXcbIntegration::themeNames() const
{
    return QGenericUnixTheme::themeNames();
}

QPlatformTheme *QXcbIntegration::createPlatformTheme(const QString &name) const
{
    return QGenericUnixTheme::createUnixTheme(name);
}

#define RETURN_VALID_XSETTINGS(key) { \
    auto value = connection()->primaryScreen()->xSettings()->setting(key); \
    if (value.isValid()) return value; \
}

QVariant QXcbIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime: {
        bool ok = false;
        // If cursor blinking is off, returns 0 to keep the cursor awlays display.
        if (connection()->primaryScreen()->xSettings()->setting(xsNetCursorBlink).toInt(&ok) == 0 && ok)
            return 0;

        RETURN_VALID_XSETTINGS(xsNetCursorBlinkTime);
        break;
    }
    case QPlatformIntegration::MouseDoubleClickInterval:
        RETURN_VALID_XSETTINGS(xsNetDoubleClickTime);
        break;
    case QPlatformIntegration::MouseDoubleClickDistance:
        RETURN_VALID_XSETTINGS(xsNetDoubleClickDistance);
        break;
    case QPlatformIntegration::KeyboardInputInterval:
    case QPlatformIntegration::StartDragTime:
    case QPlatformIntegration::KeyboardAutoRepeatRate:
    case QPlatformIntegration::PasswordMaskDelay:
    case QPlatformIntegration::StartDragVelocity:
    case QPlatformIntegration::UseRtlExtensions:
    case QPlatformIntegration::PasswordMaskCharacter:
    case QPlatformIntegration::FlickMaximumVelocity:
    case QPlatformIntegration::FlickDeceleration:
        // TODO using various xcb, gnome or KDE settings
        break; // Not implemented, use defaults
    case QPlatformIntegration::FlickStartDistance:
    case QPlatformIntegration::StartDragDistance: {
        RETURN_VALID_XSETTINGS(xsNetDndDragThreshold);
        // The default (in QPlatformTheme::defaultThemeHint) is 10 pixels, but
        // on a high-resolution screen it makes sense to increase it.
        qreal dpi = 100;
        if (const QXcbScreen *screen = connection()->primaryScreen()) {
            if (screen->logicalDpi().first > dpi)
                dpi = screen->logicalDpi().first;
            if (screen->logicalDpi().second > dpi)
                dpi = screen->logicalDpi().second;
        }
        return (hint == QPlatformIntegration::FlickStartDistance ? qreal(15) : qreal(10)) * dpi / qreal(100);
    }
    case QPlatformIntegration::ShowIsFullScreen:
        // X11 always has support for windows, but the
        // window manager could prevent it (e.g. matchbox)
        return false;
    case QPlatformIntegration::ReplayMousePressOutsidePopup:
        return false;
    default:
        break;
    }
    return QPlatformIntegration::styleHint(hint);
}

static QString argv0BaseName()
{
    QString result;
    const QStringList arguments = QCoreApplication::arguments();
    if (!arguments.isEmpty() && !arguments.front().isEmpty()) {
        result = arguments.front();
        const int lastSlashPos = result.lastIndexOf(u'/');
        if (lastSlashPos != -1)
            result.remove(0, lastSlashPos + 1);
    }
    return result;
}

static const char resourceNameVar[] = "RESOURCE_NAME";

QByteArray QXcbIntegration::wmClass() const
{
    if (m_wmClass.isEmpty()) {
        // Instance name according to ICCCM 4.1.2.5
        QString name;
        if (m_instanceName)
            name = QString::fromLocal8Bit(m_instanceName);
        if (name.isEmpty() && qEnvironmentVariableIsSet(resourceNameVar))
            name = QString::fromLocal8Bit(qgetenv(resourceNameVar));
        if (name.isEmpty())
            name = argv0BaseName();

        // Note: QCoreApplication::applicationName() cannot be called from the QGuiApplication constructor,
        // hence this delayed initialization.
        QString className = QCoreApplication::applicationName();
        if (className.isEmpty()) {
            className = argv0BaseName();
            if (!className.isEmpty() && className.at(0).isLower())
                className[0] = className.at(0).toUpper();
        }

        if (!name.isEmpty() && !className.isEmpty())
            m_wmClass = std::move(name).toLocal8Bit() + '\0' + std::move(className).toLocal8Bit() + '\0';
    }
    return m_wmClass;
}

#if QT_CONFIG(xcb_sm)
QPlatformSessionManager *QXcbIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QXcbSessionManager(id, key);
}
#endif

void QXcbIntegration::sync()
{
    m_connection->sync();
}

// For QApplication::beep()
void QXcbIntegration::beep() const
{
    QScreen *priScreen = QGuiApplication::primaryScreen();
    if (!priScreen)
        return;
    QPlatformScreen *screen = priScreen->handle();
    if (!screen)
        return;
    xcb_connection_t *connection = static_cast<QXcbScreen *>(screen)->xcb_connection();
    xcb_bell(connection, 0);
    xcb_flush(connection);
}

bool QXcbIntegration::nativePaintingEnabled() const
{
#if QT_CONFIG(xcb_native_painting)
    static bool enabled = qEnvironmentVariableIsSet("QT_XCB_NATIVE_PAINTING");
    return enabled;
#else
    return false;
#endif
}

#if QT_CONFIG(vulkan)
QPlatformVulkanInstance *QXcbIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QXcbVulkanInstance(instance);
}
#endif

void QXcbIntegration::setApplicationBadge(qint64 number)
{
    auto unixServices = dynamic_cast<QGenericUnixServices *>(services());
    unixServices->setApplicationBadge(number);
}

QT_END_NAMESPACE
