// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsintegration.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"
#include "qwin10helpers.h"
#include "qwindowsmenu.h"
#include "qwindowsopenglcontext.h"

#include "qwindowsscreen.h"
#include "qwindowstheme.h"
#include "qwindowsservices.h"
#include <QtGui/private/qtgui-config_p.h>
#if QT_CONFIG(directwrite3)
#include <QtGui/private/qwindowsdirectwritefontdatabase_p.h>
#endif
#ifndef QT_NO_FREETYPE
#  include <QtGui/private/qwindowsfontdatabase_ft_p.h>
#endif
#include <QtGui/private/qwindowsfontdatabase_p.h>
#if QT_CONFIG(clipboard)
#  include "qwindowsclipboard.h"
#  if QT_CONFIG(draganddrop)
#    include "qwindowsdrag.h"
#  endif
#endif
#include "qwindowsinputcontext.h"
#include "qwindowskeymapper.h"
#if QT_CONFIG(accessibility)
#  include "uiautomation/qwindowsuiaaccessibility.h"
#endif

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qwindowsysteminterface.h>
#if QT_CONFIG(sessionmanager)
#  include "qwindowssessionmanager.h"
#endif
#include <QtGui/qpointingdevice.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtGui/qpa/qplatforminputcontextfactory_p.h>
#include <QtGui/qpa/qplatformcursor.h>

#include <QtGui/private/qwindowsguieventdispatcher_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qvariant.h>

#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/private/qfunctions_win_p.h>

#include <wrl.h>

#include <limits.h>

#if !defined(QT_NO_OPENGL)
#  include "qwindowsglcontext.h"
#endif

#include "qwindowsopengltester.h"

#if QT_CONFIG(cpp_winrt)
#  include <QtCore/private/qt_winrtbase_p.h>
#  include <winrt/Windows.UI.Notifications.h>
#  include <winrt/Windows.Data.Xml.Dom.h>
#  include <winrt/Windows.Foundation.h>
#  include <winrt/Windows.UI.ViewManagement.h>
#endif

#include <memory>

static inline void initOpenGlBlacklistResources()
{
    Q_INIT_RESOURCE(openglblacklists);
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QWindowsIntegration
    \brief QPlatformIntegration implementation for Windows.
    \internal

    \section1 Programming Considerations

    The platform plugin should run on Desktop Windows from Windows XP onwards
    and Windows Embedded.

    It should compile with:
    \list
    \li Microsoft Visual Studio 2013 or later (using the Microsoft Windows SDK,
        (\c Q_CC_MSVC).
    \li Stock \l{http://mingw.org/}{MinGW} (\c Q_CC_MINGW).
        This version ships with headers that are missing a lot of WinAPI.
    \li MinGW distributions using GCC 4.7 or higher and a recent MinGW-w64 runtime API,
        such as \l{http://tdm-gcc.tdragon.net/}{TDM-GCC}, or
        \l{http://mingwbuilds.sourceforge.net/}{MinGW-builds}
        (\c Q_CC_MINGW and \c __MINGW64_VERSION_MAJOR indicating the version).
        MinGW-w64 provides more complete headers (compared to stock MinGW from mingw.org),
        including a considerable part of the Windows SDK.
    \endlist
*/

struct QWindowsIntegrationPrivate
{
    Q_DISABLE_COPY_MOVE(QWindowsIntegrationPrivate)
    explicit QWindowsIntegrationPrivate() = default;
    ~QWindowsIntegrationPrivate();

    void parseOptions(QWindowsIntegration *q, const QStringList &paramList);

    unsigned m_options = 0;
    QWindowsContext m_context;
    QPlatformFontDatabase *m_fontDatabase = nullptr;
#if QT_CONFIG(clipboard)
    QWindowsClipboard m_clipboard;
#  if QT_CONFIG(draganddrop)
    QWindowsDrag m_drag;
#  endif
#endif
#ifndef QT_NO_OPENGL
    QMutex m_staticContextLock;
    QScopedPointer<QWindowsStaticOpenGLContext> m_staticOpenGLContext;
#endif // QT_NO_OPENGL
    QScopedPointer<QPlatformInputContext> m_inputContext;
#if QT_CONFIG(accessibility)
   QWindowsUiaAccessibility m_accessibility;
#endif
    QWindowsServices m_services;
};

template <typename IntType>
bool parseIntOption(const QString &parameter,const QLatin1StringView &option,
                    IntType minimumValue, IntType maximumValue, IntType *target)
{
    const int valueLength = parameter.size() - option.size() - 1;
    if (valueLength < 1 || !parameter.startsWith(option) || parameter.at(option.size()) != u'=')
        return false;
    bool ok;
    const auto valueRef = QStringView{parameter}.right(valueLength);
    const int value = valueRef.toInt(&ok);
    if (ok) {
        if (value >= int(minimumValue) && value <= int(maximumValue))
            *target = static_cast<IntType>(value);
        else {
            qWarning() << "Value" << value << "for option" << option << "out of range"
                << minimumValue << ".." << maximumValue;
        }
    } else {
        qWarning() << "Invalid value" << valueRef << "for option" << option;
    }
    return true;
}

using DarkModeHandlingFlag = QNativeInterface::Private::QWindowsApplication::DarkModeHandlingFlag;
using DarkModeHandling = QNativeInterface::Private::QWindowsApplication::DarkModeHandling;

static inline unsigned parseOptions(const QStringList &paramList,
                                    int *tabletAbsoluteRange,
                                    QtWindows::DpiAwareness *dpiAwareness,
                                    DarkModeHandling *darkModeHandling)
{
    unsigned options = 0;
    for (const QString &param : paramList) {
        if (param.startsWith(u"fontengine=")) {
            if (param.endsWith(u"directwrite")) {
                options |= QWindowsIntegration::FontDatabaseDirectWrite;
            } else if (param.endsWith(u"freetype")) {
                options |= QWindowsIntegration::FontDatabaseFreeType;
            } else if (param.endsWith(u"native")) {
                options |= QWindowsIntegration::FontDatabaseNative;
            }
        } else if (param.startsWith(u"dialogs=")) {
            if (param.endsWith(u"xp")) {
                options |= QWindowsIntegration::XpNativeDialogs;
            } else if (param.endsWith(u"none")) {
                options |= QWindowsIntegration::NoNativeDialogs;
            }
        } else if (param == u"altgr") {
            options |= QWindowsIntegration::DetectAltGrModifier;
        } else if (param == u"gl=gdi") {
            options |= QWindowsIntegration::DisableArb;
        } else if (param == u"nodirectwrite") {
            options |= QWindowsIntegration::DontUseDirectWriteFonts;
        } else if (param == u"nocolorfonts") {
            options |= QWindowsIntegration::DontUseColorFonts;
        } else if (param == u"nomousefromtouch") {
            options |= QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch;
        } else if (parseIntOption(param, "verbose"_L1, 0, INT_MAX, &QWindowsContext::verbose)
            || parseIntOption(param, "tabletabsoluterange"_L1, 0, INT_MAX, tabletAbsoluteRange)
            || parseIntOption(param, "dpiawareness"_L1, QtWindows::DpiAwareness::Invalid,
                    QtWindows::DpiAwareness::PerMonitorVersion2, dpiAwareness)) {
        } else if (param == u"menus=native") {
            options |= QWindowsIntegration::AlwaysUseNativeMenus;
        } else if (param == u"menus=none") {
            options |= QWindowsIntegration::NoNativeMenus;
        } else if (param == u"nowmpointer") {
            options |= QWindowsIntegration::DontUseWMPointer;
        } else if (param == u"reverse") {
            options |= QWindowsIntegration::RtlEnabled;
        } else if (param == u"darkmode=0") {
            *darkModeHandling = {};
        } else if (param == u"darkmode=1") {
            darkModeHandling->setFlag(DarkModeHandlingFlag::DarkModeWindowFrames);
            darkModeHandling->setFlag(DarkModeHandlingFlag::DarkModeStyle, false);
        } else if (param == u"darkmode=2") {
            darkModeHandling->setFlag(DarkModeHandlingFlag::DarkModeWindowFrames);
            darkModeHandling->setFlag(DarkModeHandlingFlag::DarkModeStyle);
        } else {
            qWarning() << "Unknown option" << param;
        }
    }
    return options;
}

void QWindowsIntegrationPrivate::parseOptions(QWindowsIntegration *q, const QStringList &paramList)
{
    initOpenGlBlacklistResources();

    static bool dpiAwarenessSet = false;
    // Default to per-monitor-v2 awareness (if available)
    QtWindows::DpiAwareness dpiAwareness = QtWindows::DpiAwareness::PerMonitorVersion2;

    int tabletAbsoluteRange = -1;
    DarkModeHandling darkModeHandling = DarkModeHandlingFlag::DarkModeWindowFrames
                                      | DarkModeHandlingFlag::DarkModeStyle;
    m_options = ::parseOptions(paramList, &tabletAbsoluteRange, &dpiAwareness, &darkModeHandling);
    q->setDarkModeHandling(darkModeHandling);
    QWindowsFontDatabase::setFontOptions(m_options);
    if (tabletAbsoluteRange >= 0)
        QWindowsContext::setTabletAbsoluteRange(tabletAbsoluteRange);

    if (m_context.initPointer(m_options))
        QCoreApplication::setAttribute(Qt::AA_CompressHighFrequencyEvents);
    else
        m_context.initTablet();
    QWindowSystemInterfacePrivate::TabletEvent::setPlatformSynthesizesMouse(false);

    if (!dpiAwarenessSet) { // Set only once in case of repeated instantiations of QGuiApplication.
        if (!QCoreApplication::testAttribute(Qt::AA_PluginApplication)) {
            m_context.setProcessDpiAwareness(dpiAwareness);
            qCDebug(lcQpaWindow) << "DpiAwareness=" << dpiAwareness
                << "effective process DPI awareness=" << QWindowsContext::processDpiAwareness();
        }
        dpiAwarenessSet = true;
    }

    m_context.initTouch(m_options);
    QPlatformCursor::setCapability(QPlatformCursor::OverrideCursor);

    m_context.initPowerNotificationHandler();
}

QWindowsIntegrationPrivate::~QWindowsIntegrationPrivate()
{
    delete m_fontDatabase;
}

QWindowsIntegration *QWindowsIntegration::m_instance = nullptr;

QWindowsIntegration::QWindowsIntegration(const QStringList &paramList) :
    d(new QWindowsIntegrationPrivate)
{
    m_instance = this;
    d->parseOptions(this, paramList);
#if QT_CONFIG(clipboard)
    d->m_clipboard.registerViewer();
#endif
    d->m_context.screenManager().initialize();
    d->m_context.setDetectAltGrModifier((d->m_options & DetectAltGrModifier) != 0);
}

QWindowsIntegration::~QWindowsIntegration()
{
    m_instance = nullptr;
}

void QWindowsIntegration::initialize()
{
    QString icStr = QPlatformInputContextFactory::requested();
    icStr.isNull() ? d->m_inputContext.reset(new QWindowsInputContext)
                   : d->m_inputContext.reset(QPlatformInputContextFactory::create(icStr));
}

bool QWindowsIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps:
        return true;
#ifndef QT_NO_OPENGL
    case OpenGL:
        return true;
    case ThreadedOpenGL:
        if (const QWindowsStaticOpenGLContext *glContext = QWindowsIntegration::staticOpenGLContext())
            return glContext->supportsThreadedOpenGL();
        return false;
#endif // !QT_NO_OPENGL
    case WindowMasks:
        return true;
    case MultipleWindows:
        return true;
    case ForeignWindows:
        return true;
    case RasterGLSurface:
        return true;
    case AllGLFunctionsQueryable:
        return true;
    case SwitchableWidgetComposition:
        return false; // QTBUG-68329 QTBUG-53515 QTBUG-54734
    default:
        return QPlatformIntegration::hasCapability(cap);
    }
    return false;
}

QPlatformWindow *QWindowsIntegration::createPlatformWindow(QWindow *window) const
{
    if (window->type() == Qt::Desktop) {
        auto *result = new QWindowsDesktopWindow(window);
        qCDebug(lcQpaWindow) << "Desktop window:" << window
            << Qt::showbase << Qt::hex << result->winId() << Qt::noshowbase << Qt::dec << result->geometry();
        return result;
    }

    QWindowsWindowData requested;
    requested.flags = window->flags();
    requested.geometry = window->isTopLevel()
        ? QHighDpi::toNativePixels(window->geometry(), window)
        : QHighDpi::toNativeLocalPosition(window->geometry(), window);
    if (!(requested.flags & Qt::FramelessWindowHint)) {
        // Apply custom margins (see  QWindowsWindow::setCustomMargins())).
        const QVariant customMarginsV = window->property("_q_windowsCustomMargins");
        if (customMarginsV.isValid())
            requested.customMargins = qvariant_cast<QMargins>(customMarginsV);
    }

    QWindowsWindowData obtained =
        QWindowsWindowData::create(window, requested,
                                   QWindowsWindow::formatWindowTitle(window->title()));
    qCDebug(lcQpaWindow).nospace()
        << __FUNCTION__ << ' ' << window
        << "\n    Requested: " << requested.geometry << " frame incl.="
        << QWindowsGeometryHint::positionIncludesFrame(window)
        << ' ' << requested.flags
        << "\n    Obtained : " << obtained.geometry << " margins=" << obtained.fullFrameMargins
        << " handle=" << obtained.hwnd << ' ' << obtained.flags << '\n';

    if (Q_UNLIKELY(!obtained.hwnd))
        return nullptr;

    QWindowsWindow *result = createPlatformWindowHelper(window, obtained);
    Q_ASSERT(result);

    if (window->isTopLevel() && !QWindowsContext::shouldHaveNonClientDpiScaling(window))
        result->setFlag(QWindowsWindow::DisableNonClientScaling);

    if (QWindowsMenuBar *menuBarToBeInstalled = QWindowsMenuBar::menuBarOf(window))
        menuBarToBeInstalled->install(result);

    return result;
}

QPlatformWindow *QWindowsIntegration::createForeignWindow(QWindow *window, WId nativeHandle) const
{
    const HWND hwnd = reinterpret_cast<HWND>(nativeHandle);
    if (!IsWindow(hwnd)) {
       qWarning("Windows QPA: Invalid foreign window ID %p.", hwnd);
       return nullptr;
    }
    auto *result = new QWindowsForeignWindow(window, hwnd);
    const QRect obtainedGeometry = result->geometry();
    QScreen *screen = nullptr;
    if (const QPlatformScreen *pScreen = result->screenForGeometry(obtainedGeometry))
        screen = pScreen->screen();
    if (screen && screen != window->screen())
        window->setScreen(screen);
    qCDebug(lcQpaWindow) << "Foreign window:" << window << Qt::showbase << Qt::hex
        << result->winId() << Qt::noshowbase << Qt::dec << obtainedGeometry << screen;
    return result;
}

// Overridden to return a QWindowsDirect2DWindow in Direct2D plugin.
QWindowsWindow *QWindowsIntegration::createPlatformWindowHelper(QWindow *window, const QWindowsWindowData &data) const
{
    return new QWindowsWindow(window, data);
}

#ifndef QT_NO_OPENGL

QWindowsStaticOpenGLContext *QWindowsStaticOpenGLContext::doCreate()
{
#if defined(QT_OPENGL_DYNAMIC)
    QWindowsOpenGLTester::Renderer requestedRenderer = QWindowsOpenGLTester::requestedRenderer();
    switch (requestedRenderer) {
    case QWindowsOpenGLTester::DesktopGl:
        if (QWindowsStaticOpenGLContext *glCtx = QOpenGLStaticContext::create()) {
            if ((QWindowsOpenGLTester::supportedRenderers(requestedRenderer) & QWindowsOpenGLTester::DisableRotationFlag)
                && !QWindowsScreen::setOrientationPreference(Qt::LandscapeOrientation)) {
                qCWarning(lcQpaGl, "Unable to disable rotation.");
            }
            return glCtx;
        }
        qCWarning(lcQpaGl, "System OpenGL failed. Falling back to Software OpenGL.");
        return QOpenGLStaticContext::create(true);
    case QWindowsOpenGLTester::SoftwareRasterizer:
        if (QWindowsStaticOpenGLContext *swCtx = QOpenGLStaticContext::create(true))
            return swCtx;
        qCWarning(lcQpaGl, "Software OpenGL failed. Falling back to system OpenGL.");
        if (QWindowsOpenGLTester::supportedRenderers(requestedRenderer) & QWindowsOpenGLTester::DesktopGl)
            return QOpenGLStaticContext::create();
        return nullptr;
    default:
        break;
    }

    const QWindowsOpenGLTester::Renderers supportedRenderers = QWindowsOpenGLTester::supportedRenderers(requestedRenderer);
    if (supportedRenderers.testFlag(QWindowsOpenGLTester::DisableProgramCacheFlag)
        && !QCoreApplication::testAttribute(Qt::AA_DisableShaderDiskCache)) {
        QCoreApplication::setAttribute(Qt::AA_DisableShaderDiskCache);
    }
    if (supportedRenderers & QWindowsOpenGLTester::DesktopGl) {
        if (QWindowsStaticOpenGLContext *glCtx = QOpenGLStaticContext::create()) {
            if ((supportedRenderers & QWindowsOpenGLTester::DisableRotationFlag)
                && !QWindowsScreen::setOrientationPreference(Qt::LandscapeOrientation)) {
                qCWarning(lcQpaGl, "Unable to disable rotation.");
            }
            return glCtx;
        }
    }
    return QOpenGLStaticContext::create(true);
#else
    return QOpenGLStaticContext::create();
#endif
}

QWindowsStaticOpenGLContext *QWindowsStaticOpenGLContext::create()
{
    return QWindowsStaticOpenGLContext::doCreate();
}

QPlatformOpenGLContext *QWindowsIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    qCDebug(lcQpaGl) << __FUNCTION__ << context->format();
    if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext()) {
        std::unique_ptr<QWindowsOpenGLContext> result(staticOpenGLContext->createContext(context));
        if (result->isValid())
            return result.release();
    }
    return nullptr;
}

QOpenGLContext::OpenGLModuleType QWindowsIntegration::openGLModuleType()
{
#if !defined(QT_OPENGL_DYNAMIC)
    return QOpenGLContext::LibGL;
#else
    if (const QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
        return staticOpenGLContext->moduleType();
    return QOpenGLContext::LibGL;
#endif
}

HMODULE QWindowsIntegration::openGLModuleHandle() const
{
    if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
        return static_cast<HMODULE>(staticOpenGLContext->moduleHandle());

    return nullptr;
}

QOpenGLContext *QWindowsIntegration::createOpenGLContext(HGLRC ctx, HWND window, QOpenGLContext *shareContext) const
{
    if (!ctx || !window)
        return nullptr;

    if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext()) {
        std::unique_ptr<QWindowsOpenGLContext> result(staticOpenGLContext->createContext(ctx, window));
        if (result->isValid()) {
            auto *context = new QOpenGLContext;
            context->setShareContext(shareContext);
            auto *contextPrivate = QOpenGLContextPrivate::get(context);
            contextPrivate->adopt(result.release());
            return context;
        }
    }

    return nullptr;
}

QWindowsStaticOpenGLContext *QWindowsIntegration::staticOpenGLContext()
{
    QWindowsIntegration *integration = QWindowsIntegration::instance();
    if (!integration)
        return nullptr;
    QWindowsIntegrationPrivate *d = integration->d.data();
    QMutexLocker lock(&d->m_staticContextLock);
    if (d->m_staticOpenGLContext.isNull())
        d->m_staticOpenGLContext.reset(QWindowsStaticOpenGLContext::create());
    return d->m_staticOpenGLContext.data();
}
#endif // !QT_NO_OPENGL

QPlatformFontDatabase *QWindowsIntegration::fontDatabase() const
{
    if (!d->m_fontDatabase) {
#if QT_CONFIG(directwrite3)
        if (d->m_options & QWindowsIntegration::FontDatabaseDirectWrite)
            d->m_fontDatabase = new QWindowsDirectWriteFontDatabase;
        else
#endif
#ifndef QT_NO_FREETYPE
        if (d->m_options & QWindowsIntegration::FontDatabaseFreeType)
            d->m_fontDatabase = new QWindowsFontDatabaseFT;
        else
#endif // QT_NO_FREETYPE
        d->m_fontDatabase = new QWindowsFontDatabase();
    }
    return d->m_fontDatabase;
}

#ifdef SPI_GETKEYBOARDSPEED
static inline int keyBoardAutoRepeatRateMS()
{
  DWORD time = 0;
  if (SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &time, 0))
      return time ? 1000 / static_cast<int>(time) : 500;
  return 30;
}
#endif

QVariant QWindowsIntegration::styleHint(QPlatformIntegration::StyleHint hint) const
{
    switch (hint) {
    case QPlatformIntegration::CursorFlashTime:
        if (const unsigned timeMS = GetCaretBlinkTime())
            return QVariant(timeMS != INFINITE ? int(timeMS) * 2 : 0);
        break;
#ifdef SPI_GETKEYBOARDSPEED
    case KeyboardAutoRepeatRate:
        return QVariant(keyBoardAutoRepeatRateMS());
#endif
    case QPlatformIntegration::ShowIsMaximized:
    case QPlatformIntegration::StartDragTime:
    case QPlatformIntegration::StartDragDistance:
    case QPlatformIntegration::KeyboardInputInterval:
    case QPlatformIntegration::ShowIsFullScreen:
    case QPlatformIntegration::PasswordMaskDelay:
    case QPlatformIntegration::StartDragVelocity:
        break; // Not implemented
    case QPlatformIntegration::FontSmoothingGamma:
        return QVariant(QWindowsFontDatabase::fontSmoothingGamma());
    case QPlatformIntegration::MouseDoubleClickInterval:
        if (const UINT ms = GetDoubleClickTime())
            return QVariant(int(ms));
        break;
    case QPlatformIntegration::UseRtlExtensions:
        return QVariant(d->m_context.useRTLExtensions());
    default:
        break;
    }
    return QPlatformIntegration::styleHint(hint);
}

Qt::KeyboardModifiers QWindowsIntegration::queryKeyboardModifiers() const
{
    return QWindowsKeyMapper::queryKeyboardModifiers();
}

QList<int> QWindowsIntegration::possibleKeys(const QKeyEvent *e) const
{
    return d->m_context.possibleKeys(e);
}

#if QT_CONFIG(clipboard)
QPlatformClipboard * QWindowsIntegration::clipboard() const
{
    return &d->m_clipboard;
}
#  if QT_CONFIG(draganddrop)
QPlatformDrag *QWindowsIntegration::drag() const
{
    return &d->m_drag;
}
#  endif // QT_CONFIG(draganddrop)
#endif // !QT_NO_CLIPBOARD

QPlatformInputContext * QWindowsIntegration::inputContext() const
{
    return d->m_inputContext.data();
}

#if QT_CONFIG(accessibility)
QPlatformAccessibility *QWindowsIntegration::accessibility() const
{
    return &d->m_accessibility;
}
#endif

unsigned QWindowsIntegration::options() const
{
    return d->m_options;
}

#if QT_CONFIG(sessionmanager)
QPlatformSessionManager *QWindowsIntegration::createPlatformSessionManager(const QString &id, const QString &key) const
{
    return new QWindowsSessionManager(id, key);
}
#endif

QAbstractEventDispatcher * QWindowsIntegration::createEventDispatcher() const
{
    return new QWindowsGuiEventDispatcher;
}

QStringList QWindowsIntegration::themeNames() const
{
    return QStringList(QLatin1StringView(QWindowsTheme::name));
}

QPlatformTheme *QWindowsIntegration::createPlatformTheme(const QString &name) const
{
    if (name == QLatin1StringView(QWindowsTheme::name))
        return new QWindowsTheme;
    return QPlatformIntegration::createPlatformTheme(name);
}

QPlatformServices *QWindowsIntegration::services() const
{
    return &d->m_services;
}

void QWindowsIntegration::beep() const
{
    MessageBeep(MB_OK);  // For QApplication
}

void QWindowsIntegration::setApplicationBadge(qint64 number)
{
    // Clamp to positive numbers, as the Windows API doesn't support negative numbers
    number = qMax(0, number);

    // Persist, so we can re-apply it on setting changes and Explorer restart
    m_applicationBadgeNumber = number;

    static const bool isWindows11 = QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11;

#if QT_CONFIG(cpp_winrt)
    // We prefer the native BadgeUpdater API, that allows us to set a number directly,
    // but it requires that the application has a package identity, and also doesn't
    // seem to work in all cases on < Windows 11.
    if (isWindows11 && qt_win_hasPackageIdentity()) {
        using namespace winrt::Windows::UI::Notifications;
        auto badgeXml = BadgeUpdateManager::GetTemplateContent(BadgeTemplateType::BadgeNumber);
        badgeXml.SelectSingleNode(L"//badge/@value").NodeValue(winrt::box_value(winrt::to_hstring(number)));
        BadgeUpdateManager::CreateBadgeUpdaterForApplication().Update(BadgeNotification(badgeXml));
        return;
    }
#endif

    // Fallback for non-packaged apps, Windows 10, or Qt builds without WinRT/C++ support

    if (!number) {
        // Clear badge
        setApplicationBadge(QImage());
        return;
    }

    const bool isDarkMode = QWindowsContext::isDarkMode();

    QColor badgeColor;
    QColor textColor;

#if QT_CONFIG(cpp_winrt)
    if (isWindows11) {
        // Match colors used by BadgeUpdater
        static const auto fromUIColor = [](winrt::Windows::UI::Color &&color) {
            return QColor(color.R, color.G, color.B, color.A);
        };
        using namespace winrt::Windows::UI::ViewManagement;
        const auto settings = UISettings();
        badgeColor = fromUIColor(settings.GetColorValue(isDarkMode ?
            UIColorType::AccentLight2 : UIColorType::Accent));
        textColor = fromUIColor(settings.GetColorValue(UIColorType::Background));
    }
#endif

    if (!badgeColor.isValid()) {
        // Fall back to basic badge colors, based on Windows 10 look
        badgeColor = isDarkMode ? Qt::black : QColor(220, 220, 220);
        badgeColor.setAlphaF(0.5f);
        textColor = isDarkMode ? Qt::white : Qt::black;
    }

    const auto devicePixelRatio = qApp->devicePixelRatio();

    static const QSize iconBaseSize(16, 16);
    QImage image(iconBaseSize * devicePixelRatio,
        QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);

    QRect badgeRect = image.rect();
    QPen badgeBorderPen = Qt::NoPen;
    if (!isWindows11) {
        QColor badgeBorderColor = textColor;
        badgeBorderColor.setAlphaF(0.5f);
        badgeBorderPen = badgeBorderColor;
        badgeRect.adjust(1, 1, -1, -1);
    }
    painter.setBrush(badgeColor);
    painter.setPen(badgeBorderPen);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawEllipse(badgeRect);

    auto pixelSize = qCeil(10.5 * devicePixelRatio);
    // Unlike the BadgeUpdater API we're limited by a square
    // badge, so adjust the font size when above two digits.
    const bool textOverflow = number > 99;
    if (textOverflow)
        pixelSize *= 0.8;

    QFont font = painter.font();
    font.setPixelSize(pixelSize);
    font.setWeight(isWindows11 ? QFont::Medium : QFont::DemiBold);
    painter.setFont(font);

    painter.setRenderHint(QPainter::TextAntialiasing, devicePixelRatio > 1);
    painter.setPen(textColor);

    auto text = textOverflow ? u"99+"_s : QString::number(number);
    painter.translate(textOverflow ? 1 : 0, textOverflow ? 0 : -1);
    painter.drawText(image.rect(), Qt::AlignCenter, text);

    painter.end();

    setApplicationBadge(image);
}

void QWindowsIntegration::setApplicationBadge(const QImage &image)
{
    QComHelper comHelper;

    using Microsoft::WRL::ComPtr;

    ComPtr<ITaskbarList3> taskbarList;
    CoCreateInstance(CLSID_TaskbarList, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&taskbarList));
    if (!taskbarList) {
        // There may not be any windows with a task bar button yet,
        // in which case we'll apply the badge once a window with
        // a button has been created.
        return;
    }

    const auto hIcon = image.toHICON();

    // Apply the icon to all top level windows, since the badge is
    // set on an application level. If one of the windows go away
    // the other windows will take over in showing the badge.
    const auto topLevelWindows = QGuiApplication::topLevelWindows();
    for (auto *topLevelWindow : topLevelWindows) {
        if (!topLevelWindow->handle())
            continue;
        auto hwnd = reinterpret_cast<HWND>(topLevelWindow->winId());
        taskbarList->SetOverlayIcon(hwnd, hIcon, L"");
    }

    DestroyIcon(hIcon);

    // FIXME: Update icon when the application scale factor changes.
    // Doing so in response to screen DPI changes is too soon, as the
    // task bar is not yet ready for an updated icon, and will just
    // result in a blurred icon even if our icon is high-DPI.
}

void QWindowsIntegration::updateApplicationBadge()
{
    // The system color settings have changed, or we are reacting
    // to a task bar button being created for the fist time or after
    // Explorer had crashed and re-started. In any case, re-apply the
    // badge so that everything is up to date.
    if (m_applicationBadgeNumber)
        setApplicationBadge(m_applicationBadgeNumber);
}

#if QT_CONFIG(vulkan)
QPlatformVulkanInstance *QWindowsIntegration::createPlatformVulkanInstance(QVulkanInstance *instance) const
{
    return new QWindowsVulkanInstance(instance);
}
#endif

QT_END_NAMESPACE
