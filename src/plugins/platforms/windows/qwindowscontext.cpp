/****************************************************************************
**
** Copyright (C) 2013 Samuel Gaist <samuel.gaist@edeltech.ch>
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

#include "qwindowscontext.h"
#include "qwindowsintegration.h"
#include "qwindowswindow.h"
#include "qwindowskeymapper.h"
#include "qwindowsnativeinterface.h"
#include "qwindowsmousehandler.h"
#include "qwindowspointerhandler.h"
#include "qtwindowsglobal.h"
#include "qwindowsmenu.h"
#include "qwindowsmime.h"
#include "qwindowsinputcontext.h"
#if QT_CONFIG(tabletevent)
#  include "qwindowstabletsupport.h"
#endif
#include "qwindowstheme.h"
#include <private/qguiapplication_p.h>
#if QT_CONFIG(accessibility)
#  include "uiautomation/qwindowsuiaaccessibility.h"
#endif
#if QT_CONFIG(sessionmanager)
# include <private/qsessionmanager_p.h>
# include "qwindowssessionmanager.h"
#endif
#include "qwindowsscreen.h"
#include "qwindowstheme.h"

#include <QtGui/qwindow.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qopenglcontext.h>

#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qoperatingsystemversion.h>
#include <QtCore/qsysinfo.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/quuid.h>
#include <QtCore/private/qsystemlibrary_p.h>
#include <QtCore/private/qwinregistry_p.h>

#include <QtEventDispatcherSupport/private/qwindowsguieventdispatcher_p.h>

#include <stdlib.h>
#include <stdio.h>
#include <windowsx.h>
#include <comdef.h>
#include <dbt.h>
#include <wtsapi32.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaWindows, "qt.qpa.windows")
Q_LOGGING_CATEGORY(lcQpaEvents, "qt.qpa.events")
Q_LOGGING_CATEGORY(lcQpaGl, "qt.qpa.gl")
Q_LOGGING_CATEGORY(lcQpaMime, "qt.qpa.mime")
Q_LOGGING_CATEGORY(lcQpaInputMethods, "qt.qpa.input.methods")
Q_LOGGING_CATEGORY(lcQpaDialogs, "qt.qpa.dialogs")
Q_LOGGING_CATEGORY(lcQpaMenus, "qt.qpa.menus")
Q_LOGGING_CATEGORY(lcQpaTablet, "qt.qpa.input.tablet")
Q_LOGGING_CATEGORY(lcQpaAccessibility, "qt.qpa.accessibility")
Q_LOGGING_CATEGORY(lcQpaUiAutomation, "qt.qpa.uiautomation")
Q_LOGGING_CATEGORY(lcQpaTrayIcon, "qt.qpa.trayicon")

int QWindowsContext::verbose = 0;

#if !defined(LANG_SYRIAC)
#    define LANG_SYRIAC 0x5a
#endif

static inline bool useRTL_Extensions()
{
    // Since the IsValidLanguageGroup/IsValidLocale functions always return true on
    // Vista, check the Keyboard Layouts for enabling RTL.
    if (const int nLayouts = GetKeyboardLayoutList(0, nullptr)) {
        QScopedArrayPointer<HKL> lpList(new HKL[nLayouts]);
        GetKeyboardLayoutList(nLayouts, lpList.data());
        for (int i = 0; i < nLayouts; ++i) {
            switch (PRIMARYLANGID((quintptr)lpList[i])) {
            case LANG_ARABIC:
            case LANG_HEBREW:
            case LANG_FARSI:
            case LANG_SYRIAC:
                return true;
            default:
                break;
            }
        }
    }
    return false;
}

#if QT_CONFIG(sessionmanager)
static inline QWindowsSessionManager *platformSessionManager()
{
    auto *guiPrivate = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(qApp));
    auto *managerPrivate = static_cast<QSessionManagerPrivate*>(QObjectPrivate::get(guiPrivate->session_manager));
    return static_cast<QWindowsSessionManager *>(managerPrivate->platformSessionManager);
}

static inline bool sessionManagerInteractionBlocked()
{
    return platformSessionManager()->isInteractionBlocked();
}
#else // QT_CONFIG(sessionmanager)
static inline bool sessionManagerInteractionBlocked() { return false; }
#endif

static inline int windowDpiAwareness(HWND hwnd)
{
    return QWindowsContext::user32dll.getWindowDpiAwarenessContext && QWindowsContext::user32dll.getAwarenessFromDpiAwarenessContext
        ? QWindowsContext::user32dll.getAwarenessFromDpiAwarenessContext(QWindowsContext::user32dll.getWindowDpiAwarenessContext(hwnd))
        : -1;
}

// Note: This only works within WM_NCCREATE
static bool enableNonClientDpiScaling(HWND hwnd)
{
    bool result = false;
    if (QWindowsContext::user32dll.enableNonClientDpiScaling && windowDpiAwareness(hwnd) == 2) {
        result = QWindowsContext::user32dll.enableNonClientDpiScaling(hwnd) != FALSE;
        if (!result) {
            const DWORD errorCode = GetLastError();
            qErrnoWarning(int(errorCode), "EnableNonClientDpiScaling() failed for HWND %p (%lu)",
                          hwnd, errorCode);
        }
    }
    return result;
}

/*!
    \class QWindowsUser32DLL
    \brief Struct that contains dynamically resolved symbols of User32.dll.

    The stub libraries shipped with the MinGW compiler miss some of the
    functions. They need to be retrieved dynamically.

    In addition, touch-related functions are available only from Windows onwards.
    These need to resolved dynamically for Q_CC_MSVC as well.

    \sa QWindowsShell32DLL

    \internal
*/

void QWindowsUser32DLL::init()
{
    QSystemLibrary library(QStringLiteral("user32"));
    setProcessDPIAware = (SetProcessDPIAware)library.resolve("SetProcessDPIAware");

    addClipboardFormatListener = (AddClipboardFormatListener)library.resolve("AddClipboardFormatListener");
    removeClipboardFormatListener = (RemoveClipboardFormatListener)library.resolve("RemoveClipboardFormatListener");

    getDisplayAutoRotationPreferences = (GetDisplayAutoRotationPreferences)library.resolve("GetDisplayAutoRotationPreferences");
    setDisplayAutoRotationPreferences = (SetDisplayAutoRotationPreferences)library.resolve("SetDisplayAutoRotationPreferences");

    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8) {
        enableMouseInPointer = (EnableMouseInPointer)library.resolve("EnableMouseInPointer");
        getPointerType = (GetPointerType)library.resolve("GetPointerType");
        getPointerInfo = (GetPointerInfo)library.resolve("GetPointerInfo");
        getPointerDeviceRects = (GetPointerDeviceRects)library.resolve("GetPointerDeviceRects");
        getPointerTouchInfo = (GetPointerTouchInfo)library.resolve("GetPointerTouchInfo");
        getPointerFrameTouchInfo = (GetPointerFrameTouchInfo)library.resolve("GetPointerFrameTouchInfo");
        getPointerFrameTouchInfoHistory = (GetPointerFrameTouchInfoHistory)library.resolve("GetPointerFrameTouchInfoHistory");
        getPointerPenInfo = (GetPointerPenInfo)library.resolve("GetPointerPenInfo");
        getPointerPenInfoHistory = (GetPointerPenInfoHistory)library.resolve("GetPointerPenInfoHistory");
        skipPointerFrameMessages = (SkipPointerFrameMessages)library.resolve("SkipPointerFrameMessages");
    }

    if (QOperatingSystemVersion::current()
        >= QOperatingSystemVersion(QOperatingSystemVersion::Windows, 10, 0, 14393)) {
        adjustWindowRectExForDpi = (AdjustWindowRectExForDpi)library.resolve("AdjustWindowRectExForDpi");
        enableNonClientDpiScaling = (EnableNonClientDpiScaling)library.resolve("EnableNonClientDpiScaling");
        getWindowDpiAwarenessContext = (GetWindowDpiAwarenessContext)library.resolve("GetWindowDpiAwarenessContext");
        getAwarenessFromDpiAwarenessContext = (GetAwarenessFromDpiAwarenessContext)library.resolve("GetAwarenessFromDpiAwarenessContext");
        systemParametersInfoForDpi = (SystemParametersInfoForDpi)library.resolve("SystemParametersInfoForDpi");
    }
}

bool QWindowsUser32DLL::supportsPointerApi()
{
    return enableMouseInPointer && getPointerType && getPointerInfo && getPointerDeviceRects
            && getPointerTouchInfo && getPointerFrameTouchInfo && getPointerFrameTouchInfoHistory
            && getPointerPenInfo && getPointerPenInfoHistory && skipPointerFrameMessages;
}

void QWindowsShcoreDLL::init()
{
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows8_1)
        return;
    QSystemLibrary library(QStringLiteral("SHCore"));
    getProcessDpiAwareness = (GetProcessDpiAwareness)library.resolve("GetProcessDpiAwareness");
    setProcessDpiAwareness = (SetProcessDpiAwareness)library.resolve("SetProcessDpiAwareness");
    getDpiForMonitor = (GetDpiForMonitor)library.resolve("GetDpiForMonitor");
}

QWindowsUser32DLL QWindowsContext::user32dll;
QWindowsShcoreDLL QWindowsContext::shcoredll;

QWindowsContext *QWindowsContext::m_instance = nullptr;

/*!
    \class QWindowsContext
    \brief Singleton container for all relevant information.

    Holds state information formerly stored in \c qapplication_win.cpp.

    \internal
*/

typedef QHash<HWND, QWindowsWindow *> HandleBaseWindowHash;

struct QWindowsContextPrivate {
    QWindowsContextPrivate();

    unsigned m_systemInfo = 0;
    QSet<QString> m_registeredWindowClassNames;
    HandleBaseWindowHash m_windows;
    HDC m_displayContext = nullptr;
    int m_defaultDPI = 96;
    QWindowsKeyMapper m_keyMapper;
    QWindowsMouseHandler m_mouseHandler;
    QWindowsPointerHandler m_pointerHandler;
    QWindowsMimeConverter m_mimeConverter;
    QWindowsScreenManager m_screenManager;
    QSharedPointer<QWindowCreationContext> m_creationContext;
#if QT_CONFIG(tabletevent)
    QScopedPointer<QWindowsTabletSupport> m_tabletSupport;
#endif
    const HRESULT m_oleInitializeResult;
    QWindow *m_lastActiveWindow = nullptr;
    bool m_asyncExpose = false;
    HPOWERNOTIFY m_powerNotification = nullptr;
    HWND m_powerDummyWindow = nullptr;
    static bool m_darkMode;
};

bool QWindowsContextPrivate::m_darkMode = false;

QWindowsContextPrivate::QWindowsContextPrivate()
    : m_oleInitializeResult(OleInitialize(nullptr))
{
    QWindowsContext::user32dll.init();
    QWindowsContext::shcoredll.init();

    if (m_pointerHandler.touchDevice() || m_mouseHandler.touchDevice())
        m_systemInfo |= QWindowsContext::SI_SupportsTouch;
    m_displayContext = GetDC(nullptr);
    m_defaultDPI = GetDeviceCaps(m_displayContext, LOGPIXELSY);
    if (useRTL_Extensions()) {
        m_systemInfo |= QWindowsContext::SI_RTL_Extensions;
        m_keyMapper.setUseRTLExtensions(true);
    }
    m_darkMode = QWindowsTheme::queryDarkMode();
    if (FAILED(m_oleInitializeResult)) {
       qWarning() << "QWindowsContext: OleInitialize() failed: "
           << QWindowsContext::comErrorString(m_oleInitializeResult);
    }
}

QWindowsContext::QWindowsContext() :
    d(new QWindowsContextPrivate)
{
#ifdef Q_CC_MSVC
#    pragma warning( disable : 4996 )
#endif
    m_instance = this;
    // ### FIXME: Remove this once the logging system has other options of configurations.
    const QByteArray bv = qgetenv("QT_QPA_VERBOSE");
    if (!bv.isEmpty())
        QLoggingCategory::setFilterRules(QString::fromLocal8Bit(bv));
}

QWindowsContext::~QWindowsContext()
{
#if QT_CONFIG(tabletevent)
    d->m_tabletSupport.reset(); // Destroy internal window before unregistering classes.
#endif

    if (d->m_powerNotification)
        UnregisterPowerSettingNotification(d->m_powerNotification);

    if (d->m_powerDummyWindow)
        DestroyWindow(d->m_powerDummyWindow);

    unregisterWindowClasses();
    if (d->m_oleInitializeResult == S_OK || d->m_oleInitializeResult == S_FALSE)
        OleUninitialize();

    d->m_screenManager.clearScreens(); // Order: Potentially calls back to the windows.
    if (d->m_displayContext)
        ReleaseDC(nullptr, d->m_displayContext);
    m_instance = nullptr;
}

bool QWindowsContext::initTouch()
{
    return initTouch(QWindowsIntegration::instance()->options());
}

bool QWindowsContext::initTouch(unsigned integrationOptions)
{
    if (d->m_systemInfo & QWindowsContext::SI_SupportsTouch)
        return true;

    QTouchDevice *touchDevice = (d->m_systemInfo & QWindowsContext::SI_SupportsPointer) ?
                d->m_pointerHandler.ensureTouchDevice() : d->m_mouseHandler.ensureTouchDevice();
    if (!touchDevice)
        return false;

    if (!(integrationOptions & QWindowsIntegration::DontPassOsMouseEventsSynthesizedFromTouch))
        touchDevice->setCapabilities(touchDevice->capabilities() | QTouchDevice::MouseEmulation);

    QWindowSystemInterface::registerTouchDevice(touchDevice);

    d->m_systemInfo |= QWindowsContext::SI_SupportsTouch;

    // A touch device was plugged while the app is running. Register all windows for touch.
    if (QGuiApplicationPrivate::is_app_running) {
        for (QWindowsWindow *w : qAsConst(d->m_windows))
            w->registerTouchWindow();
    }

    return true;
}

bool QWindowsContext::initTablet(unsigned integrationOptions)
{
    Q_UNUSED(integrationOptions);
#if QT_CONFIG(tabletevent)
    d->m_tabletSupport.reset(QWindowsTabletSupport::create());
    return true;
#else
    return false;
#endif
}

bool QWindowsContext::initPointer(unsigned integrationOptions)
{
    if (integrationOptions & QWindowsIntegration::DontUseWMPointer)
        return false;

    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows8)
        return false;

    if (!QWindowsContext::user32dll.supportsPointerApi())
        return false;

    d->m_systemInfo |= QWindowsContext::SI_SupportsPointer;
    return true;
}

extern "C" LRESULT QT_WIN_CALLBACK qWindowsPowerWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_POWERBROADCAST || wParam != PBT_POWERSETTINGCHANGE)
        return DefWindowProc(hwnd, message, wParam, lParam);

    static bool initialized = false; // ignore the initial change
    if (!initialized) {
        initialized = true;
        return DefWindowProc(hwnd, message, wParam, lParam);
    }

    auto setting = reinterpret_cast<const POWERBROADCAST_SETTING *>(lParam);
    if (setting) {
        auto data = reinterpret_cast<const DWORD *>(&setting->Data);
        if (*data == 1) {
            // Repaint the windows when returning from sleeping display mode.
            const auto tlw = QGuiApplication::topLevelWindows();
            for (auto w : tlw) {
                if (w->isVisible() && w->windowState() != Qt::WindowMinimized) {
                    if (auto tw = QWindowsWindow::windowsWindowOf(w)) {
                        if (HWND hwnd = tw->handle()) {
                            InvalidateRect(hwnd, nullptr, false);
                        }
                    }
                }
            }
        }
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

bool QWindowsContext::initPowerNotificationHandler()
{
    if (d->m_powerNotification)
        return false;

    d->m_powerDummyWindow = createDummyWindow(QStringLiteral("PowerDummyWindow"), L"QtPowerDummyWindow", qWindowsPowerWindowProc);
    if (!d->m_powerDummyWindow)
        return false;

    d->m_powerNotification = RegisterPowerSettingNotification(d->m_powerDummyWindow, &GUID_MONITOR_POWER_ON, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (!d->m_powerNotification) {
        DestroyWindow(d->m_powerDummyWindow);
        d->m_powerDummyWindow = nullptr;
        return false;
    }
    return true;
}

void QWindowsContext::setTabletAbsoluteRange(int a)
{
#if QT_CONFIG(tabletevent)
    if (!d->m_tabletSupport.isNull())
        d->m_tabletSupport->setAbsoluteRange(a);
#else
    Q_UNUSED(a)
#endif
}

void QWindowsContext::setDetectAltGrModifier(bool a)
{
    d->m_keyMapper.setDetectAltGrModifier(a);
}

int QWindowsContext::processDpiAwareness()
{
    int result;
    if (QWindowsContext::shcoredll.getProcessDpiAwareness
        && SUCCEEDED(QWindowsContext::shcoredll.getProcessDpiAwareness(nullptr, &result))) {
        return result;
    }
    return -1;
}

void QWindowsContext::setProcessDpiAwareness(QtWindows::ProcessDpiAwareness dpiAwareness)
{
    qCDebug(lcQpaWindows) << __FUNCTION__ << dpiAwareness;
    if (QWindowsContext::shcoredll.isValid()) {
        const HRESULT hr = QWindowsContext::shcoredll.setProcessDpiAwareness(dpiAwareness);
        // E_ACCESSDENIED means set externally (MSVC manifest or external app loading Qt plugin).
        // Silence warning in that case unless debug is enabled.
        if (FAILED(hr) && (hr != E_ACCESSDENIED || lcQpaWindows().isDebugEnabled())) {
            qWarning().noquote().nospace() << "SetProcessDpiAwareness("
                << dpiAwareness << ") failed: " << QWindowsContext::comErrorString(hr)
                << ", using " << QWindowsContext::processDpiAwareness();
        }
    } else {
        if (dpiAwareness != QtWindows::ProcessDpiUnaware && QWindowsContext::user32dll.setProcessDPIAware) {
            if (!QWindowsContext::user32dll.setProcessDPIAware())
                qErrnoWarning("SetProcessDPIAware() failed");
        }
    }
}

bool QWindowsContext::isDarkMode()
{
    return QWindowsContextPrivate::m_darkMode;
}

QWindowsContext *QWindowsContext::instance()
{
    return m_instance;
}

unsigned QWindowsContext::systemInfo() const
{
    return d->m_systemInfo;
}

bool QWindowsContext::useRTLExtensions() const
{
    return d->m_keyMapper.useRTLExtensions();
}

QList<int> QWindowsContext::possibleKeys(const QKeyEvent *e) const
{
    return d->m_keyMapper.possibleKeys(e);
}

QSharedPointer<QWindowCreationContext> QWindowsContext::setWindowCreationContext(const QSharedPointer<QWindowCreationContext> &ctx)
{
    const QSharedPointer<QWindowCreationContext> old = d->m_creationContext;
    d->m_creationContext = ctx;
    return old;
}

QSharedPointer<QWindowCreationContext> QWindowsContext::windowCreationContext() const
{
    return d->m_creationContext;
}

int QWindowsContext::defaultDPI() const
{
    return d->m_defaultDPI;
}

HDC QWindowsContext::displayContext() const
{
    return d->m_displayContext;
}

QWindow *QWindowsContext::keyGrabber() const
{
    return d->m_keyMapper.keyGrabber();
}

void QWindowsContext::setKeyGrabber(QWindow *w)
{
    d->m_keyMapper.setKeyGrabber(w);
}

QString QWindowsContext::classNamePrefix()
{
    static QString result;
    if (result.isEmpty()) {
        QTextStream str(&result);
        str << "Qt" << QT_VERSION_MAJOR << QT_VERSION_MINOR << QT_VERSION_PATCH;
        if (QLibraryInfo::isDebugBuild())
            str << 'd';
#ifdef QT_NAMESPACE
#  define xstr(s) str(s)
#  define str(s) #s
        str << xstr(QT_NAMESPACE);
#endif
    }
    return result;
}

// Window class registering code (from qapplication_win.cpp)

QString QWindowsContext::registerWindowClass(const QWindow *w)
{
    Q_ASSERT(w);
    const Qt::WindowFlags flags = w->flags();
    const Qt::WindowFlags type = flags & Qt::WindowType_Mask;
    // Determine style and icon.
    uint style = CS_DBLCLKS;
    bool icon = true;
    // The following will not set CS_OWNDC for any widget window, even if it contains a
    // QOpenGLWidget or QQuickWidget later on. That cannot be detected at this stage.
    if (w->surfaceType() == QSurface::OpenGLSurface || (flags & Qt::MSWindowsOwnDC))
        style |= CS_OWNDC;
    if (!(flags & Qt::NoDropShadowWindowHint)
        && (type == Qt::Popup || w->property("_q_windowsDropShadow").toBool())) {
        style |= CS_DROPSHADOW;
    }
    switch (type) {
    case Qt::Tool:
    case Qt::ToolTip:
    case Qt::Popup:
        style |= CS_SAVEBITS; // Save/restore background
        icon = false;
        break;
    case Qt::Dialog:
        if (!(flags & Qt::WindowSystemMenuHint))
            icon = false; // QTBUG-2027, dialogs without system menu.
        break;
    }
    // Create a unique name for the flag combination
    QString cname = classNamePrefix();
    cname += QLatin1String("QWindow");
    switch (type) {
    case Qt::Tool:
        cname += QLatin1String("Tool");
        break;
    case Qt::ToolTip:
        cname += QLatin1String("ToolTip");
        break;
    case Qt::Popup:
        cname += QLatin1String("Popup");
        break;
    default:
        break;
    }
    if (style & CS_DROPSHADOW)
        cname += QLatin1String("DropShadow");
    if (style & CS_SAVEBITS)
        cname += QLatin1String("SaveBits");
    if (style & CS_OWNDC)
        cname += QLatin1String("OwnDC");
    if (icon)
        cname += QLatin1String("Icon");

    return registerWindowClass(cname, qWindowsWndProc, style, GetSysColorBrush(COLOR_WINDOW), icon);
}

QString QWindowsContext::registerWindowClass(QString cname,
                                             WNDPROC proc,
                                             unsigned style,
                                             HBRUSH brush,
                                             bool icon)
{
    // since multiple Qt versions can be used in one process
    // each one has to have window class names with a unique name
    // The first instance gets the unmodified name; if the class
    // has already been registered by another instance of Qt then
    // add a UUID. The check needs to be performed for each name
    // in case new message windows are added (QTBUG-81347).
    // Note: GetClassInfo() returns != 0 when a class exists.
    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
    WNDCLASS wcinfo;
    const bool classExists = GetClassInfo(appInstance, reinterpret_cast<LPCWSTR>(cname.utf16()), &wcinfo) != FALSE
        && wcinfo.lpfnWndProc != proc;

    if (classExists)
        cname += QUuid::createUuid().toString();

    if (d->m_registeredWindowClassNames.contains(cname))        // already registered in our list
        return cname;

    WNDCLASSEX wc;
    wc.cbSize       = sizeof(WNDCLASSEX);
    wc.style        = style;
    wc.lpfnWndProc  = proc;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = 0;
    wc.hInstance    = appInstance;
    wc.hCursor      = nullptr;
    wc.hbrBackground = brush;
    if (icon) {
        wc.hIcon = static_cast<HICON>(LoadImage(appInstance, L"IDI_ICON1", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
        if (wc.hIcon) {
            int sw = GetSystemMetrics(SM_CXSMICON);
            int sh = GetSystemMetrics(SM_CYSMICON);
            wc.hIconSm = static_cast<HICON>(LoadImage(appInstance, L"IDI_ICON1", IMAGE_ICON, sw, sh, 0));
        } else {
            wc.hIcon = static_cast<HICON>(LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED));
            wc.hIconSm = nullptr;
        }
    } else {
        wc.hIcon    = nullptr;
        wc.hIconSm  = nullptr;
    }

    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = reinterpret_cast<LPCWSTR>(cname.utf16());
    ATOM atom = RegisterClassEx(&wc);
    if (!atom)
        qErrnoWarning("QApplication::regClass: Registering window class '%s' failed.",
                      qPrintable(cname));

    d->m_registeredWindowClassNames.insert(cname);
    qCDebug(lcQpaWindows).nospace() << __FUNCTION__ << ' ' << cname
        << " style=0x" << Qt::hex << style << Qt::dec
        << " brush=" << brush << " icon=" << icon << " atom=" << atom;
    return cname;
}

void QWindowsContext::unregisterWindowClasses()
{
    const auto appInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));

    for (const QString &name : qAsConst(d->m_registeredWindowClassNames)) {
        if (!UnregisterClass(reinterpret_cast<LPCWSTR>(name.utf16()), appInstance) && QWindowsContext::verbose)
            qErrnoWarning("UnregisterClass failed for '%s'", qPrintable(name));
    }
    d->m_registeredWindowClassNames.clear();
}

int QWindowsContext::screenDepth() const
{
    return GetDeviceCaps(d->m_displayContext, BITSPIXEL);
}

QString QWindowsContext::windowsErrorMessage(unsigned long errorCode)
{
    QString rc = QString::fromLatin1("#%1: ").arg(errorCode);
    ushort *lpMsgBuf;

    const DWORD len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, errorCode, 0, reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, nullptr);
    if (len) {
        rc = QString::fromUtf16(lpMsgBuf, int(len));
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc;
}

void QWindowsContext::addWindow(HWND hwnd, QWindowsWindow *w)
{
    d->m_windows.insert(hwnd, w);
}

void QWindowsContext::removeWindow(HWND hwnd)
{
    const HandleBaseWindowHash::iterator it = d->m_windows.find(hwnd);
    if (it != d->m_windows.end()) {
        if (d->m_keyMapper.keyGrabber() == it.value()->window())
            d->m_keyMapper.setKeyGrabber(nullptr);
        d->m_windows.erase(it);
    }
}

QWindowsWindow *QWindowsContext::findPlatformWindow(const QWindowsMenuBar *mb) const
{
    for (auto it = d->m_windows.cbegin(), end = d->m_windows.cend(); it != end; ++it) {
        if ((*it)->menuBar() == mb)
            return *it;
    }
    return nullptr;
}

QWindowsWindow *QWindowsContext::findPlatformWindow(HWND hwnd) const
{
    return d->m_windows.value(hwnd);
}

QWindowsWindow *QWindowsContext::findClosestPlatformWindow(HWND hwnd) const
{
    QWindowsWindow *window = d->m_windows.value(hwnd);

    // Requested hwnd may also be a child of a platform window in case of embedded native windows.
    // Find the closest parent that has a platform window.
    if (!window) {
        for (HWND w = hwnd; w; w = GetParent(w)) {
            window = d->m_windows.value(w);
            if (window)
                break;
        }
    }

    return window;
}

QWindow *QWindowsContext::findWindow(HWND hwnd) const
{
    if (const QWindowsWindow *bw = findPlatformWindow(hwnd))
            return bw->window();
    return nullptr;
}

QWindow *QWindowsContext::windowUnderMouse() const
{
    return (d->m_systemInfo & QWindowsContext::SI_SupportsPointer) ?
        d->m_pointerHandler.windowUnderMouse() : d->m_mouseHandler.windowUnderMouse();
}

void QWindowsContext::clearWindowUnderMouse()
{
    if (d->m_systemInfo & QWindowsContext::SI_SupportsPointer)
        d->m_pointerHandler.clearWindowUnderMouse();
    else
        d->m_mouseHandler.clearWindowUnderMouse();
}

/*!
    \brief Find a child window at a screen point.

    Deep search for a QWindow at global point, skipping non-owned
    windows (accessibility?). Implemented using ChildWindowFromPointEx()
    instead of (historically used) WindowFromPoint() to get a well-defined
    behaviour for hidden/transparent windows.

    \a cwex_flags are flags of ChildWindowFromPointEx().
    \a parent is the parent window, pass GetDesktopWindow() for top levels.
*/

static inline bool findPlatformWindowHelper(const POINT &screenPoint, unsigned cwexFlags,
                                            const QWindowsContext *context,
                                            HWND *hwnd, QWindowsWindow **result)
{
    POINT point = screenPoint;
    screenToClient(*hwnd, &point);
    // Returns parent if inside & none matched.
    const HWND child = ChildWindowFromPointEx(*hwnd, point, cwexFlags);
    if (!child || child == *hwnd)
        return false;
    if (QWindowsWindow *window = context->findPlatformWindow(child)) {
        *result = window;
        *hwnd = child;
        return true;
    }
    // QTBUG-40555: despite CWP_SKIPINVISIBLE, it is possible to hit on invisible
    // full screen windows of other applications that have WS_EX_TRANSPARENT set
    // (for example created by  screen sharing applications). In that case, try to
    // find a Qt window by searching again with CWP_SKIPTRANSPARENT.
    // Note that Qt 5 uses WS_EX_TRANSPARENT for Qt::WindowTransparentForInput
    // as well.
    if (!(cwexFlags & CWP_SKIPTRANSPARENT)
        && (GetWindowLongPtr(child, GWL_EXSTYLE) & WS_EX_TRANSPARENT)) {
        const HWND nonTransparentChild = ChildWindowFromPointEx(*hwnd, point, cwexFlags | CWP_SKIPTRANSPARENT);
        if (QWindowsWindow *nonTransparentWindow = context->findPlatformWindow(nonTransparentChild)) {
            *result = nonTransparentWindow;
            *hwnd = nonTransparentChild;
            return true;
        }
    }
    *hwnd = child;
    return true;
}

QWindowsWindow *QWindowsContext::findPlatformWindowAt(HWND parent,
                                                          const QPoint &screenPointIn,
                                                          unsigned cwex_flags) const
{
    QWindowsWindow *result = nullptr;
    const POINT screenPoint = { screenPointIn.x(), screenPointIn.y() };
    while (findPlatformWindowHelper(screenPoint, cwex_flags, this, &parent, &result)) {}
    // QTBUG-40815: ChildWindowFromPointEx() can hit on special windows from
    // screen recorder applications like ScreenToGif. Fall back to WindowFromPoint().
    if (result == nullptr) {
        if (const HWND window = WindowFromPoint(screenPoint))
            result = findPlatformWindow(window);
    }
    return result;
}

bool QWindowsContext::isSessionLocked()
{
    bool result = false;
    const DWORD sessionId = WTSGetActiveConsoleSessionId();
    if (sessionId != 0xFFFFFFFF) {
        LPTSTR buffer = nullptr;
        DWORD size = 0;
#if !defined(Q_CC_MINGW)
        if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionId,
                                       WTSSessionInfoEx, &buffer, &size) == TRUE
            && size > 0) {
            const WTSINFOEXW *info = reinterpret_cast<WTSINFOEXW *>(buffer);
            result = info->Level == 1 && info->Data.WTSInfoExLevel1.SessionFlags == WTS_SESSIONSTATE_LOCK;
            WTSFreeMemory(buffer);
        }
#else   // MinGW as of 7.3 does not have WTSINFOEXW in wtsapi32.h
        // Retrieve the flags which are at offset 16 due to padding for 32/64bit alike.
        if (WTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, sessionId,
                                       WTS_INFO_CLASS(25), &buffer, &size) == TRUE
            && size >= 20) {
            const DWORD *p = reinterpret_cast<DWORD *>(buffer);
            const DWORD level = *p;
            const DWORD sessionFlags = *(p + 4);
            result = level == 1 && sessionFlags == 1;
            WTSFreeMemory(buffer);
        }
#endif // Q_CC_MINGW
    }
    return result;
}

QWindowsMimeConverter &QWindowsContext::mimeConverter() const
{
    return d->m_mimeConverter;
}

QWindowsScreenManager &QWindowsContext::screenManager()
{
    return d->m_screenManager;
}

QWindowsTabletSupport *QWindowsContext::tabletSupport() const
{
#if QT_CONFIG(tabletevent)
    return d->m_tabletSupport.data();
#else
    return 0;
#endif
}

/*!
    \brief Convenience to create a non-visible, message-only dummy
    window for example used as clipboard watcher or for GL.
*/

HWND QWindowsContext::createDummyWindow(const QString &classNameIn,
                                        const wchar_t *windowName,
                                        WNDPROC wndProc, DWORD style)
{
    if (!wndProc)
        wndProc = DefWindowProc;
    QString className = registerWindowClass(classNamePrefix() + classNameIn, wndProc);
    return CreateWindowEx(0, reinterpret_cast<LPCWSTR>(className.utf16()),
                          windowName, style,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          HWND_MESSAGE, nullptr, static_cast<HINSTANCE>(GetModuleHandle(nullptr)), nullptr);
}

// Re-engineered from the inline function _com_error::ErrorMessage().
// We cannot use it directly since it uses swprintf_s(), which is not
// present in the MSVCRT.DLL found on Windows XP (QTBUG-35617).
static inline QString errorMessageFromComError(const _com_error &comError)
{
     TCHAR *message = nullptr;
     FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, DWORD(comError.Error()), MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
                   message, 0, nullptr);
     if (message) {
         const QString result = QString::fromWCharArray(message).trimmed();
         LocalFree(static_cast<HLOCAL>(message));
         return result;
     }
     if (const WORD wCode = comError.WCode())
         return QString::asprintf("IDispatch error #%u", uint(wCode));
     return QString::asprintf("Unknown error 0x0%x", uint(comError.Error()));
}

/*!
    \brief Common COM error strings.
*/

QByteArray QWindowsContext::comErrorString(HRESULT hr)
{
    QByteArray result = QByteArrayLiteral("COM error 0x")
        + QByteArray::number(quintptr(hr), 16) + ' ';
    switch (hr) {
    case S_OK:
        result += QByteArrayLiteral("S_OK");
        break;
    case S_FALSE:
        result += QByteArrayLiteral("S_FALSE");
        break;
    case E_UNEXPECTED:
        result += QByteArrayLiteral("E_UNEXPECTED");
        break;
    case E_ACCESSDENIED:
        result += QByteArrayLiteral("E_ACCESSDENIED");
        break;
    case CO_E_ALREADYINITIALIZED:
        result += QByteArrayLiteral("CO_E_ALREADYINITIALIZED");
        break;
    case CO_E_NOTINITIALIZED:
        result += QByteArrayLiteral("CO_E_NOTINITIALIZED");
        break;
    case RPC_E_CHANGED_MODE:
        result += QByteArrayLiteral("RPC_E_CHANGED_MODE");
        break;
    case OLE_E_WRONGCOMPOBJ:
        result += QByteArrayLiteral("OLE_E_WRONGCOMPOBJ");
        break;
    case CO_E_NOT_SUPPORTED:
        result += QByteArrayLiteral("CO_E_NOT_SUPPORTED");
        break;
    case E_NOTIMPL:
        result += QByteArrayLiteral("E_NOTIMPL");
        break;
    case E_INVALIDARG:
        result += QByteArrayLiteral("E_INVALIDARG");
        break;
    case E_NOINTERFACE:
        result += QByteArrayLiteral("E_NOINTERFACE");
        break;
    case E_POINTER:
        result += QByteArrayLiteral("E_POINTER");
        break;
    case E_HANDLE:
        result += QByteArrayLiteral("E_HANDLE");
        break;
    case E_ABORT:
        result += QByteArrayLiteral("E_ABORT");
        break;
    case E_FAIL:
        result += QByteArrayLiteral("E_FAIL");
        break;
    case RPC_E_WRONG_THREAD:
        result += QByteArrayLiteral("RPC_E_WRONG_THREAD");
        break;
    case RPC_E_THREAD_NOT_INIT:
        result += QByteArrayLiteral("RPC_E_THREAD_NOT_INIT");
        break;
    default:
        break;
    }
    _com_error error(hr);
    result += QByteArrayLiteral(" (");
    result += errorMessageFromComError(error);
    result += ')';
    return result;
}

void QWindowsContext::forceNcCalcSize(HWND hwnd)
{
    // Force WM_NCCALCSIZE to adjust margin
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

bool QWindowsContext::systemParametersInfo(unsigned action, unsigned param, void *out,
                                           unsigned dpi)
{
    const BOOL result = QWindowsContext::user32dll.systemParametersInfoForDpi != nullptr && dpi != 0
        ? QWindowsContext::user32dll.systemParametersInfoForDpi(action, param, out, 0, dpi)
        : SystemParametersInfo(action, param, out, 0);
    return result == TRUE;
}

bool QWindowsContext::systemParametersInfoForScreen(unsigned action, unsigned param, void *out,
                                                    const QPlatformScreen *screen)
{
    return systemParametersInfo(action, param, out, screen ? unsigned(screen->logicalDpi().first) : 0u);
}

bool QWindowsContext::systemParametersInfoForWindow(unsigned action, unsigned param, void *out,
                                                    const QPlatformWindow *win)
{
    return systemParametersInfoForScreen(action, param, out, win ? win->screen() : nullptr);
}

bool QWindowsContext::nonClientMetrics(NONCLIENTMETRICS *ncm, unsigned dpi)
{
    memset(ncm, 0, sizeof(NONCLIENTMETRICS));
    ncm->cbSize = sizeof(NONCLIENTMETRICS);
    return systemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm->cbSize, ncm, dpi);
}

bool QWindowsContext::nonClientMetricsForScreen(NONCLIENTMETRICS *ncm,
                                                const QPlatformScreen *screen)
{
    const int dpi = screen ? qRound(screen->logicalDpi().first) : 0;
    return nonClientMetrics(ncm, unsigned(dpi));
}

bool QWindowsContext::nonClientMetricsForWindow(NONCLIENTMETRICS *ncm, const QPlatformWindow *win)
{
    return nonClientMetricsForScreen(ncm, win ? win->screen() : nullptr);
}

static inline QWindowsInputContext *windowsInputContext()
{
    return qobject_cast<QWindowsInputContext *>(QWindowsIntegration::instance()->inputContext());
}


// Child windows, fixed-size windows or pop-ups and similar should not be resized
static inline bool resizeOnDpiChanged(const QWindow *w)
{
    bool result = false;
    if (w->isTopLevel()) {
        switch (w->type()) {
        case Qt::Window:
        case Qt::Dialog:
        case Qt::Sheet:
        case Qt::Drawer:
        case Qt::Tool:
            result = !w->flags().testFlag(Qt::MSWindowsFixedSizeDialogHint);
            break;
        default:
            break;
        }
    }
    return result;
}

bool QWindowsContext::shouldHaveNonClientDpiScaling(const QWindow *window)
{
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows10
        && window->isTopLevel()
        && !window->property(QWindowsWindow::embeddedNativeParentHandleProperty).isValid()
#if QT_CONFIG(opengl) // /QTBUG-62901, EnableNonClientDpiScaling has problems with GL
        && (window->surfaceType() != QSurface::OpenGLSurface
            || QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL)
#endif
       ;
}

static inline bool isInputMessage(UINT m)
{
    switch (m) {
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_INPUT:
    case WM_TOUCH:
    case WM_MOUSEHOVER:
    case WM_MOUSELEAVE:
    case WM_NCMOUSEHOVER:
    case WM_NCMOUSELEAVE:
    case WM_SIZING:
    case WM_MOVING:
    case WM_SYSCOMMAND:
    case WM_COMMAND:
    case WM_DWMNCRENDERINGCHANGED:
    case WM_PAINT:
        return true;
    default:
        break;
    }
    return (m >= WM_MOUSEFIRST && m <= WM_MOUSELAST)
        || (m >= WM_NCMOUSEMOVE && m <= WM_NCXBUTTONDBLCLK)
        || (m >= WM_KEYFIRST && m <= WM_KEYLAST);
}

/*!
     \brief Main windows procedure registered for windows.

     \sa QWindowsGuiEventDispatcher
*/

bool QWindowsContext::windowsProc(HWND hwnd, UINT message,
                                  QtWindows::WindowsEventType et,
                                  WPARAM wParam, LPARAM lParam,
                                  LRESULT *result,
                                  QWindowsWindow **platformWindowPtr)
{
    *result = 0;

    MSG msg;
    msg.hwnd = hwnd;         // re-create MSG structure
    msg.message = message;   // time and pt fields ignored
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.pt.x = msg.pt.y = 0;
    if (et != QtWindows::CursorEvent && (et & (QtWindows::MouseEventFlag | QtWindows::NonClientEventFlag))) {
        msg.pt.x = GET_X_LPARAM(lParam);
        msg.pt.y = GET_Y_LPARAM(lParam);
        // For non-client-area messages, these are screen coordinates (as expected
        // in the MSG structure), otherwise they are client coordinates.
        if (!(et & QtWindows::NonClientEventFlag)) {
            clientToScreen(msg.hwnd, &msg.pt);
        }
    } else {
        GetCursorPos(&msg.pt);
    }

    QWindowsWindow *platformWindow = findPlatformWindow(hwnd);
    *platformWindowPtr = platformWindow;

    // Run the native event filters. QTBUG-67095: Exclude input messages which are sent
    // by QEventDispatcherWin32::processEvents()
    if (!isInputMessage(msg.message) && filterNativeEvent(&msg, result))
        return true;

    if (platformWindow && filterNativeEvent(platformWindow->window(), &msg, result))
        return true;

    if (et & QtWindows::InputMethodEventFlag) {
        QWindowsInputContext *windowsInputContext = ::windowsInputContext();
        // Disable IME assuming this is a special implementation hooking into keyboard input.
        // "Real" IME implementations should use a native event filter intercepting IME events.
        if (!windowsInputContext) {
            QWindowsInputContext::setWindowsImeEnabled(platformWindow, false);
            return false;
        }
        switch (et) {
        case QtWindows::InputMethodStartCompositionEvent:
            return windowsInputContext->startComposition(hwnd);
        case QtWindows::InputMethodCompositionEvent:
            return windowsInputContext->composition(hwnd, lParam);
        case QtWindows::InputMethodEndCompositionEvent:
            return windowsInputContext->endComposition(hwnd);
        case QtWindows::InputMethodRequest:
            return windowsInputContext->handleIME_Request(wParam, lParam, result);
        default:
            break;
        }
    } // InputMethodEventFlag

    switch (et) {
    case QtWindows::GestureEvent:
        if (!(d->m_systemInfo & QWindowsContext::SI_SupportsPointer))
            return sessionManagerInteractionBlocked() || d->m_mouseHandler.translateGestureEvent(platformWindow->window(), hwnd, et, msg, result);
        break;
    case QtWindows::InputMethodOpenCandidateWindowEvent:
    case QtWindows::InputMethodCloseCandidateWindowEvent:
        // TODO: Release/regrab mouse if a popup has mouse grab.
        return false;
    case QtWindows::DestroyEvent:
        if (platformWindow && !platformWindow->testFlag(QWindowsWindow::WithinDestroy)) {
            qWarning() << "External WM_DESTROY received for " << platformWindow->window()
                       << ", parent: " << platformWindow->window()->parent()
                       << ", transient parent: " << platformWindow->window()->transientParent();
            }
        return false;
    case QtWindows::ClipboardEvent:
        return false;
    case QtWindows::CursorEvent: // Sent to windows that do not have capture (see QTBUG-58590).
        if (QWindowsCursor::hasOverrideCursor()) {
            QWindowsCursor::enforceOverrideCursor();
            return true;
        }
        break;
    case QtWindows::UnknownEvent:
        return false;
    case QtWindows::AccessibleObjectFromWindowRequest:
#if QT_CONFIG(accessibility)
        return QWindowsUiaAccessibility::handleWmGetObject(hwnd, wParam, lParam, result);
#else
        return false;
#endif
    case QtWindows::DisplayChangedEvent:
        if (QWindowsTheme *t = QWindowsTheme::instance())
            t->displayChanged();
        QWindowsWindow::displayChanged();
        return d->m_screenManager.handleDisplayChange(wParam, lParam);
    case QtWindows::SettingChangedEvent: {
        QWindowsWindow::settingsChanged();
        const bool darkMode = QWindowsTheme::queryDarkMode();
        if (darkMode != QWindowsContextPrivate::m_darkMode) {
            QWindowsContextPrivate::m_darkMode = darkMode;
            auto nativeInterface =
                static_cast<QWindowsNativeInterface *>(QWindowsIntegration::instance()->nativeInterface());
            emit nativeInterface->darkModeChanged(darkMode);
            const auto options = QWindowsIntegration::instance()->options();
            if ((options & QWindowsIntegration::DarkModeWindowFrames) != 0) {
                for (QWindowsWindow *w : d->m_windows)
                    w->setDarkBorder(QWindowsContextPrivate::m_darkMode);
            }
            if ((options & QWindowsIntegration::DarkModeStyle) != 0) {
                QWindowsTheme::instance()->refresh();
                for (QWindowsWindow *w : d->m_windows)
                    QWindowSystemInterface::handleThemeChange(w->window());
            }
        }
        return d->m_screenManager.handleScreenChanges();
    }
    default:
        break;
    }

    // Before CreateWindowEx() returns, some events are sent,
    // for example WM_GETMINMAXINFO asking for size constraints for top levels.
    // Pass on to current creation context
    if (!platformWindow && !d->m_creationContext.isNull()) {
        switch (et) {
        case QtWindows::QuerySizeHints:
            d->m_creationContext->applyToMinMaxInfo(reinterpret_cast<MINMAXINFO *>(lParam));
            return true;
        case QtWindows::ResizeEvent:
            d->m_creationContext->obtainedSize = QSize(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return true;
        case QtWindows::MoveEvent:
            d->m_creationContext->obtainedPos = QPoint(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return true;
        case QtWindows::NonClientCreate:
            if (shouldHaveNonClientDpiScaling(d->m_creationContext->window))
                enableNonClientDpiScaling(msg.hwnd);
            return false;
        case QtWindows::CalculateSize:
            return QWindowsGeometryHint::handleCalculateSize(d->m_creationContext->customMargins, msg, result);
        case QtWindows::GeometryChangingEvent:
            return QWindowsWindow::handleGeometryChangingMessage(&msg, d->m_creationContext->window,
                                                                 d->m_creationContext->margins + d->m_creationContext->customMargins);
        default:
            break;
        }
    }
    if (platformWindow) {
        // Suppress events sent during DestroyWindow() for native children.
        if (platformWindow->testFlag(QWindowsWindow::WithinDestroy))
            return false;
        if (QWindowsContext::verbose > 1)
            qCDebug(lcQpaEvents) << "Event window: " << platformWindow->window();
    } else {
        qWarning("%s: No Qt Window found for event 0x%x (%s), hwnd=0x%p.",
                 __FUNCTION__, message,
                 QWindowsGuiEventDispatcher::windowsMessageName(message), hwnd);
        return false;
    }

    switch (et) {
    case QtWindows::DeviceChangeEvent:
        if (d->m_systemInfo & QWindowsContext::SI_SupportsTouch)
            break;
        // See if there are any touch devices added
        if (wParam == DBT_DEVNODES_CHANGED)
            initTouch();
        break;
    case QtWindows::KeyboardLayoutChangeEvent:
        if (QWindowsInputContext *wic = windowsInputContext())
            wic->handleInputLanguageChanged(wParam, lParam);
        Q_FALLTHROUGH();
    case QtWindows::KeyDownEvent:
    case QtWindows::KeyEvent:
    case QtWindows::InputMethodKeyEvent:
    case QtWindows::InputMethodKeyDownEvent:
    case QtWindows::AppCommandEvent:
        return sessionManagerInteractionBlocked() ||  d->m_keyMapper.translateKeyEvent(platformWindow->window(), hwnd, msg, result);
    case QtWindows::MenuAboutToShowEvent:
        if (sessionManagerInteractionBlocked())
            return true;
        if (QWindowsPopupMenu::notifyAboutToShow(reinterpret_cast<HMENU>(wParam)))
            return true;
        if (platformWindow == nullptr || platformWindow->menuBar() == nullptr)
            return false;
        return platformWindow->menuBar()->notifyAboutToShow(reinterpret_cast<HMENU>(wParam));
    case QtWindows::MenuCommandEvent:
        if (sessionManagerInteractionBlocked())
            return true;
        if (QWindowsPopupMenu::notifyTriggered(LOWORD(wParam)))
            return true;
        if (platformWindow == nullptr || platformWindow->menuBar() == nullptr)
            return false;
        return platformWindow->menuBar()->notifyTriggered(LOWORD(wParam));
    case QtWindows::MoveEvent:
        platformWindow->handleMoved();
        return true;
    case QtWindows::ResizeEvent:
        platformWindow->handleResized(static_cast<int>(wParam));
        return true;
    case QtWindows::QuerySizeHints:
        platformWindow->getSizeHints(reinterpret_cast<MINMAXINFO *>(lParam));
        return true;// maybe available on some SDKs revisit WM_NCCALCSIZE
    case QtWindows::CalculateSize:
        return QWindowsGeometryHint::handleCalculateSize(platformWindow->customMargins(), msg, result);
    case QtWindows::NonClientHitTest:
        return platformWindow->handleNonClientHitTest(QPoint(msg.pt.x, msg.pt.y), result);
    case QtWindows::GeometryChangingEvent:
        return platformWindow->QWindowsWindow::handleGeometryChanging(&msg);
    case QtWindows::ExposeEvent:
        return platformWindow->handleWmPaint(hwnd, message, wParam, lParam);
    case QtWindows::NonClientMouseEvent:
        if ((d->m_systemInfo & QWindowsContext::SI_SupportsPointer) && platformWindow->frameStrutEventsEnabled())
            return sessionManagerInteractionBlocked() || d->m_pointerHandler.translateMouseEvent(platformWindow->window(), hwnd, et, msg, result);
        else
            return sessionManagerInteractionBlocked() || d->m_mouseHandler.translateMouseEvent(platformWindow->window(), hwnd, et, msg, result);
    case QtWindows::NonClientPointerEvent:
        if ((d->m_systemInfo & QWindowsContext::SI_SupportsPointer) && platformWindow->frameStrutEventsEnabled())
            return sessionManagerInteractionBlocked() || d->m_pointerHandler.translatePointerEvent(platformWindow->window(), hwnd, et, msg, result);
        break;
    case QtWindows::EnterSizeMoveEvent:
        platformWindow->setFlag(QWindowsWindow::ResizeMoveActive);
        return true;
    case QtWindows::ExitSizeMoveEvent:
        platformWindow->clearFlag(QWindowsWindow::ResizeMoveActive);
        platformWindow->checkForScreenChanged();
        handleExitSizeMove(platformWindow->window());
        return true;
    case QtWindows::ScrollEvent:
        if (!(d->m_systemInfo & QWindowsContext::SI_SupportsPointer))
            return sessionManagerInteractionBlocked() || d->m_mouseHandler.translateScrollEvent(platformWindow->window(), hwnd, msg, result);
        break;
    case QtWindows::MouseWheelEvent:
    case QtWindows::MouseEvent:
    case QtWindows::LeaveEvent:
        {
            QWindow *window = platformWindow->window();
            while (window && (window->flags() & Qt::WindowTransparentForInput))
                window = window->parent();
            if (!window)
                return false;
            if (d->m_systemInfo & QWindowsContext::SI_SupportsPointer)
                return sessionManagerInteractionBlocked() || d->m_pointerHandler.translateMouseEvent(window, hwnd, et, msg, result);
            else
                return sessionManagerInteractionBlocked() || d->m_mouseHandler.translateMouseEvent(window, hwnd, et, msg, result);
        }
        break;
    case QtWindows::TouchEvent:
        if (!(d->m_systemInfo & QWindowsContext::SI_SupportsPointer))
            return sessionManagerInteractionBlocked() || d->m_mouseHandler.translateTouchEvent(platformWindow->window(), hwnd, et, msg, result);
        break;
    case QtWindows::PointerEvent:
        if (d->m_systemInfo & QWindowsContext::SI_SupportsPointer)
            return sessionManagerInteractionBlocked() || d->m_pointerHandler.translatePointerEvent(platformWindow->window(), hwnd, et, msg, result);
        break;
    case QtWindows::FocusInEvent: // see QWindowsWindow::requestActivateWindow().
    case QtWindows::FocusOutEvent:
        handleFocusEvent(et, platformWindow);
        return true;
    case QtWindows::ShowEventOnParentRestoring: // QTBUG-40696, prevent Windows from re-showing hidden transient children (dialogs).
        if (!platformWindow->window()->isVisible()) {
            *result = 0;
            return true;
        }
        break;
    case QtWindows::HideEvent:
        platformWindow->handleHidden();
        return false;// Indicate transient children should be hidden by windows (SW_PARENTCLOSING)
    case QtWindows::CloseEvent:
        QWindowSystemInterface::handleCloseEvent(platformWindow->window());
        return true;
    case QtWindows::ThemeChanged: {
        // Switch from Aero to Classic changes margins.
        if (QWindowsTheme *theme = QWindowsTheme::instance())
            theme->windowsThemeChanged(platformWindow->window());
        return true;
    }
    case QtWindows::CompositionSettingsChanged:
        platformWindow->handleCompositionSettingsChanged();
        return true;
    case QtWindows::ActivateWindowEvent:
        if (platformWindow->window()->flags() & Qt::WindowDoesNotAcceptFocus) {
            *result = LRESULT(MA_NOACTIVATE);
            return true;
        }
#if QT_CONFIG(tabletevent)
        if (!d->m_tabletSupport.isNull())
            d->m_tabletSupport->notifyActivate();
#endif // QT_CONFIG(tabletevent)
        if (platformWindow->testFlag(QWindowsWindow::BlockedByModal))
            if (const QWindow *modalWindow = QGuiApplication::modalWindow()) {
                QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(modalWindow);
                Q_ASSERT(platformWindow);
                platformWindow->alertWindow();
            }
        break;
    case QtWindows::MouseActivateWindowEvent:
    case QtWindows::PointerActivateWindowEvent:
        if (platformWindow->window()->flags() & Qt::WindowDoesNotAcceptFocus) {
            *result = LRESULT(MA_NOACTIVATE);
            return true;
        }
        break;
#ifndef QT_NO_CONTEXTMENU
    case QtWindows::ContextMenu:
        return handleContextMenuEvent(platformWindow->window(), msg);
#endif
    case QtWindows::WhatsThisEvent: {
#ifndef QT_NO_WHATSTHIS
        QWindowSystemInterface::handleEnterWhatsThisEvent();
        return true;
#endif
    }   break;
    case QtWindows::DpiChangedEvent: {
        // Try to apply the suggested size first and then notify ScreenChanged
        // so that the resize event sent from QGuiApplication incorporates it
        // WM_DPICHANGED is sent with a size that avoids resize loops (by
        // snapping back to the previous screen, see QTBUG-65580).
        const bool doResize = resizeOnDpiChanged(platformWindow->window());
        if (doResize) {
            platformWindow->setFlag(QWindowsWindow::WithinDpiChanged);
            platformWindow->updateFullFrameMargins();
            const auto prcNewWindow = reinterpret_cast<RECT *>(lParam);
            qCDebug(lcQpaWindows) << __FUNCTION__ << "WM_DPICHANGED"
                << platformWindow->window() << *prcNewWindow;
            SetWindowPos(hwnd, nullptr, prcNewWindow->left, prcNewWindow->top,
                         prcNewWindow->right - prcNewWindow->left,
                         prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
            platformWindow->clearFlag(QWindowsWindow::WithinDpiChanged);
        }
        platformWindow->checkForScreenChanged(QWindowsWindow::FromDpiChange);
        return doResize;
    }
#if QT_CONFIG(sessionmanager)
    case QtWindows::QueryEndSessionApplicationEvent: {
        QWindowsSessionManager *sessionManager = platformSessionManager();
        if (sessionManager->isActive()) { // bogus message from windows
            *result = sessionManager->wasCanceled() ? 0 : 1;
            return true;
        }

        sessionManager->setActive(true);
        sessionManager->blocksInteraction();
        sessionManager->clearCancellation();

        auto *qGuiAppPriv = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(qApp));
        qGuiAppPriv->commitData();

        if (lParam & ENDSESSION_LOGOFF)
            fflush(nullptr);

        *result = sessionManager->wasCanceled() ? 0 : 1;
        return true;
    }
    case QtWindows::EndSessionApplicationEvent: {
        QWindowsSessionManager *sessionManager = platformSessionManager();

        sessionManager->setActive(false);
        sessionManager->allowsInteraction();
        const bool endsession = wParam != 0;

        // we receive the message for each toplevel window included internal hidden ones,
        // but the aboutToQuit signal should be emitted only once.
        auto *qGuiAppPriv = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(qApp));
        if (endsession && !qGuiAppPriv->aboutToQuitEmitted) {
            qGuiAppPriv->aboutToQuitEmitted = true;
            int index = QGuiApplication::staticMetaObject.indexOfSignal("aboutToQuit()");
            qApp->qt_metacall(QMetaObject::InvokeMetaMethod, index, nullptr);
            // since the process will be killed immediately quit() has no real effect
            QGuiApplication::quit();
        }
        return true;
    }
#endif // !defined(QT_NO_SESSIONMANAGER)
    default:
        break;
    }
    return false;
}

/* Compress activation events. If the next focus window is already known
 * at the time the current one receives focus-out, pass that to
 * QWindowSystemInterface instead of sending 0 and ignore its consecutive
 * focus-in event.
 * This helps applications that do handling in focus-out events. */
void QWindowsContext::handleFocusEvent(QtWindows::WindowsEventType et,
                                       QWindowsWindow *platformWindow)
{
    QWindow *nextActiveWindow = nullptr;
    if (et == QtWindows::FocusInEvent) {
        QWindow *topWindow = QWindowsWindow::topLevelOf(platformWindow->window());
        QWindow *modalWindow = nullptr;
        if (QGuiApplicationPrivate::instance()->isWindowBlocked(topWindow, &modalWindow) && topWindow != modalWindow) {
            modalWindow->requestActivate();
            return;
        }
        // QTBUG-32867: Invoking WinAPI SetParent() can cause focus-in for the
        // window which is not desired for native child widgets.
        if (platformWindow->testFlag(QWindowsWindow::WithinSetParent)) {
            QWindow *currentFocusWindow = QGuiApplication::focusWindow();
            if (currentFocusWindow && currentFocusWindow != platformWindow->window()) {
                currentFocusWindow->requestActivate();
                return;
            }
        }
        nextActiveWindow = platformWindow->window();
    } else {
        // Focus out: Is the next window known and different
        // from the receiving the focus out.
        if (const HWND nextActiveHwnd = GetFocus())
            if (QWindowsWindow *nextActivePlatformWindow = findClosestPlatformWindow(nextActiveHwnd))
                if (nextActivePlatformWindow != platformWindow)
                    nextActiveWindow = nextActivePlatformWindow->window();
    }
    if (nextActiveWindow != d->m_lastActiveWindow) {
         d->m_lastActiveWindow = nextActiveWindow;
         QWindowSystemInterface::handleWindowActivated(nextActiveWindow);
    }
}

#ifndef QT_NO_CONTEXTMENU
bool QWindowsContext::handleContextMenuEvent(QWindow *window, const MSG &msg)
{
    bool mouseTriggered = false;
    QPoint globalPos;
    QPoint pos;
    if (msg.lParam != int(0xffffffff)) {
        mouseTriggered = true;
        globalPos.setX(msg.pt.x);
        globalPos.setY(msg.pt.y);
        pos = QWindowsGeometryHint::mapFromGlobal(msg.hwnd, globalPos);

        RECT clientRect;
        if (GetClientRect(msg.hwnd, &clientRect)) {
            if (pos.x() < clientRect.left || pos.x() >= clientRect.right ||
                pos.y() < clientRect.top || pos.y() >= clientRect.bottom)
            {
                // This is the case that user has right clicked in the window's caption,
                // We should call DefWindowProc() to display a default shortcut menu
                // instead of sending a Qt window system event.
                return false;
            }
        }
    }

    QWindowSystemInterface::handleContextMenuEvent(window, mouseTriggered, pos, globalPos,
                                                   QWindowsKeyMapper::queryKeyboardModifiers());
    return true;
}
#endif

void QWindowsContext::handleExitSizeMove(QWindow *window)
{
    // Windows can be moved/resized by:
    // 1) User moving a window by dragging the title bar: Causes a sequence
    //    of WM_NCLBUTTONDOWN, WM_NCMOUSEMOVE but no WM_NCLBUTTONUP,
    //    leaving the left mouse button 'pressed'
    // 2) User choosing Resize/Move from System menu and using mouse/cursor keys:
    //    No mouse events are received
    // 3) Programmatically via QSizeGrip calling QPlatformWindow::startSystemResize/Move():
    //    Mouse is left in pressed state after press on size grip (inside window),
    //    no further mouse events are received
    // For cases 1,3, intercept WM_EXITSIZEMOVE to sync the buttons.
    const Qt::MouseButtons currentButtons = QWindowsMouseHandler::queryMouseButtons();
    const Qt::MouseButtons appButtons = QGuiApplication::mouseButtons();
    if (currentButtons == appButtons)
        return;
    const Qt::KeyboardModifiers keyboardModifiers = QWindowsKeyMapper::queryKeyboardModifiers();
    const QPoint globalPos = QWindowsCursor::mousePosition();
    const QPlatformWindow *platWin = window->handle();
    const QPoint localPos = platWin->mapFromGlobal(globalPos);
    const QEvent::Type type = platWin->geometry().contains(globalPos)
        ? QEvent::MouseButtonRelease : QEvent::NonClientAreaMouseButtonRelease;
    for (Qt::MouseButton button : {Qt::LeftButton, Qt::RightButton, Qt::MiddleButton}) {
        if (appButtons.testFlag(button) && !currentButtons.testFlag(button)) {
            QWindowSystemInterface::handleMouseEvent(window, localPos, globalPos,
                                                     currentButtons, button, type,
                                                     keyboardModifiers);
        }
    }
    if (d->m_systemInfo & QWindowsContext::SI_SupportsPointer)
        d->m_pointerHandler.clearEvents();
    else
        d->m_mouseHandler.clearEvents();
}

bool QWindowsContext::asyncExpose() const
{
    return d->m_asyncExpose;
}

void QWindowsContext::setAsyncExpose(bool value)
{
    d->m_asyncExpose = value;
}

QTouchDevice *QWindowsContext::touchDevice() const
{
    return (d->m_systemInfo & QWindowsContext::SI_SupportsPointer) ?
        d->m_pointerHandler.touchDevice() : d->m_mouseHandler.touchDevice();
}

DWORD QWindowsContext::readAdvancedExplorerSettings(const wchar_t *subKey, DWORD defaultValue)
{
    const auto value =
        QWinRegistryKey(HKEY_CURRENT_USER,
                        LR"(Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced)")
                       .dwordValue(subKey);
    return value.second ? value.first : defaultValue;
}

static inline bool isEmptyRect(const RECT &rect)
{
    return rect.right - rect.left == 0 && rect.bottom - rect.top == 0;
}

static inline QMargins marginsFromRects(const RECT &frame, const RECT &client)
{
    return QMargins(client.left - frame.left, client.top - frame.top,
                    frame.right - client.right, frame.bottom - client.bottom);
}

static RECT rectFromNcCalcSize(UINT message, WPARAM wParam, LPARAM lParam, int n)
{
    RECT result = {0, 0, 0, 0};
    if (message == WM_NCCALCSIZE && wParam)
        result = reinterpret_cast<const NCCALCSIZE_PARAMS *>(lParam)->rgrc[n];
    return result;
}

static inline bool isMinimized(HWND hwnd)
{
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    return GetWindowPlacement(hwnd, &windowPlacement) && windowPlacement.showCmd == SW_SHOWMINIMIZED;
}

static inline bool isTopLevel(HWND hwnd)
{
    return (GetWindowLongPtr(hwnd, GWL_STYLE) & WS_CHILD) == 0;
}

/*!
    \brief Windows functions for actual windows.

    There is another one for timers, sockets, etc in
    QEventDispatcherWin32.

*/

extern "C" LRESULT QT_WIN_CALLBACK qWindowsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result;
    const QtWindows::WindowsEventType et = windowsEventType(message, wParam, lParam);
    QWindowsWindow *platformWindow = nullptr;
    const RECT ncCalcSizeFrame = rectFromNcCalcSize(message, wParam, lParam, 0);
    const bool handled = QWindowsContext::instance()->windowsProc(hwnd, message, et, wParam, lParam, &result, &platformWindow);
    if (QWindowsContext::verbose > 1 && lcQpaEvents().isDebugEnabled()) {
        if (const char *eventName = QWindowsGuiEventDispatcher::windowsMessageName(message)) {
            qCDebug(lcQpaEvents).nospace() << "EVENT: hwd=" << hwnd << ' ' << eventName
                << " msg=0x" << Qt::hex << message << " et=0x" << et << Qt::dec << " wp="
                << int(wParam) << " at " << GET_X_LPARAM(lParam) << ','
                << GET_Y_LPARAM(lParam) << " handled=" << handled;
        }
    }
    if (!handled)
        result = DefWindowProc(hwnd, message, wParam, lParam);

    // Capture WM_NCCALCSIZE on top level windows and obtain the window margins by
    // subtracting the rectangles before and after processing. This will correctly
    // capture client code overriding the message and allow for per-monitor margins
    // for High DPI (QTBUG-53255, QTBUG-40578).
    if (message == WM_NCCALCSIZE && !isEmptyRect(ncCalcSizeFrame) && isTopLevel(hwnd) && !isMinimized(hwnd)) {
        const QMargins margins =
            marginsFromRects(ncCalcSizeFrame, rectFromNcCalcSize(message, wParam, lParam, 0));
        if (margins.left() >= 0) {
            if (platformWindow) {
                qCDebug(lcQpaWindows) << __FUNCTION__ << "WM_NCCALCSIZE for" << hwnd << margins;
                platformWindow->setFullFrameMargins(margins);
            } else {
                const QSharedPointer<QWindowCreationContext> ctx = QWindowsContext::instance()->windowCreationContext();
                if (!ctx.isNull())
                    ctx->margins = margins;
            }
        }
    }
    return result;
}


static inline QByteArray nativeEventType() { return QByteArrayLiteral("windows_generic_MSG"); }

// Send to QAbstractEventDispatcher
bool QWindowsContext::filterNativeEvent(MSG *msg, LRESULT *result)
{
    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qintptr filterResult = 0;
#else
    long filterResult = 0;
#endif
    if (dispatcher && dispatcher->filterNativeEvent(nativeEventType(), msg, &filterResult)) {
        *result = LRESULT(filterResult);
        return true;
    }
    return false;
}

// Send to QWindowSystemInterface
bool QWindowsContext::filterNativeEvent(QWindow *window, MSG *msg, LRESULT *result)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qintptr filterResult = 0;
#else
    long filterResult = 0;
#endif
    if (QWindowSystemInterface::handleNativeEvent(window, nativeEventType(), msg, &filterResult)) {
        *result = LRESULT(filterResult);
        return true;
    }
    return false;
}

QT_END_NAMESPACE
