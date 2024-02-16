// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qt_windows.h>

#include "qwindowswindow.h"
#include "qwindowscontext.h"
#if QT_CONFIG(draganddrop)
#  include "qwindowsdrag.h"
#endif
#include "qwindowsscreen.h"
#include "qwindowsintegration.h"
#include "qwindowsmenu.h"
#include "qwindowsnativeinterface.h"
#if QT_CONFIG(dynamicgl)
#  include "qwindowsglcontext.h"
#else
#  include "qwindowsopenglcontext.h"
#endif
#include "qwindowsopengltester.h"
#ifdef QT_NO_CURSOR
#  include "qwindowscursor.h"
#endif

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <QtGui/qregion.h>
#include <QtGui/qopenglcontext.h>
#include <private/qwindow_p.h> // QWINDOWSIZE_MAX
#include <private/qguiapplication_p.h>
#include <private/qhighdpiscaling_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <QtCore/qdebug.h>
#include <QtCore/qlibraryinfo.h>

#include <dwmapi.h>

#if QT_CONFIG(vulkan)
#include "qwindowsvulkaninstance.h"
#endif

#include <shellscalingapi.h>

QT_BEGIN_NAMESPACE

using QWindowCreationContextPtr = QSharedPointer<QWindowCreationContext>;

enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

Q_GUI_EXPORT HICON qt_pixmapToWinHICON(const QPixmap &);

static QByteArray debugWinStyle(DWORD style)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(qulonglong(style), 16);
    if (style & WS_POPUP)
        rc += " WS_POPUP";
    if (style & WS_CHILD)
        rc += " WS_CHILD";
    if (style & WS_OVERLAPPED)
        rc += " WS_OVERLAPPED";
    if (style & WS_CLIPSIBLINGS)
        rc += " WS_CLIPSIBLINGS";
    if (style & WS_CLIPCHILDREN)
        rc += " WS_CLIPCHILDREN";
    if (style & WS_THICKFRAME)
        rc += " WS_THICKFRAME";
    if (style & WS_DLGFRAME)
        rc += " WS_DLGFRAME";
    if (style & WS_SYSMENU)
        rc += " WS_SYSMENU";
    if (style & WS_MINIMIZEBOX)
        rc += " WS_MINIMIZEBOX";
    if (style & WS_MAXIMIZEBOX)
        rc += " WS_MAXIMIZEBOX";
    if (style & WS_BORDER)
        rc += " WS_BORDER";
    if (style & WS_CAPTION)
        rc += " WS_CAPTION";
    if (style & WS_CHILDWINDOW)
        rc += " WS_CHILDWINDOW";
    if (style & WS_DISABLED)
        rc += " WS_DISABLED";
    if (style & WS_GROUP)
        rc += " WS_GROUP";
    if (style & WS_HSCROLL)
        rc += " WS_HSCROLL";
    if (style & WS_ICONIC)
        rc += " WS_ICONIC";
    if (style & WS_MAXIMIZE)
        rc += " WS_MAXIMIZE";
    if (style & WS_MINIMIZE)
        rc += " WS_MINIMIZE";
    if (style & WS_SIZEBOX)
        rc += " WS_SIZEBOX";
    if (style & WS_TABSTOP)
        rc += " WS_TABSTOP";
    if (style & WS_TILED)
        rc += " WS_TILED";
    if (style & WS_VISIBLE)
        rc += " WS_VISIBLE";
    if (style & WS_VSCROLL)
        rc += " WS_VSCROLL";
    return rc;
}

static QByteArray debugWinExStyle(DWORD exStyle)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(qulonglong(exStyle), 16);
    if (exStyle & WS_EX_TOOLWINDOW)
        rc += " WS_EX_TOOLWINDOW";
    if (exStyle & WS_EX_CONTEXTHELP)
        rc += " WS_EX_CONTEXTHELP";
    if (exStyle & WS_EX_LAYERED)
        rc += " WS_EX_LAYERED";
    if (exStyle & WS_EX_DLGMODALFRAME)
        rc += " WS_EX_DLGMODALFRAME";
    if (exStyle & WS_EX_LAYOUTRTL)
        rc += " WS_EX_LAYOUTRTL";
    if (exStyle & WS_EX_NOINHERITLAYOUT)
        rc += " WS_EX_NOINHERITLAYOUT";
    if (exStyle & WS_EX_ACCEPTFILES)
        rc += " WS_EX_ACCEPTFILES";
    if (exStyle & WS_EX_APPWINDOW)
        rc += " WS_EX_APPWINDOW";
    if (exStyle & WS_EX_CLIENTEDGE)
        rc += " WS_EX_CLIENTEDGE";
    if (exStyle & WS_EX_COMPOSITED)
        rc += " WS_EX_COMPOSITED";
    if (exStyle & WS_EX_CONTROLPARENT)
        rc += " WS_EX_CONTROLPARENT";
    if (exStyle & WS_EX_LEFT)
        rc += " WS_EX_LEFT";
    if (exStyle & WS_EX_LEFTSCROLLBAR)
        rc += " WS_EX_LEFTSCROLLBAR";
    if (exStyle & WS_EX_LTRREADING)
        rc += " WS_EX_LTRREADING";
    if (exStyle & WS_EX_MDICHILD)
        rc += " WS_EX_MDICHILD";
    if (exStyle & WS_EX_NOACTIVATE)
        rc += " WS_EX_NOACTIVATE";
    if (exStyle & WS_EX_NOPARENTNOTIFY)
        rc += " WS_EX_NOPARENTNOTIFY";
    if (exStyle & WS_EX_NOREDIRECTIONBITMAP)
        rc += " WS_EX_NOREDIRECTIONBITMAP";
    if (exStyle & WS_EX_RIGHT)
        rc += " WS_EX_RIGHT";
    if (exStyle & WS_EX_RIGHTSCROLLBAR)
        rc += " WS_EX_RIGHTSCROLLBAR";
    if (exStyle & WS_EX_RTLREADING)
        rc += " WS_EX_RTLREADING";
    if (exStyle & WS_EX_STATICEDGE)
        rc += " WS_EX_STATICEDGE";
    if (exStyle & WS_EX_TOPMOST)
        rc += " WS_EX_TOPMOST";
    if (exStyle & WS_EX_TRANSPARENT)
        rc += " WS_EX_TRANSPARENT";
    if (exStyle & WS_EX_WINDOWEDGE)
        rc += " WS_EX_WINDOWEDGE";
    return rc;
}

static QByteArray debugWinSwpPos(UINT flags)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(flags, 16);
    if (flags & SWP_FRAMECHANGED)
        rc += " SWP_FRAMECHANGED";
    if (flags & SWP_HIDEWINDOW)
        rc += " SWP_HIDEWINDOW";
    if (flags & SWP_NOACTIVATE)
        rc += " SWP_NOACTIVATE";
    if (flags & SWP_NOCOPYBITS)
        rc += " SWP_NOCOPYBITS";
    if (flags & SWP_NOMOVE)
        rc += " SWP_NOMOVE";
    if (flags & SWP_NOOWNERZORDER)
        rc += " SWP_NOOWNERZORDER";
    if (flags & SWP_NOREDRAW)
        rc += " SWP_NOREDRAW";
    if (flags & SWP_NOSENDCHANGING)
        rc += " SWP_NOSENDCHANGING";
    if (flags & SWP_NOSIZE)
        rc += " SWP_NOSIZE";
    if (flags & SWP_NOZORDER)
        rc += " SWP_NOZORDER";
    if (flags & SWP_SHOWWINDOW)
        rc += " SWP_SHOWWINDOW";
    if (flags & SWP_ASYNCWINDOWPOS)
        rc += " SWP_ASYNCWINDOWPOS";
    if (flags & SWP_DEFERERASE)
        rc += " SWP_DEFERERASE";
    if (flags & SWP_DRAWFRAME)
        rc += " SWP_DRAWFRAME";
    if (flags & SWP_NOREPOSITION)
        rc += " SWP_NOREPOSITION";
    return rc;
}

[[nodiscard]] static inline QByteArray debugWindowPlacementFlags(const UINT flags)
{
    QByteArray rc = "0x";
    rc += QByteArray::number(flags, 16);
    if (flags & WPF_SETMINPOSITION)
        rc += " WPF_SETMINPOSITION";
    if (flags & WPF_RESTORETOMAXIMIZED)
        rc += " WPF_RESTORETOMAXIMIZED";
    if (flags & WPF_ASYNCWINDOWPLACEMENT)
        rc += " WPF_ASYNCWINDOWPLACEMENT";
    return rc;
}

[[nodiscard]] static inline QByteArray debugShowWindowCmd(const UINT cmd)
{
    QByteArray rc = {};
    rc += QByteArray::number(cmd);
    if (cmd == SW_HIDE)
        rc += " SW_HIDE";
    if (cmd == SW_SHOWNORMAL)
        rc += " SW_SHOWNORMAL";
    if (cmd == SW_NORMAL)
        rc += " SW_NORMAL";
    if (cmd == SW_SHOWMINIMIZED)
        rc += " SW_SHOWMINIMIZED";
    if (cmd == SW_SHOWMAXIMIZED)
        rc += " SW_SHOWMAXIMIZED";
    if (cmd == SW_MAXIMIZE)
        rc += " SW_MAXIMIZE";
    if (cmd == SW_SHOWNOACTIVATE)
        rc += " SW_SHOWNOACTIVATE";
    if (cmd == SW_SHOW)
        rc += " SW_SHOW";
    if (cmd == SW_MINIMIZE)
        rc += " SW_MINIMIZE";
    if (cmd == SW_SHOWMINNOACTIVE)
        rc += " SW_SHOWMINNOACTIVE";
    if (cmd == SW_SHOWNA)
        rc += " SW_SHOWNA";
    if (cmd == SW_RESTORE)
        rc += " SW_RESTORE";
    if (cmd == SW_SHOWDEFAULT)
        rc += " SW_SHOWDEFAULT";
    if (cmd == SW_FORCEMINIMIZE)
        rc += " SW_FORCEMINIMIZE";
    return rc;
}

static inline QSize qSizeOfRect(const RECT &rect)
{
    return QSize(rect.right -rect.left, rect.bottom - rect.top);
}

static inline QRect qrectFromRECT(const RECT &rect)
{
    return QRect(QPoint(rect.left, rect.top), qSizeOfRect(rect));
}

static inline RECT RECTfromQRect(const QRect &rect)
{
    const int x = rect.left();
    const int y = rect.top();
    RECT result = { x, y, x + rect.width(), y + rect.height() };
    return result;
}


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const RECT &r)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "RECT(left=" << r.left << ", top=" << r.top
        << ", right=" << r.right << ", bottom=" << r.bottom
        << " (" << r.right - r.left << 'x' << r.bottom - r.top << "))";
    return d;
}

QDebug operator<<(QDebug d, const POINT &p)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "POINT(x=" << p.x << ", y=" << p.y << ')';
    return d;
}

QDebug operator<<(QDebug d, const WINDOWPOS &wp)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d <<  "WINDOWPOS(flags=" << debugWinSwpPos(wp.flags) << ", hwnd="
       << wp.hwnd << ", hwndInsertAfter=" << wp.hwndInsertAfter << ", x=" << wp.x
       <<  ", y=" << wp.y << ", cx=" << wp.cx <<  ", cy=" << wp.cy << ')';
    return d;
}

QDebug operator<<(QDebug d, const NCCALCSIZE_PARAMS &p)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "NCCALCSIZE_PARAMS(rgrc=[" << p.rgrc[0] << ", " << p.rgrc[1] << ", "
        << p.rgrc[2] << "], lppos=" << *p.lppos << ')';
    return d;
}

QDebug operator<<(QDebug d, const MINMAXINFO &i)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "MINMAXINFO(maxSize=" << i.ptMaxSize << ", "
      << "maxpos=" << i.ptMaxPosition << ", "
      << "maxtrack=" << i.ptMaxTrackSize << ", "
      << "mintrack=" << i.ptMinTrackSize << ')';
    return d;
}

QDebug operator<<(QDebug d, const WINDOWPLACEMENT &wp)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d <<  "WINDOWPLACEMENT(flags=" << debugWindowPlacementFlags(wp.flags) << ", showCmd="
      << debugShowWindowCmd(wp.showCmd) << ", ptMinPosition=" << wp.ptMinPosition
      << ", ptMaxPosition=" << wp.ptMaxPosition << ", rcNormalPosition="
      << wp.rcNormalPosition << ')';
    return d;
}

QDebug operator<<(QDebug d, const GUID &guid)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << '{' << Qt::hex << Qt::uppercasedigits << qSetPadChar(u'0')
      << qSetFieldWidth(8) << guid.Data1
      << qSetFieldWidth(0) << '-' << qSetFieldWidth(4)
      << guid.Data2 << qSetFieldWidth(0) << '-' << qSetFieldWidth(4)
      << guid.Data3 << qSetFieldWidth(0) << '-' << qSetFieldWidth(4)
      << qSetFieldWidth(2) << guid.Data4[0] << guid.Data4[1]
      << qSetFieldWidth(0) << '-' << qSetFieldWidth(2);
    for (int i = 2; i < 8; ++i)
        d << guid.Data4[i];
    d << qSetFieldWidth(0) << '}';
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

static void formatBriefRectangle(QDebug &d, const QRect &r)
{
    d << r.width() << 'x' << r.height() << Qt::forcesign << r.x() << r.y() << Qt::noforcesign;
}

static void formatBriefMargins(QDebug &d, const QMargins &m)
{
    d << m.left() << ", " << m.top() << ", " << m.right() << ", " << m.bottom();
}

// QTBUG-43872, for windows that do not have WS_EX_TOOLWINDOW set, WINDOWPLACEMENT
// is in workspace/available area coordinates.
static QPoint windowPlacementOffset(HWND hwnd, const QPoint &point)
{
    if (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return QPoint(0, 0);
    const QWindowsScreenManager &screenManager = QWindowsContext::instance()->screenManager();
    const QWindowsScreen *screen = screenManager.screens().size() == 1
        ? screenManager.screens().constFirst() : screenManager.screenAtDp(point);
    if (screen)
        return screen->availableGeometry().topLeft() - screen->geometry().topLeft();
    return QPoint(0, 0);
}

// Return the frame geometry relative to the parent
// if there is one.
static inline QRect frameGeometry(HWND hwnd, bool topLevel)
{
    RECT rect = { 0, 0, 0, 0 };
    if (topLevel) {
        WINDOWPLACEMENT windowPlacement;
        windowPlacement.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hwnd, &windowPlacement);
        if (windowPlacement.showCmd == SW_SHOWMINIMIZED) {
            const QRect result = qrectFromRECT(windowPlacement.rcNormalPosition);
            return result.translated(windowPlacementOffset(hwnd, result.topLeft()));
        }
    }
    GetWindowRect(hwnd, &rect); // Screen coordinates.
    const HWND parent = GetParent(hwnd);
    if (parent && !topLevel) {
        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        POINT leftTop = { rect.left, rect.top };
        screenToClient(parent, &leftTop);
        rect.left = leftTop.x;
        rect.top = leftTop.y;
        rect.right = leftTop.x + width;
        rect.bottom = leftTop.y + height;
    }
    return qrectFromRECT(rect);
}

// Return the visibility of the Window (except full screen since it is not a window state).
static QWindow::Visibility windowVisibility_sys(HWND hwnd)
{
    if (!IsWindowVisible(hwnd))
        return QWindow::Hidden;
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &windowPlacement)) {
        switch (windowPlacement.showCmd) {
        case SW_SHOWMINIMIZED:
        case SW_MINIMIZE:
        case SW_FORCEMINIMIZE:
            return QWindow::Minimized;
        case SW_SHOWMAXIMIZED:
            return QWindow::Maximized;
        default:
            break;
        }
    }
    return QWindow::Windowed;
}

static inline bool windowIsAccelerated(const QWindow *w)
{
    switch (w->surfaceType()) {
    case QSurface::OpenGLSurface:
        return true;
    case QSurface::RasterGLSurface:
        return qt_window_private(const_cast<QWindow *>(w))->compositing;
    case QSurface::VulkanSurface:
        return true;
    case QSurface::Direct3DSurface:
        return true;
    default:
        return false;
    }
}

static bool applyBlurBehindWindow(HWND hwnd)
{
    DWM_BLURBEHIND blurBehind = {0, 0, nullptr, 0};

    blurBehind.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    blurBehind.fEnable = TRUE;
    blurBehind.hRgnBlur = CreateRectRgn(0, 0, -1, -1);

    const bool result = DwmEnableBlurBehindWindow(hwnd, &blurBehind) == S_OK;

    if (blurBehind.hRgnBlur)
        DeleteObject(blurBehind.hRgnBlur);

    return result;
}

// from qwidget_win.cpp, pass flags separately in case they have been "autofixed".
static bool shouldShowMaximizeButton(const QWindow *w, Qt::WindowFlags flags)
{
    if ((flags & Qt::MSWindowsFixedSizeDialogHint) || !(flags & Qt::WindowMaximizeButtonHint))
        return false;
    // if the user explicitly asked for the maximize button, we try to add
    // it even if the window has fixed size.
    return (flags & Qt::CustomizeWindowHint) ||
        w->maximumSize() == QSize(QWINDOWSIZE_MAX, QWINDOWSIZE_MAX);
}

bool QWindowsWindow::hasNoNativeFrame(HWND hwnd, Qt::WindowFlags flags)
{
    const LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    return (style & WS_CHILD) || (flags & Qt::FramelessWindowHint);
}

// Set the WS_EX_LAYERED flag on a HWND if required. This is required for
// translucent backgrounds, not fully opaque windows and for
// Qt::WindowTransparentForInput (in combination with WS_EX_TRANSPARENT).
bool QWindowsWindow::setWindowLayered(HWND hwnd, Qt::WindowFlags flags, bool hasAlpha, qreal opacity)
{
    const LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    // Native children are frameless by nature, so check for that as well.
    const bool needsLayered = (flags & Qt::WindowTransparentForInput)
        || (hasAlpha && hasNoNativeFrame(hwnd, flags)) || opacity < 1.0;
    const bool isLayered = (exStyle & WS_EX_LAYERED);
    if (needsLayered != isLayered) {
        if (needsLayered) {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
        } else {
            SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYERED);
        }
    }
    return needsLayered;
}

static void setWindowOpacity(HWND hwnd, Qt::WindowFlags flags, bool hasAlpha, bool accelerated, qreal level)
{
    if (QWindowsWindow::setWindowLayered(hwnd, flags, hasAlpha, level)) {
        const BYTE alpha = BYTE(qRound(255.0 * level));
        if (hasAlpha && !accelerated && QWindowsWindow::hasNoNativeFrame(hwnd, flags)) {
            // Non-GL windows with alpha: Use blend function to update.
            BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
            UpdateLayeredWindow(hwnd, nullptr, nullptr, nullptr, nullptr, nullptr, 0, &blend, ULW_ALPHA);
        } else {
            SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
        }
    } else if (IsWindowVisible(hwnd)) { // Repaint when switching from layered.
        InvalidateRect(hwnd, nullptr, TRUE);
    }
}

static inline void updateGLWindowSettings(const QWindow *w, HWND hwnd, Qt::WindowFlags flags, qreal opacity)
{
    const bool isAccelerated = windowIsAccelerated(w);
    const bool hasAlpha = w->format().hasAlpha();

    if (isAccelerated && hasAlpha)
        applyBlurBehindWindow(hwnd);

    setWindowOpacity(hwnd, flags, hasAlpha, isAccelerated, opacity);
}

[[nodiscard]] static inline int getResizeBorderThickness(const UINT dpi)
{
    // The width of the padded border will always be 0 if DWM composition is
    // disabled, but since it will always be enabled and can't be programtically
    // disabled from Windows 8, we are safe to go.
    return GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi)
           + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
}

/*!
    Calculates the dimensions of the invisible borders within the
    window frames which only exist on Windows 10 and onwards.
*/

static QMargins invisibleMargins(QPoint screenPoint)
{
    POINT pt = {screenPoint.x(), screenPoint.y()};
    if (HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL)) {
        UINT dpiX;
        UINT dpiY;
        if (SUCCEEDED(GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
            const int gap = getResizeBorderThickness(dpiX);
            return QMargins(gap, 0, gap, gap);
        }
    }
    return QMargins();
}

[[nodiscard]] static inline QMargins invisibleMargins(const HWND hwnd)
{
    const UINT dpi = GetDpiForWindow(hwnd);
    const int gap = getResizeBorderThickness(dpi);
    return QMargins(gap, 0, gap, gap);
}

/*!
    \class WindowCreationData
    \brief Window creation code.

    This struct gathers all information required to create a window.
    Window creation is split in 3 steps:

    \list
    \li fromWindow() Gather all required information
    \li create() Create the system handle.
    \li initialize() Post creation initialization steps.
    \endlist

    The reason for this split is to also enable changing the QWindowFlags
    by calling:

    \list
    \li fromWindow() Gather information and determine new system styles
    \li applyWindowFlags() to apply the new window system styles.
    \li initialize() Post creation initialization steps.
    \endlist

    Contains the window creation code formerly in qwidget_win.cpp.

    \sa QWindowCreationContext
    \internal
*/

struct WindowCreationData
{
    using WindowData = QWindowsWindowData;
    enum Flags { ForceChild = 0x1, ForceTopLevel = 0x2 };

    void fromWindow(const QWindow *w, const Qt::WindowFlags flags, unsigned creationFlags = 0);
    inline WindowData create(const QWindow *w, const WindowData &data, QString title) const;
    inline void applyWindowFlags(HWND hwnd) const;
    void initialize(const QWindow *w, HWND h, bool frameChange, qreal opacityLevel) const;

    Qt::WindowFlags flags;
    HWND parentHandle = nullptr;
    Qt::WindowType type = Qt::Widget;
    unsigned style = 0;
    unsigned exStyle = 0;
    bool topLevel = false;
    bool popup = false;
    bool dialog = false;
    bool tool = false;
    bool embedded = false;
    bool hasAlpha = false;
};

QDebug operator<<(QDebug debug, const WindowCreationData &d)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();
    debug << "WindowCreationData: " << d.flags
        << "\n  topLevel=" << d.topLevel;
     if (d.parentHandle)
         debug << " parent=" << d.parentHandle;
     debug << " popup=" << d.popup << " dialog=" << d.dialog
        << " embedded=" << d.embedded << " tool=" << d.tool
        << "\n  style=" << debugWinStyle(d.style);
    if (d.exStyle)
        debug << "\n  exStyle=" << debugWinExStyle(d.exStyle);
    return debug;
}

// Fix top level window flags in case only the type flags are passed.
static inline void fixTopLevelWindowFlags(Qt::WindowFlags &flags)
{
    // Not supported on Windows, also do correction when it is set.
    flags &= ~Qt::WindowFullscreenButtonHint;
    switch (flags) {
    case Qt::Window:
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint
              |Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint;
        break;
    case Qt::Dialog:
    case Qt::Tool:
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
        break;
    default:
        break;
    }
    if ((flags & Qt::WindowType_Mask) == Qt::SplashScreen)
        flags |= Qt::FramelessWindowHint;
}

static QScreen *screenForDeviceName(const QWindow *w, const QString &name)
{
    const auto getDeviceName = [](const QScreen *screen) -> QString {
        if (const auto s = static_cast<const QWindowsScreen *>(screen->handle()))
            return s->data().deviceName;
        return {};
    };
    QScreen *winScreen = w ? w->screen() : QGuiApplication::primaryScreen();
    if (winScreen && getDeviceName(winScreen) != name) {
        const auto screens = winScreen->virtualSiblings();
        for (QScreen *screen : screens) {
            if (getDeviceName(screen) == name)
                return screen;
        }
    }
    return winScreen;
}

static QPoint calcPosition(const QWindow *w, const QWindowCreationContextPtr &context, const QMargins &invMargins)
{
    const QPoint orgPos(context->frameX - invMargins.left(), context->frameY - invMargins.top());

    if (!w || w->type() != Qt::Window)
        return orgPos;

    // Workaround for QTBUG-50371
    const QScreen *screenForGL = QWindowsWindow::forcedScreenForGLWindow(w);
    if (!screenForGL)
        return orgPos;

    const QPoint posFrame(context->frameX, context->frameY);
    const QMargins margins = context->margins;
    const QRect scrGeo = screenForGL->handle()->availableGeometry();

    // Point is already in the required screen.
    if (scrGeo.contains(orgPos))
        return orgPos;

    // If the visible part of the window is already in the
    // required screen, just ignore the invisible offset.
    if (scrGeo.contains(posFrame))
        return posFrame;

    // Find the original screen containing the coordinates.
    const auto screens = screenForGL->virtualSiblings();
    const QScreen *orgScreen = nullptr;
    for (QScreen *screen : screens) {
        if (screen->handle()->availableGeometry().contains(posFrame)) {
            orgScreen = screen;
            break;
        }
    }
    const QPoint ctPos = QPoint(qMax(scrGeo.left(), scrGeo.center().x()
                                     + (margins.right() - margins.left() - context->frameWidth)/2),
                                qMax(scrGeo.top(), scrGeo.center().y()
                                     + (margins.bottom() - margins.top() - context->frameHeight)/2));

    // If initial coordinates were outside all screens, center the window on the required screen.
    if (!orgScreen)
        return ctPos;

    const QRect orgGeo = orgScreen->handle()->availableGeometry();
    const QRect orgFrame(QPoint(context->frameX, context->frameY),
                         QSize(context->frameWidth, context->frameHeight));

    // Window would be centered on orgScreen. Center it on the required screen.
    if (orgGeo.center() == (orgFrame - margins).center())
        return ctPos;

    // Transform the coordinates to map them into the required screen.
    const QPoint newPos(scrGeo.left() + ((posFrame.x() - orgGeo.left()) * scrGeo.width()) / orgGeo.width(),
                        scrGeo.top() + ((posFrame.y() - orgGeo.top()) * scrGeo.height()) / orgGeo.height());
    const QPoint newPosNoMargin(newPos.x() - invMargins.left(), newPos.y() - invMargins.top());

    return scrGeo.contains(newPosNoMargin) ? newPosNoMargin : newPos;
}

void WindowCreationData::fromWindow(const QWindow *w, const Qt::WindowFlags flagsIn,
                                    unsigned creationFlags)
{
    flags = flagsIn;

    // Sometimes QWindow doesn't have a QWindow parent but does have a native parent window,
    // e.g. in case of embedded ActiveQt servers. They should not be considered a top-level
    // windows in such cases.
    QVariant prop = w->property(QWindowsWindow::embeddedNativeParentHandleProperty);
    if (prop.isValid()) {
        embedded = true;
        parentHandle = reinterpret_cast<HWND>(prop.value<WId>());
    }

    if (creationFlags & ForceChild) {
        topLevel = false;
    } else if (embedded) {
        // Embedded native windows (for example Active X server windows) are by
        // definition never toplevel, even though they do not have QWindow parents.
        topLevel = false;
    } else {
        topLevel = (creationFlags & ForceTopLevel) ? true : w->isTopLevel();
    }

    if (topLevel)
        fixTopLevelWindowFlags(flags);

    type = static_cast<Qt::WindowType>(int(flags) & Qt::WindowType_Mask);
    switch (type) {
    case Qt::Dialog:
    case Qt::Sheet:
        dialog = true;
        break;
    case Qt::Drawer:
    case Qt::Tool:
        tool = true;
        break;
    case Qt::Popup:
        popup = true;
        break;
    default:
        break;
    }
    if ((flags & Qt::MSWindowsFixedSizeDialogHint))
        dialog = true;

    // This causes the title bar to drawn RTL and the close button
    // to be left. Note that this causes:
    // - All DCs created on the Window to have RTL layout (see SetLayout)
    // - ClientToScreen() and ScreenToClient() to work in reverse as well.
    // - Mouse event coordinates to be mirrored.
    // - Positioning of child Windows.
    if (QGuiApplication::layoutDirection() == Qt::RightToLeft
        && (QWindowsIntegration::instance()->options() & QWindowsIntegration::RtlEnabled) != 0) {
        exStyle |= WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT;
    }

    // Parent: Use transient parent for top levels.
    if (popup) {
        flags |= Qt::WindowStaysOnTopHint; // a popup stays on top, no parent.
    } else if (!embedded) {
        if (const QWindow *parentWindow = topLevel ? w->transientParent() : w->parent())
            parentHandle = QWindowsWindow::handleOf(parentWindow);
    }

    if (popup || (type == Qt::ToolTip) || (type == Qt::SplashScreen)) {
        style = WS_POPUP;
    } else if (topLevel) {
        if (flags & Qt::FramelessWindowHint)
            style = WS_POPUP;                // no border
        else if (flags & Qt::WindowTitleHint)
            style = WS_OVERLAPPED;
        else
            style = 0;
    } else {
        style = WS_CHILD;
    }

    // if (!testAttribute(Qt::WA_PaintUnclipped))
    // ### Commented out for now as it causes some problems, but
    // this should be correct anyway, so dig some more into this
#ifdef Q_FLATTEN_EXPOSE
    if (windowIsOpenGL(w)) // a bit incorrect since the is-opengl status may change from false to true at any time later on
        style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN; // see SetPixelFormat
#else
    style |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN ;
#endif
    if (topLevel) {
        if ((type == Qt::Window || dialog || tool)) {
            if (!(flags & Qt::FramelessWindowHint)) {
                style |= WS_POPUP;
                if (flags & Qt::MSWindowsFixedSizeDialogHint) {
                    style |= WS_DLGFRAME;
                } else {
                    style |= WS_THICKFRAME;
                }
                if (flags & Qt::WindowTitleHint)
                    style |= WS_CAPTION; // Contains WS_DLGFRAME
            }
            if (flags & Qt::WindowSystemMenuHint)
                style |= WS_SYSMENU;
            else if (dialog && (flags & Qt::WindowCloseButtonHint) && !(flags & Qt::FramelessWindowHint)) {
                style |= WS_SYSMENU | WS_BORDER; // QTBUG-2027, dialogs without system menu.
                exStyle |= WS_EX_DLGMODALFRAME;
            }
            const bool showMinimizeButton = flags & Qt::WindowMinimizeButtonHint;
            if (showMinimizeButton)
                style |= WS_MINIMIZEBOX;
            const bool showMaximizeButton = shouldShowMaximizeButton(w, flags);
            if (showMaximizeButton)
                style |= WS_MAXIMIZEBOX;
            if (showMinimizeButton || showMaximizeButton)
                style |= WS_SYSMENU;
            if (tool)
                exStyle |= WS_EX_TOOLWINDOW;
            if ((flags & Qt::WindowContextHelpButtonHint) && !showMinimizeButton
                && !showMaximizeButton)
                exStyle |= WS_EX_CONTEXTHELP;
        } else {
             exStyle |= WS_EX_TOOLWINDOW;
        }

        // make mouse events fall through this window
        // NOTE: WS_EX_TRANSPARENT flag can make mouse inputs fall through a layered window
        if (flagsIn & Qt::WindowTransparentForInput)
            exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
    }
}

static inline bool shouldApplyDarkFrame(const QWindow *w)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return false;
    // the application has explicitly opted out of dark frames
    if (!QWindowsIntegration::instance()->darkModeHandling().testFlag(QWindowsApplication::DarkModeWindowFrames))
        return false;

    // if the application supports a dark border, and the palette is dark (window background color
    // is darker than the text), then turn dark-border support on, otherwise use a light border.
    auto *dWindow = QWindowPrivate::get(const_cast<QWindow*>(w));
    const QPalette windowPal = dWindow->windowPalette();
    return windowPal.color(QPalette::WindowText).lightness()
         > windowPal.color(QPalette::Window).lightness();
}

QWindowsWindowData
    WindowCreationData::create(const QWindow *w, const WindowData &data, QString title) const
{
    WindowData result;
    result.flags = flags;

    const auto appinst = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));

    const QString windowClassName = QWindowsContext::instance()->registerWindowClass(w);

    const QScreen *screen{};
    const QRect rect = QPlatformWindow::initialGeometry(w, data.geometry,
                                                        defaultWindowWidth, defaultWindowHeight,
                                                        &screen);

    if (title.isEmpty() && (result.flags & Qt::WindowTitleHint))
        title = topLevel ? qAppName() : w->objectName();

    const auto *titleUtf16 = reinterpret_cast<const wchar_t *>(title.utf16());
    const auto *classNameUtf16 = reinterpret_cast<const wchar_t *>(windowClassName.utf16());

    // Capture events before CreateWindowEx() returns. The context is cleared in
    // the QWindowsWindow constructor.
    const QWindowCreationContextPtr context(new QWindowCreationContext(w, screen, data.geometry,
                                                                       rect, data.customMargins,
                                                                       style, exStyle));
    QWindowsContext::instance()->setWindowCreationContext(context);

    const bool hasFrame = (style & (WS_DLGFRAME | WS_THICKFRAME))
            && !(result.flags & Qt::FramelessWindowHint);
    QMargins invMargins = topLevel && hasFrame && QWindowsGeometryHint::positionIncludesFrame(w)
            ? invisibleMargins(QPoint(context->frameX, context->frameY)) : QMargins();

    qCDebug(lcQpaWindow).nospace()
        << "CreateWindowEx: " << w << " class=" << windowClassName << " title=" << title
        << '\n' << *this << "\nrequested: " << rect << ": "
        << context->frameWidth << 'x' <<  context->frameHeight
        << '+' << context->frameX << '+' << context->frameY
        << " custom margins: " << context->customMargins
        << " invisible margins: " << invMargins;


    QPoint pos = calcPosition(w, context, invMargins);

    // Mirror the position when creating on a parent in RTL mode, ditto for the obtained geometry.
    int mirrorParentWidth = 0;
    if (!w->isTopLevel() && QWindowsBaseWindow::isRtlLayout(parentHandle)) {
        RECT rect;
        GetClientRect(parentHandle, &rect);
        mirrorParentWidth = rect.right;
    }
    if (mirrorParentWidth != 0 && pos.x() != CW_USEDEFAULT && context->frameWidth != CW_USEDEFAULT)
        pos.setX(mirrorParentWidth - context->frameWidth - pos.x());

    result.hwnd = CreateWindowEx(exStyle, classNameUtf16, titleUtf16,
                                 style,
                                 pos.x(), pos.y(),
                                 context->frameWidth, context->frameHeight,
                                 parentHandle, nullptr, appinst, nullptr);
    qCDebug(lcQpaWindow).nospace()
        << "CreateWindowEx: returns " << w << ' ' << result.hwnd << " obtained geometry: "
        << context->obtainedPos << context->obtainedSize << ' ' << context->margins;

    if (!result.hwnd) {
        qErrnoWarning("%s: CreateWindowEx failed", __FUNCTION__);
        return result;
    }

    if (QWindowsContext::isDarkMode() && shouldApplyDarkFrame(w))
        QWindowsWindow::setDarkBorderToWindow(result.hwnd, true);

    if (mirrorParentWidth != 0) {
        context->obtainedPos.setX(mirrorParentWidth - context->obtainedSize.width()
                                  -  context->obtainedPos.x());
    }

    QRect obtainedGeometry(context->obtainedPos, context->obtainedSize);

    result.geometry = obtainedGeometry;
    result.restoreGeometry = frameGeometry(result.hwnd, topLevel);
    result.fullFrameMargins = context->margins;
    result.embedded = embedded;
    result.hasFrame = hasFrame;
    result.customMargins = context->customMargins;

    return result;
}

void WindowCreationData::applyWindowFlags(HWND hwnd) const
{
    // Keep enabled and visible from the current style.
    const LONG_PTR oldStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    const LONG_PTR oldExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);

    const LONG_PTR newStyle = style | (oldStyle & (WS_DISABLED|WS_VISIBLE));
    if (oldStyle != newStyle)
        SetWindowLongPtr(hwnd, GWL_STYLE, newStyle);
    const LONG_PTR newExStyle = exStyle;
    if (newExStyle != oldExStyle)
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, newExStyle);
    qCDebug(lcQpaWindow).nospace() << __FUNCTION__ << hwnd << *this
        << "\n    Style from " << debugWinStyle(DWORD(oldStyle)) << "\n    to "
        << debugWinStyle(DWORD(newStyle)) << "\n    ExStyle from "
        << debugWinExStyle(DWORD(oldExStyle)) << " to "
        << debugWinExStyle(DWORD(newExStyle));
}

void WindowCreationData::initialize(const QWindow *w, HWND hwnd, bool frameChange, qreal opacityLevel) const
{
    if (!hwnd)
        return;
    UINT swpFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER;
    if (frameChange)
        swpFlags |= SWP_FRAMECHANGED;
    if (topLevel) {
        swpFlags |= SWP_NOACTIVATE;
        if ((flags & Qt::WindowStaysOnTopHint) || (type == Qt::ToolTip)) {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, swpFlags);
            if (flags & Qt::WindowStaysOnBottomHint)
                qWarning("QWidget: Incompatible window flags: the window can't be on top and on bottom at the same time");
        } else if (flags & Qt::WindowStaysOnBottomHint) {
            SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, swpFlags);
        } else if (frameChange) { // Force WM_NCCALCSIZE with wParam=1 in case of custom margins.
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, swpFlags);
        }
        if (flags & (Qt::CustomizeWindowHint|Qt::WindowTitleHint)) {
            HMENU systemMenu = GetSystemMenu(hwnd, FALSE);
            if (flags & Qt::WindowCloseButtonHint)
                EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND|MF_ENABLED);
            else
                EnableMenuItem(systemMenu, SC_CLOSE, MF_BYCOMMAND|MF_GRAYED);
        }
        updateGLWindowSettings(w, hwnd, flags, opacityLevel);
    } else { // child.
        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, swpFlags);
    }
}


// Scaling helpers for size constraints.
static QSize toNativeSizeConstrained(QSize dip, const QScreen *s)
{
    if (QHighDpiScaling::isActive()) {
        const qreal factor = QHighDpiScaling::factor(s);
        if (!qFuzzyCompare(factor, qreal(1))) {
            if (dip.width() > 0 && dip.width() < QWINDOWSIZE_MAX)
                dip.setWidth(qRound(qreal(dip.width()) * factor));
            if (dip.height() > 0 && dip.height() < QWINDOWSIZE_MAX)
                dip.setHeight(qRound(qreal(dip.height()) * factor));
        }
    }
    return dip;
}

/*!
    \class QWindowsGeometryHint
    \brief Stores geometry constraints and provides utility functions.

    Geometry constraints ready to apply to a MINMAXINFO taking frame
    into account.

    \internal
*/

QMargins QWindowsGeometryHint::frameOnPrimaryScreen(const QWindow *w, DWORD style, DWORD exStyle)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return {};
    RECT rect = {0,0,0,0};
    style &= ~DWORD(WS_OVERLAPPED); // Not permitted, see docs.
    if (AdjustWindowRectEx(&rect, style, FALSE, exStyle) == FALSE)
        qErrnoWarning("%s: AdjustWindowRectEx failed", __FUNCTION__);
    const QMargins result(qAbs(rect.left), qAbs(rect.top),
                          qAbs(rect.right), qAbs(rect.bottom));
    qCDebug(lcQpaWindow).nospace() << __FUNCTION__ << " style="
        << Qt::showbase << Qt::hex << style << " exStyle=" << exStyle << Qt::dec << Qt::noshowbase
        << ' ' << rect << ' ' << result;
    return result;
}

QMargins QWindowsGeometryHint::frameOnPrimaryScreen(const QWindow *w, HWND hwnd)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return {};
    return frameOnPrimaryScreen(w, DWORD(GetWindowLongPtr(hwnd, GWL_STYLE)),
                                DWORD(GetWindowLongPtr(hwnd, GWL_EXSTYLE)));
}

QMargins QWindowsGeometryHint::frame(const QWindow *w, DWORD style, DWORD exStyle, qreal dpi)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return {};
    RECT rect = {0,0,0,0};
    style &= ~DWORD(WS_OVERLAPPED); // Not permitted, see docs.
    if (AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, unsigned(qRound(dpi))) == FALSE) {
        qErrnoWarning("%s: AdjustWindowRectExForDpi failed", __FUNCTION__);
    }
    const QMargins result(qAbs(rect.left), qAbs(rect.top),
                          qAbs(rect.right), qAbs(rect.bottom));
    qCDebug(lcQpaWindow).nospace() << __FUNCTION__ << " style="
        << Qt::showbase << Qt::hex << style << " exStyle=" << exStyle << Qt::dec << Qt::noshowbase
        << " dpi=" << dpi
        << ' ' << rect << ' ' << result;
    return result;
}

QMargins QWindowsGeometryHint::frame(const QWindow *w, HWND hwnd, DWORD style, DWORD exStyle)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return {};
    if (QWindowsScreenManager::isSingleScreen())
        return frameOnPrimaryScreen(w, style, exStyle);
    auto &screenManager = QWindowsContext::instance()->screenManager();
    auto screen = screenManager.screenForHwnd(hwnd);
    if (!screen)
        screen = screenManager.screens().value(0);
    const auto dpi = screen ? screen->logicalDpi().first : qreal(96);
    return frame(w, style, exStyle, dpi);
}

QMargins QWindowsGeometryHint::frame(const QWindow *w, HWND hwnd)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return {};
    return frame(w, hwnd, DWORD(GetWindowLongPtr(hwnd, GWL_STYLE)),
                 DWORD(GetWindowLongPtr(hwnd, GWL_EXSTYLE)));
}

// For newly created windows.
QMargins QWindowsGeometryHint::frame(const QWindow *w, const QRect &geometry,
                                     DWORD style, DWORD exStyle)
{
    if (!w->isTopLevel() || w->flags().testFlag(Qt::FramelessWindowHint))
        return {};
    if (QWindowsScreenManager::isSingleScreen()
        || !QWindowsContext::shouldHaveNonClientDpiScaling(w)) {
        return frameOnPrimaryScreen(w, style, exStyle);
    }
    qreal dpi = 96;
    auto &screenManager = QWindowsContext::instance()->screenManager();
    auto screen = screenManager.screenAtDp(geometry.center());
    if (!screen)
        screen = screenManager.screens().value(0);
    if (screen)
        dpi = screen->logicalDpi().first;
    return QWindowsGeometryHint::frame(w, style, exStyle, dpi);
}

bool QWindowsGeometryHint::handleCalculateSize(const QMargins &customMargins, const MSG &msg, LRESULT *result)
{
    // NCCALCSIZE_PARAMS structure if wParam==TRUE
    if (!msg.wParam || customMargins.isNull())
        return false;
    *result = DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
    auto *ncp = reinterpret_cast<NCCALCSIZE_PARAMS *>(msg.lParam);
    const RECT oldClientArea = ncp->rgrc[0];
    ncp->rgrc[0].left += customMargins.left();
    ncp->rgrc[0].top += customMargins.top();
    ncp->rgrc[0].right -= customMargins.right();
    ncp->rgrc[0].bottom -= customMargins.bottom();
    result = nullptr;
    qCDebug(lcQpaWindow).nospace() << __FUNCTION__ << oldClientArea << '+' << customMargins << "-->"
        << ncp->rgrc[0] << ' ' << ncp->rgrc[1] << ' ' << ncp->rgrc[2]
        << ' ' << ncp->lppos->cx << ',' << ncp->lppos->cy;
    return true;
}

void QWindowsGeometryHint::frameSizeConstraints(const QWindow *w, const QScreen *screen,
                                                const QMargins &margins,
                                                QSize *minimumSize, QSize *maximumSize)
{
    *minimumSize = toNativeSizeConstrained(w->minimumSize(), screen);
    *maximumSize = toNativeSizeConstrained(w->maximumSize(), screen);

    const int maximumWidth = qMax(maximumSize->width(), minimumSize->width());
    const int maximumHeight = qMax(maximumSize->height(), minimumSize->height());
    const int frameWidth = margins.left() + margins.right();
    const int frameHeight = margins.top() + margins.bottom();

    if (minimumSize->width() > 0)
        minimumSize->rwidth() += frameWidth;
    if (minimumSize->height() > 0)
        minimumSize->rheight() += frameHeight;
    if (maximumWidth < QWINDOWSIZE_MAX)
        maximumSize->setWidth(maximumWidth + frameWidth);
    if (maximumHeight < QWINDOWSIZE_MAX)
        maximumSize->setHeight(maximumHeight + frameHeight);
}

void QWindowsGeometryHint::applyToMinMaxInfo(const QWindow *w,
                                             const QScreen *screen,
                                             const QMargins &margins,
                                             MINMAXINFO *mmi)
{
    QSize minimumSize;
    QSize maximumSize;
    frameSizeConstraints(w, screen, margins, &minimumSize, &maximumSize);
    qCDebug(lcQpaWindow).nospace() << '>' << __FUNCTION__ << '<' << " min="
        << minimumSize.width() << ',' << minimumSize.height()
        << " max=" << maximumSize.width() << ',' << maximumSize.height()
        << " margins=" << margins
        << " in " << *mmi;

    if (minimumSize.width() > 0)
        mmi->ptMinTrackSize.x = minimumSize.width();
    if (minimumSize.height() > 0)
        mmi->ptMinTrackSize.y = minimumSize.height();

    if (maximumSize.width() < QWINDOWSIZE_MAX)
        mmi->ptMaxTrackSize.x = maximumSize.width();
    if (maximumSize.height() < QWINDOWSIZE_MAX)
        mmi->ptMaxTrackSize.y = maximumSize.height();
    qCDebug(lcQpaWindow).nospace() << '<' << __FUNCTION__ << " out " << *mmi;
}

void QWindowsGeometryHint::applyToMinMaxInfo(const QWindow *w,
                                             const QMargins &margins,
                                             MINMAXINFO *mmi)
{
    applyToMinMaxInfo(w, w->screen(), margins, mmi);
}

bool QWindowsGeometryHint::positionIncludesFrame(const QWindow *w)
{
    return qt_window_private(const_cast<QWindow *>(w))->positionPolicy
           == QWindowPrivate::WindowFrameInclusive;
}

/*!
    \class QWindowsBaseWindow
    \brief Base class for QWindowsForeignWindow, QWindowsWindow

    The class provides some _sys() getters for querying window
    data from a HWND and some _sys() setters.

    Derived classes wrapping foreign windows may use them directly
    to calculate geometry, margins, etc.

    Derived classes representing windows created by Qt may defer
    expensive calculations until change notifications are received.

    \since 5.6
    \internal
*/

bool QWindowsBaseWindow::isRtlLayout(HWND hwnd)
{
    return (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0;
}

QWindowsBaseWindow *QWindowsBaseWindow::baseWindowOf(const QWindow *w)
{
    if (w) {
        if (QPlatformWindow *pw = w->handle())
            return static_cast<QWindowsBaseWindow *>(pw);
    }
    return nullptr;
}

HWND QWindowsBaseWindow::handleOf(const QWindow *w)
{
    const QWindowsBaseWindow *bw = QWindowsBaseWindow::baseWindowOf(w);
    return bw ? bw->handle() : HWND(nullptr);
}

bool QWindowsBaseWindow::isTopLevel_sys() const
{
    const HWND parent = parentHwnd();
    return !parent || parent == GetDesktopWindow();
}

QRect QWindowsBaseWindow::frameGeometry_sys() const
{
    return frameGeometry(handle(), isTopLevel());
}

QRect QWindowsBaseWindow::geometry_sys() const
{
    return frameGeometry_sys().marginsRemoved(fullFrameMargins());
}

QMargins QWindowsBaseWindow::frameMargins_sys() const
{
    return QWindowsGeometryHint::frame(window(), handle(), style(), exStyle());
}

std::optional<QWindowsBaseWindow::TouchWindowTouchTypes>
    QWindowsBaseWindow::touchWindowTouchTypes_sys() const
{
    ULONG touchFlags = 0;
    if (IsTouchWindow(handle(), &touchFlags) == FALSE)
        return {};
    TouchWindowTouchTypes result;
    if ((touchFlags & TWF_FINETOUCH) != 0)
        result.setFlag(TouchWindowTouchType::FineTouch);
    if ((touchFlags & TWF_WANTPALM) != 0)
        result.setFlag(TouchWindowTouchType::WantPalmTouch);
    return result;
}

void QWindowsBaseWindow::hide_sys() // Normal hide, do not activate other windows.
{
    SetWindowPos(handle(), nullptr , 0, 0, 0, 0,
                 SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void QWindowsBaseWindow::raise_sys()
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window();
    const Qt::WindowType type = window()->type();
    if (type == Qt::Popup
        || type == Qt::SubWindow // Special case for QTBUG-63121: MDI subwindows with WindowStaysOnTopHint
        || !(window()->flags() & Qt::WindowStaysOnBottomHint)) {
        SetWindowPos(handle(), HWND_TOP, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }
}

void QWindowsBaseWindow::lower_sys()
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window();
    if (!(window()->flags() & Qt::WindowStaysOnTopHint))
        SetWindowPos(handle(), HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

void QWindowsBaseWindow::setWindowTitle_sys(const QString &title)
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window() << title;
    SetWindowText(handle(), reinterpret_cast<const wchar_t *>(title.utf16()));
}

QPoint QWindowsBaseWindow::mapToGlobal(const QPoint &pos) const
{
    return QWindowsGeometryHint::mapToGlobal(handle(), pos);
}

QPoint QWindowsBaseWindow::mapFromGlobal(const QPoint &pos) const
{
    return QWindowsGeometryHint::mapFromGlobal(handle(), pos);
}

void QWindowsBaseWindow::setHasBorderInFullScreen(bool)
{
    Q_UNIMPLEMENTED();
}

bool QWindowsBaseWindow::hasBorderInFullScreen() const
{
    Q_UNIMPLEMENTED();
    return false;
}

QMargins QWindowsBaseWindow::customMargins() const
{
    return {};
}

void QWindowsBaseWindow::setCustomMargins(const QMargins &)
{
    Q_UNIMPLEMENTED();
}

/*!
    \class QWindowsDesktopWindow
    \brief Window wrapping GetDesktopWindow not allowing any manipulation.
    \since 5.6
    \internal
*/

/*!
    \class QWindowsForeignWindow
    \brief Window wrapping a foreign native window.

    QWindowsForeignWindow stores a native HWND and implements getters for
    geometry, margins, etc. reparenting and geometry manipulation for use as a
    child window in Qt.

    \since 5.6
    \internal
*/

QWindowsForeignWindow::QWindowsForeignWindow(QWindow *window, HWND hwnd)
    : QWindowsBaseWindow(window)
    , m_hwnd(hwnd)
    , m_topLevelStyle(0)
{
}

void QWindowsForeignWindow::setParent(const QPlatformWindow *newParentWindow)
{
    const bool wasTopLevel = isTopLevel_sys();
    const HWND newParent = newParentWindow ? reinterpret_cast<HWND>(newParentWindow->winId()) : HWND(nullptr);
    const bool isTopLevel = !newParent;
    const DWORD oldStyle = style();
    qCDebug(lcQpaWindow) << __FUNCTION__ << window() << "newParent="
        << newParentWindow << newParent << "oldStyle=" << debugWinStyle(oldStyle);
    SetParent(m_hwnd, newParent);
    if (wasTopLevel != isTopLevel) { // Top level window flags need to be set/cleared manually.
        DWORD newStyle = oldStyle;
        if (isTopLevel) {
            newStyle = m_topLevelStyle;
        } else {
            m_topLevelStyle = oldStyle;
            newStyle &= ~(WS_OVERLAPPEDWINDOW | WS_POPUPWINDOW);
            newStyle |= WS_CHILD;
        }
        SetWindowLongPtr(m_hwnd, GWL_STYLE, newStyle);
    }
}

void QWindowsForeignWindow::setVisible(bool visible)
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << window() << visible;
    if (visible)
        ShowWindow(handle(), SW_SHOWNOACTIVATE);
    else
        hide_sys();
}

/*!
    \class QWindowCreationContext
    \brief Active Context for creating windows.

    There is a phase in window creation (WindowCreationData::create())
    in which events are sent before the system API CreateWindowEx() returns
    the handle. These cannot be handled by the platform window as the association
    of the unknown handle value to the window does not exist yet and as not
    to trigger recursive handle creation, etc.

    In that phase, an instance of  QWindowCreationContext is set on
    QWindowsContext.

    QWindowCreationContext stores the information to answer the initial
    WM_GETMINMAXINFO and obtains the corrected size/position.

    \sa WindowCreationData, QWindowsContext
    \internal
*/

QWindowCreationContext::QWindowCreationContext(const QWindow *w, const QScreen *s,
                                               const QRect &geometryIn, const QRect &geometry,
                                               const QMargins &cm,
                                               DWORD style, DWORD exStyle) :
    window(w),
    screen(s),
    requestedGeometryIn(geometryIn),
    requestedGeometry(geometry),
    obtainedPos(geometryIn.topLeft()),
    obtainedSize(geometryIn.size()),
    margins(QWindowsGeometryHint::frame(w, geometry, style, exStyle))
{
    // Geometry of toplevels does not consider window frames.
    // TODO: No concept of WA_wasMoved yet that would indicate a
    // CW_USEDEFAULT unless set. For now, assume that 0,0 means 'default'
    // for toplevels.

    if (!(w->flags() & Qt::FramelessWindowHint))
        customMargins = cm;

    if (geometry.isValid()
        || !qt_window_private(const_cast<QWindow *>(w))->resizeAutomatic) {
        frameX = geometry.x();
        frameY = geometry.y();
        const QMargins effectiveMargins = margins + customMargins;
        frameWidth = effectiveMargins.left() + geometry.width() + effectiveMargins.right();
        frameHeight = effectiveMargins.top() + geometry.height() + effectiveMargins.bottom();
        if (QWindowsMenuBar::menuBarOf(w) != nullptr) {
            menuHeight = GetSystemMetrics(SM_CYMENU);
            frameHeight += menuHeight;
        }
        const bool isDefaultPosition = !frameX && !frameY && w->isTopLevel();
        if (!QWindowsGeometryHint::positionIncludesFrame(w) && !isDefaultPosition) {
            frameX -= effectiveMargins.left();
            frameY -= effectiveMargins.top();
        }
    }

    qCDebug(lcQpaWindow).nospace()
        << __FUNCTION__ << ' ' << w << ' ' << geometry
        << " pos incl. frame=" << QWindowsGeometryHint::positionIncludesFrame(w)
        << " frame=" << frameWidth << 'x' << frameHeight << '+'
        << frameX << '+' << frameY
        << " margins=" << margins << " custom margins=" << customMargins;
}

void QWindowCreationContext::applyToMinMaxInfo(MINMAXINFO *mmi) const
{
    QWindowsGeometryHint::applyToMinMaxInfo(window, screen, margins + customMargins, mmi);
}

/*!
    \class QWindowsWindow
    \brief Raster or OpenGL Window.

    \list
    \li Raster type: handleWmPaint() is implemented to
       to bitblt the image. The DC can be accessed
       via getDC/releaseDC, which has special handling
       when within a paint event (in that case, the DC obtained
       from BeginPaint() is returned).

    \li Open GL: The first time QWindowsGLContext accesses
       the handle, it sets up the pixelformat on the DC
       which in turn sets it on the window (see flag
       PixelFormatInitialized).
       handleWmPaint() is empty (although required).
    \endlist

    \internal
*/

const char *QWindowsWindow::embeddedNativeParentHandleProperty = "_q_embedded_native_parent_handle";
const char *QWindowsWindow::hasBorderInFullScreenProperty = "_q_has_border_in_fullscreen";
bool QWindowsWindow::m_borderInFullScreenDefault = false;
bool QWindowsWindow::m_inSetgeometry = false;

QWindowsWindow::QWindowsWindow(QWindow *aWindow, const QWindowsWindowData &data) :
    QWindowsBaseWindow(aWindow),
    m_data(data),
    m_cursor(new CursorHandle)
#if QT_CONFIG(vulkan)
  , m_vkSurface(VK_NULL_HANDLE)
#endif
{
    QWindowsContext::instance()->addWindow(m_data.hwnd, this);
    const Qt::WindowType type = aWindow->type();
    if (type == Qt::Desktop)
        return; // No further handling for Qt::Desktop
    if (aWindow->surfaceType() == QWindow::Direct3DSurface)
        setFlag(Direct3DSurface);
#if QT_CONFIG(opengl)
    if (aWindow->surfaceType() == QWindow::OpenGLSurface)
        setFlag(OpenGLSurface);
#endif
#if QT_CONFIG(vulkan)
    if (aWindow->surfaceType() == QSurface::VulkanSurface)
        setFlag(VulkanSurface);
#endif
    updateDropSite(window()->isTopLevel());

    // Register touch unless if the flags are already set by a hook
    // such as HCBT_CREATEWND
    if (!touchWindowTouchTypes_sys().has_value())
        registerTouchWindow();

    const qreal opacity = qt_window_private(aWindow)->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);

    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));

    if (aWindow->isTopLevel())
        setWindowIcon(aWindow->icon());
    if (m_borderInFullScreenDefault || aWindow->property(hasBorderInFullScreenProperty).toBool())
        setFlag(HasBorderInFullScreen);
    clearFlag(WithinCreate);
}

QWindowsWindow::~QWindowsWindow()
{
    setFlag(WithinDestroy);
    if (testFlag(TouchRegistered))
        UnregisterTouchWindow(m_data.hwnd);
    destroyWindow();
    destroyIcon();
}

void QWindowsWindow::initialize()
{
    // Clear the creation context as the window can be found in QWindowsContext's map.
    QWindowCreationContextPtr creationContext =
        QWindowsContext::instance()->setWindowCreationContext(QWindowCreationContextPtr());

    QWindow *w = window();
    setWindowState(w->windowStates());

    // Trigger geometry change (unless it has a special state in which case setWindowState()
    // will send the message) and screen change signals of QWindow.
    if (w->type() != Qt::Desktop) {
        const Qt::WindowState state = w->windowState();
        const QRect obtainedGeometry(creationContext->obtainedPos, creationContext->obtainedSize);
        QPlatformScreen *obtainedScreen = screenForGeometry(obtainedGeometry);
        if (obtainedScreen && screen() != obtainedScreen)
            QWindowSystemInterface::handleWindowScreenChanged<QWindowSystemInterface::SynchronousDelivery>(w, obtainedScreen->screen());
        if (state != Qt::WindowMaximized && state != Qt::WindowFullScreen
            && creationContext->requestedGeometryIn != obtainedGeometry) {
            QWindowSystemInterface::handleGeometryChange<QWindowSystemInterface::SynchronousDelivery>(w, obtainedGeometry);
        }
    }
    QWindowsWindow::setSavedDpi(GetDpiForWindow(handle()));
}

QSurfaceFormat QWindowsWindow::format() const
{
    return window()->requestedFormat();
}

void QWindowsWindow::fireExpose(const QRegion &region, bool force)
{
    if (region.isEmpty() && !force)
        clearFlag(Exposed);
    else
        setFlag(Exposed);
    QWindowSystemInterface::handleExposeEvent(window(), region);
}

void QWindowsWindow::fireFullExpose(bool force)
{
    fireExpose(QRect(QPoint(0, 0), m_data.geometry.size()), force);
}

void QWindowsWindow::destroyWindow()
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window() << m_data.hwnd;
    if (m_data.hwnd) { // Stop event dispatching before Window is destroyed.
        setFlag(WithinDestroy);
        // Clear any transient child relationships as Windows will otherwise destroy them (QTBUG-35499, QTBUG-36666)
        const auto tlw = QGuiApplication::topLevelWindows();
        for (QWindow *w : tlw) {
            if (w->transientParent() == window()) {
                if (QWindowsWindow *tw = QWindowsWindow::windowsWindowOf(w))
                    tw->updateTransientParent();
            }
        }
        QWindowsContext *context = QWindowsContext::instance();
        if (context->windowUnderMouse() == window())
            context->clearWindowUnderMouse();
        if (hasMouseCapture())
            setMouseGrabEnabled(false);
        setDropSiteEnabled(false);
#if QT_CONFIG(vulkan)
        if (m_vkSurface) {
            QVulkanInstance *inst = window()->vulkanInstance();
            if (inst)
                static_cast<QWindowsVulkanInstance *>(inst->handle())->destroySurface(m_vkSurface);
            m_vkSurface = VK_NULL_HANDLE;
        }
#endif
#ifndef QT_NO_OPENGL
        if (m_surface) {
            if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
                staticOpenGLContext->destroyWindowSurface(m_surface);
            m_surface = nullptr;
        }
#endif
        DestroyWindow(m_data.hwnd);
        context->removeWindow(m_data.hwnd);
        m_data.hwnd = nullptr;
    }
}

void QWindowsWindow::updateDropSite(bool topLevel)
{
    bool enabled = false;
    bool parentIsEmbedded = false;

    if (!topLevel) {
        // if the parent window is a foreign window wrapped via QWindow::fromWinId, we need to enable the drop site
        // on the first child window
        const QWindow *parent = window()->parent();
        if (parent && parent->handle() && parent->handle()->isForeignWindow())
            parentIsEmbedded = true;
    }

    if (topLevel || parentIsEmbedded) {
        switch (window()->type()) {
        case Qt::Window:
        case Qt::Dialog:
        case Qt::Sheet:
        case Qt::Drawer:
        case Qt::Popup:
        case Qt::Tool:
            enabled = true;
            break;
        default:
            break;
        }
    }
    setDropSiteEnabled(enabled);
}

void QWindowsWindow::setDropSiteEnabled(bool dropEnabled)
{
    if (isDropSiteEnabled() == dropEnabled)
        return;
    qCDebug(lcQpaMime) << __FUNCTION__ << window() << dropEnabled;
#if QT_CONFIG(clipboard) && QT_CONFIG(draganddrop)
    if (dropEnabled) {
        Q_ASSERT(m_data.hwnd);
        m_dropTarget = new QWindowsOleDropTarget(window());
        RegisterDragDrop(m_data.hwnd, m_dropTarget);
        CoLockObjectExternal(m_dropTarget, true, true);
    } else {
        CoLockObjectExternal(m_dropTarget, false, true);
        m_dropTarget->Release();
        RevokeDragDrop(m_data.hwnd);
        m_dropTarget = nullptr;
    }
#endif // QT_CONFIG(clipboard) && QT_CONFIG(draganddrop)
}

bool QWindowsWindow::m_screenForGLInitialized = false;

void QWindowsWindow::displayChanged()
{
    m_screenForGLInitialized = false;
}

void QWindowsWindow::settingsChanged()
{
    m_screenForGLInitialized = false;
}

QScreen *QWindowsWindow::forcedScreenForGLWindow(const QWindow *w)
{
    static QString forceToScreen;
    if (!m_screenForGLInitialized) {
        forceToScreen = GpuDescription::detect().gpuSuitableScreen;
        m_screenForGLInitialized = true;
    }
    return forceToScreen.isEmpty() ? nullptr : screenForDeviceName(w, forceToScreen);
}

// Returns topmost QWindowsWindow ancestor even if there are embedded windows in the chain.
// Returns this window if it is the topmost ancestor.
QWindow *QWindowsWindow::topLevelOf(QWindow *w)
{
    while (QWindow *parent = w->parent())
        w = parent;

    if (const QPlatformWindow *handle = w->handle()) {
        const auto *ww = static_cast<const QWindowsWindow *>(handle);
        if (ww->isEmbedded()) {
            HWND parentHWND = GetAncestor(ww->handle(), GA_PARENT);
            const HWND desktopHwnd = GetDesktopWindow();
            const QWindowsContext *ctx = QWindowsContext::instance();
            while (parentHWND && parentHWND != desktopHwnd) {
                if (QWindowsWindow *ancestor = ctx->findPlatformWindow(parentHWND))
                    return topLevelOf(ancestor->window());
                parentHWND = GetAncestor(parentHWND, GA_PARENT);
            }
        }
    }
    return w;
}

QWindowsWindowData
    QWindowsWindowData::create(const QWindow *w,
                                       const QWindowsWindowData &parameters,
                                       const QString &title)
{
    WindowCreationData creationData;
    creationData.fromWindow(w, parameters.flags);
    QWindowsWindowData result = creationData.create(w, parameters, title);
    // Force WM_NCCALCSIZE (with wParam=1) via SWP_FRAMECHANGED for custom margin.
    creationData.initialize(w, result.hwnd, !parameters.customMargins.isNull(), 1);
    return result;
}

void QWindowsWindow::setVisible(bool visible)
{
    const QWindow *win = window();
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << win << m_data.hwnd << visible;
    if (m_data.hwnd) {
        if (visible) {
            show_sys();

            // When the window is layered, we won't get WM_PAINT, and "we" are in control
            // over the rendering of the window
            // There is nobody waiting for this, so we don't need to flush afterwards.
            if (isLayered())
                fireFullExpose();
            // QTBUG-44928, QTBUG-7386: This is to resolve the problem where popups are
            // opened from the system tray and not being implicitly activated

            if (win->type() == Qt::Popup && !win->parent() && !QGuiApplication::focusWindow())
                SetForegroundWindow(m_data.hwnd);
        } else {
            if (hasMouseCapture())
                setMouseGrabEnabled(false);
            if (window()->flags() & Qt::Popup) // from QWidgetPrivate::hide_sys(), activate other
                ShowWindow(m_data.hwnd, SW_HIDE);
            else
                hide_sys();
            fireExpose(QRegion());
        }
    }
}

bool QWindowsWindow::isVisible() const
{
    return m_data.hwnd && IsWindowVisible(m_data.hwnd);
}

bool QWindowsWindow::isActive() const
{
    // Check for native windows or children of the active native window.
    if (const HWND activeHwnd = GetForegroundWindow())
        if (m_data.hwnd == activeHwnd || IsChild(activeHwnd, m_data.hwnd))
            return true;
    return false;
}

bool QWindowsWindow::isAncestorOf(const QPlatformWindow *child) const
{
    const auto *childWindow = static_cast<const QWindowsWindow *>(child);
    return IsChild(m_data.hwnd, childWindow->handle());
}

bool QWindowsWindow::isEmbedded() const
{
    return m_data.embedded;
}

QPoint QWindowsWindow::mapToGlobal(const QPoint &pos) const
{
    return m_data.hwnd ? QWindowsGeometryHint::mapToGlobal(m_data.hwnd, pos) : pos;
}

QPoint QWindowsWindow::mapFromGlobal(const QPoint &pos) const
{
    return m_data.hwnd ? QWindowsGeometryHint::mapFromGlobal(m_data.hwnd, pos) : pos;
}

// Update the transient parent for a toplevel window. The concept does not
// really exist on Windows, the relationship is set by passing a parent along with !WS_CHILD
// to window creation or by setting the parent using  GWL_HWNDPARENT (as opposed to
// SetParent, which would make it a real child).

#ifndef GWL_HWNDPARENT
#    define GWL_HWNDPARENT (-8)
#endif

void QWindowsWindow::updateTransientParent() const
{
    if (window()->type() == Qt::Popup)
        return; // QTBUG-34503, // a popup stays on top, no parent, see also WindowCreationData::fromWindow().
    // Update transient parent.
    const HWND oldTransientParent = GetWindow(m_data.hwnd, GW_OWNER);
    HWND newTransientParent = nullptr;
    if (const QWindow *tp = window()->transientParent())
        if (const QWindowsWindow *tw = QWindowsWindow::windowsWindowOf(tp))
            if (!tw->testFlag(WithinDestroy)) // Prevent destruction by parent window (QTBUG-35499, QTBUG-36666)
                newTransientParent = tw->handle();

    // QTSOLBUG-71: When using the MFC/winmigrate solution, it is possible that a child
    // window is found, which can cause issues with modality. Loop up to top level.
    while (newTransientParent && (GetWindowLongPtr(newTransientParent, GWL_STYLE) & WS_CHILD) != 0)
        newTransientParent = GetParent(newTransientParent);

    if (newTransientParent != oldTransientParent)
        SetWindowLongPtr(m_data.hwnd, GWL_HWNDPARENT, LONG_PTR(newTransientParent));
}

static inline bool testShowWithoutActivating(const QWindow *window)
{
    // QWidget-attribute Qt::WA_ShowWithoutActivating .
    const QVariant showWithoutActivating = window->property("_q_showWithoutActivating");
    return showWithoutActivating.isValid() && showWithoutActivating.toBool();
}

static void setMinimizedGeometry(HWND hwnd, const QRect &r)
{
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &windowPlacement)) {
        windowPlacement.showCmd = SW_SHOWMINIMIZED;
        windowPlacement.rcNormalPosition = RECTfromQRect(r);
        SetWindowPlacement(hwnd, &windowPlacement);
    }
}

static void setRestoreMaximizedFlag(HWND hwnd, bool set = true)
{
    // Let Windows know that we need to restore as maximized
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &windowPlacement)) {
        if (set)
            windowPlacement.flags |= WPF_RESTORETOMAXIMIZED;
        else
            windowPlacement.flags &= ~WPF_RESTORETOMAXIMIZED;
        SetWindowPlacement(hwnd, &windowPlacement);
    }
}

// partially from QWidgetPrivate::show_sys()
void QWindowsWindow::show_sys() const
{
    int sm = SW_SHOWNORMAL;
    bool fakedMaximize = false;
    bool restoreMaximize = false;
    const QWindow *w = window();
    const Qt::WindowFlags flags = w->flags();
    const Qt::WindowType type = w->type();
    if (w->isTopLevel()) {
        const Qt::WindowStates state = w->windowStates();
        if (state & Qt::WindowMinimized) {
            sm = SW_SHOWMINIMIZED;
            if (!isVisible())
                sm = SW_SHOWMINNOACTIVE;
            if (state & Qt::WindowMaximized)
                restoreMaximize = true;
        } else {
            updateTransientParent();
            if (state & Qt::WindowMaximized) {
                sm = SW_SHOWMAXIMIZED;
                // Windows will not behave correctly when we try to maximize a window which does not
                // have minimize nor maximize buttons in the window frame. Windows would then ignore
                // non-available geometry, and rather maximize the widget to the full screen, minus the
                // window frame (caption). So, we do a trick here, by adding a maximize button before
                // maximizing the widget, and then remove the maximize button afterwards.
                if (flags & Qt::WindowTitleHint &&
                        !(flags & (Qt::WindowMinMaxButtonsHint | Qt::FramelessWindowHint))) {
                    fakedMaximize = TRUE;
                    setStyle(style() | WS_MAXIMIZEBOX);
                }
            } // Qt::WindowMaximized
        } // !Qt::WindowMinimized
    }
    if (type == Qt::Popup || type == Qt::ToolTip || type == Qt::Tool || testShowWithoutActivating(w))
        sm = SW_SHOWNOACTIVATE;

    if (w->windowStates() & Qt::WindowMaximized)
        setFlag(WithinMaximize); // QTBUG-8361

    ShowWindow(m_data.hwnd, sm);

    clearFlag(WithinMaximize);

    if (fakedMaximize) {
        setStyle(style() & ~WS_MAXIMIZEBOX);
        SetWindowPos(m_data.hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER
                     | SWP_FRAMECHANGED);
    }
    if (restoreMaximize)
        setRestoreMaximizedFlag(m_data.hwnd);
}

void QWindowsWindow::setParent(const QPlatformWindow *newParent)
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << window() << newParent;

    if (m_data.hwnd)
        setParent_sys(newParent);
}

void QWindowsWindow::setParent_sys(const QPlatformWindow *parent)
{
    // Use GetAncestor instead of GetParent, as GetParent can return owner window for toplevels
    HWND oldParentHWND = parentHwnd();
    HWND newParentHWND = nullptr;
    if (parent) {
        const auto *parentW = static_cast<const QWindowsWindow *>(parent);
        newParentHWND = parentW->handle();

    }

    // NULL handle means desktop window, which also has its proper handle -> disambiguate
    HWND desktopHwnd = GetDesktopWindow();
    if (oldParentHWND == desktopHwnd)
        oldParentHWND = nullptr;
    if (newParentHWND == desktopHwnd)
        newParentHWND = nullptr;

    if (newParentHWND != oldParentHWND) {
        const bool wasTopLevel = oldParentHWND == nullptr;
        const bool isTopLevel = newParentHWND == nullptr;

        setFlag(WithinSetParent);
        SetParent(m_data.hwnd, newParentHWND);
        clearFlag(WithinSetParent);

        // WS_CHILD/WS_POPUP must be manually set/cleared in addition
        // to dialog frames, etc (see  SetParent() ) if the top level state changes.
        // Force toplevel state as QWindow::isTopLevel cannot be relied upon here.
        if (wasTopLevel != isTopLevel) {
            setDropSiteEnabled(false);
            setWindowFlags_sys(window()->flags(), unsigned(isTopLevel ? WindowCreationData::ForceTopLevel : WindowCreationData::ForceChild));
            updateDropSite(isTopLevel);
        }
    }
}

void QWindowsWindow::handleHidden()
{
    fireExpose(QRegion());
}

void QWindowsWindow::handleCompositionSettingsChanged()
{
    const QWindow *w = window();
    if ((w->surfaceType() == QWindow::OpenGLSurface
         || w->surfaceType() == QWindow::VulkanSurface
         || w->surfaceType() == QWindow::Direct3DSurface)
        && w->format().hasAlpha())
    {
        applyBlurBehindWindow(handle());
    }
}

qreal QWindowsWindow::dpiRelativeScale(const UINT dpi) const
{
    return QHighDpiScaling::roundScaleFactor(qreal(dpi) / QWindowsScreen::baseDpi) /
           QHighDpiScaling::roundScaleFactor(qreal(savedDpi()) / QWindowsScreen::baseDpi);
}

void QWindowsWindow::handleDpiScaledSize(WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    // We want to keep QWindow's device independent size constant across the
    // DPI change. To accomplish this, scale QPlatformWindow's native size
    // by the change of DPI (e.g. 120 -> 144 = 1.2), also taking any scale
    // factor rounding into account. The win32 window size includes the margins;
    // add the margins for the new DPI to the window size.
    const UINT dpi = UINT(wParam);
    const qreal scale = dpiRelativeScale(dpi);
    const QMargins margins = fullFrameMargins();
    if (!(m_data.flags & Qt::FramelessWindowHint)) {
        // We need to update the custom margins to match the current DPI, because
        // we don't want our users manually hook into this message just to set a
        // new margin, but here we can't call setCustomMargins() directly, that
        // function will change the window geometry which conflicts with what we
        // are currently doing.
        m_data.customMargins *= scale;
    }

    const QSize windowSize = (geometry().size() * scale).grownBy((margins * scale) + customMargins());
    SIZE *size = reinterpret_cast<SIZE *>(lParam);
    size->cx = windowSize.width();
    size->cy = windowSize.height();
    *result = true; // Inform Windows that we've set a size
}

void QWindowsWindow::handleDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    const UINT dpi = HIWORD(wParam);
    const qreal scale = dpiRelativeScale(dpi);
    setSavedDpi(dpi);
    // Send screen change first, so that the new screen is set during any following resize
    checkForScreenChanged(QWindowsWindow::FromDpiChange);

    if (!IsZoomed(hwnd))
        m_data.restoreGeometry.setSize(m_data.restoreGeometry.size() * scale);

    // We get WM_DPICHANGED in one of two situations:
    //
    // 1. The DPI change is a "spontaneous" DPI change as a result of e.g.
    // the user dragging the window to a new screen. In this case Windows
    // first sends WM_GETDPISCALEDSIZE, where we set the new window size,
    // followed by this event where we apply the suggested window geometry
    // to the native window. This will make sure the window tracks the mouse
    // cursor during screen change, and also that the window size is scaled
    // according to the DPI change.
    //
    // 2. The DPI change is a result of a setGeometry() call. In this case
    // Qt has already scaled the window size for the new DPI. Further, Windows
    // does not call WM_GETDPISCALEDSIZE, and also applies its own scaling
    // to the already scaled window size. Since there is no need to set the
    // window geometry again, and the provided geometry is incorrect, we omit
    // making the SetWindowPos() call.
    if (!m_inSetgeometry) {
        updateFullFrameMargins();
        const auto prcNewWindow = reinterpret_cast<RECT *>(lParam);
        SetWindowPos(hwnd, nullptr, prcNewWindow->left, prcNewWindow->top,
                     prcNewWindow->right - prcNewWindow->left,
                     prcNewWindow->bottom - prcNewWindow->top, SWP_NOZORDER | SWP_NOACTIVATE);
        // If the window does not have a frame, WM_MOVE and WM_SIZE won't be
        // called which prevents the content from being scaled appropriately
        // after a DPI change.
        if (m_data.flags & Qt::FramelessWindowHint)
            handleGeometryChange();
    }

    // Re-apply mask now that we have a new DPI, which have resulted in
    // a new scale factor.
    setMask(QHighDpi::toNativeLocalRegion(window()->mask(), window()));
}

void QWindowsWindow::handleDpiChangedAfterParent(HWND hwnd)
{
    const UINT dpi = GetDpiForWindow(hwnd);
    const qreal scale = dpiRelativeScale(dpi);
    setSavedDpi(dpi);

    checkForScreenChanged(QWindowsWindow::FromDpiChange);

    // Child windows do not get WM_GETDPISCALEDSIZE messages to inform
    // Windows about the new size, so we need to manually scale them.
    QRect currentGeometry = geometry();
    QRect scaledGeometry = QRect(currentGeometry.topLeft() * scale, currentGeometry.size() * scale);
    setGeometry(scaledGeometry);
}

static QRect normalFrameGeometry(HWND hwnd)
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &wp)) {
        const QRect result = qrectFromRECT(wp.rcNormalPosition);
        return result.translated(windowPlacementOffset(hwnd, result.topLeft()));
    }
    return QRect();
}

QRect QWindowsWindow::normalGeometry() const
{
    // Check for fake 'fullscreen' mode.
    const bool fakeFullScreen =
        m_savedFrameGeometry.isValid() && (window()->windowStates() & Qt::WindowFullScreen);
    const QRect frame = fakeFullScreen ? m_savedFrameGeometry : normalFrameGeometry(m_data.hwnd);
    const QMargins margins = fakeFullScreen
        ? QWindowsGeometryHint::frame(window(), handle(), m_savedStyle, 0)
        : fullFrameMargins();
    return frame.isValid() ? frame.marginsRemoved(margins) : frame;
}

static QString msgUnableToSetGeometry(const QWindowsWindow *platformWindow,
                                      const QRect &requestedRect,
                                      const QRect &obtainedRect,
                                      const QMargins &fullMargins,
                                      const QMargins &customMargins)
{
    QString result;
    QDebug debug(&result);
    debug.nospace();
    debug.noquote();
    const auto window = platformWindow->window();
    debug << "Unable to set geometry ";
    formatBriefRectangle(debug, requestedRect);
    debug << " (frame: ";
    formatBriefRectangle(debug, requestedRect + fullMargins);
    debug << ") on " << window->metaObject()->className() << "/\""
          << window->objectName() << "\" on \"" << window->screen()->name()
          << "\". Resulting geometry: ";
    formatBriefRectangle(debug, obtainedRect);
    debug << " (frame: ";
    formatBriefRectangle(debug, obtainedRect + fullMargins);
    debug << ") margins: ";
    formatBriefMargins(debug, fullMargins);
    if (!customMargins.isNull()) {
       debug << " custom margin: ";
       formatBriefMargins(debug, customMargins);
    }
    const auto minimumSize = window->minimumSize();
    const bool hasMinimumSize = !minimumSize.isEmpty();
    if (hasMinimumSize)
        debug << " minimum size: " << minimumSize.width() << 'x' << minimumSize.height();
    const auto maximumSize = window->maximumSize();
    const bool hasMaximumSize = maximumSize.width() != QWINDOWSIZE_MAX || maximumSize.height() != QWINDOWSIZE_MAX;
    if (hasMaximumSize)
        debug << " maximum size: " << maximumSize.width() << 'x' << maximumSize.height();
    if (hasMinimumSize || hasMaximumSize) {
        MINMAXINFO minmaxInfo;
        memset(&minmaxInfo, 0, sizeof(minmaxInfo));
        platformWindow->getSizeHints(&minmaxInfo);
        debug << ' ' << minmaxInfo;
    }
    debug << ')';
    return result;
}

void QWindowsWindow::setGeometry(const QRect &rectIn)
{
    QBoolBlocker b(m_inSetgeometry);

    QRect rect = rectIn;
    // This means it is a call from QWindow::setFramePosition() and
    // the coordinates include the frame (size is still the contents rectangle).
    if (QWindowsGeometryHint::positionIncludesFrame(window())) {
        const QMargins margins = frameMargins();
        rect.moveTopLeft(rect.topLeft() + QPoint(margins.left(), margins.top()));
    }

    if (m_windowState & Qt::WindowMinimized)
        m_data.geometry = rect; // Otherwise set by handleGeometryChange() triggered by event.
    else
        setWindowState(Qt::WindowNoState);// Update window state to WindowNoState unless minimized

    if (m_data.hwnd) {
        // A ResizeEvent with resulting geometry will be sent. If we cannot
        // achieve that size (for example, window title minimal constraint),
        // notify and warn.
        setFlag(WithinSetGeometry);
        setGeometry_sys(rect);
        clearFlag(WithinSetGeometry);
        if (m_data.geometry != rect && (isVisible() || QLibraryInfo::isDebugBuild())) {
            const auto warning =
                msgUnableToSetGeometry(this, rectIn, m_data.geometry,
                                       fullFrameMargins(), customMargins());
            qWarning("%s: %s", __FUNCTION__, qPrintable(warning));
        }
    } else {
        QPlatformWindow::setGeometry(rect);
    }
}

void QWindowsWindow::handleMoved()
{
    // Minimize/Set parent can send nonsensical move events.
    if (!IsIconic(m_data.hwnd) && !testFlag(WithinSetParent))
        handleGeometryChange();
}

void QWindowsWindow::handleResized(int wParam, LPARAM lParam)
{
    /* Prevents borderless windows from covering the taskbar when maximized. */
    if ((m_data.flags.testFlag(Qt::FramelessWindowHint)
         || (m_data.flags.testFlag(Qt::CustomizeWindowHint) && !m_data.flags.testFlag(Qt::WindowTitleHint)))
        && IsZoomed(m_data.hwnd)) {
        const int resizedWidth = LOWORD(lParam);
        const int resizedHeight = HIWORD(lParam);

        const HMONITOR monitor = MonitorFromWindow(m_data.hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFO);
        GetMonitorInfoW(monitor, &monitorInfo);

        int correctLeft = monitorInfo.rcMonitor.left;
        int correctTop = monitorInfo.rcMonitor.top;
        int correctWidth = monitorInfo.rcWork.right - monitorInfo.rcWork.left;
        int correctHeight = monitorInfo.rcWork.bottom - monitorInfo.rcWork.top;

        if (!m_data.flags.testFlag(Qt::FramelessWindowHint)) {
            const int borderWidth = invisibleMargins(m_data.hwnd).left();
            correctLeft -= borderWidth;
            correctTop -= borderWidth;
            correctWidth += borderWidth * 2;
            correctHeight += borderWidth * 2;
        }

        if (resizedWidth != correctWidth || resizedHeight != correctHeight) {
            qCDebug(lcQpaWindow) << __FUNCTION__ << "correcting: " << resizedWidth << "x"
                                 << resizedHeight << " -> " << correctWidth << "x" << correctHeight;
            SetWindowPos(m_data.hwnd, nullptr, correctLeft, correctTop, correctWidth, correctHeight,
                         SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    switch (wParam) {
    case SIZE_MAXHIDE: // Some other window affected.
    case SIZE_MAXSHOW:
        return;
    case SIZE_MINIMIZED: // QTBUG-53577, prevent state change events during programmatic state change
        if (!testFlag(WithinSetStyle) && !testFlag(WithinSetGeometry))
            handleWindowStateChange(m_windowState | Qt::WindowMinimized);
        return;
    case SIZE_MAXIMIZED:
        handleGeometryChange();
        if (!testFlag(WithinSetStyle) && !testFlag(WithinSetGeometry))
            handleWindowStateChange(Qt::WindowMaximized | (isFullScreen_sys() ? Qt::WindowFullScreen
                                                                              : Qt::WindowNoState));
        break;
    case SIZE_RESTORED:
        handleGeometryChange();
        if (!testFlag(WithinSetStyle) && !testFlag(WithinSetGeometry)) {
            if (isFullScreen_sys())
                handleWindowStateChange(
                    Qt::WindowFullScreen
                    | (testFlag(MaximizeToFullScreen) ? Qt::WindowMaximized : Qt::WindowNoState));
            else if (m_windowState != Qt::WindowNoState && !testFlag(MaximizeToFullScreen))
                handleWindowStateChange(Qt::WindowNoState);
        }
        break;
    }
}

static inline bool equalDpi(const QDpi &d1, const QDpi &d2)
{
    return qFuzzyCompare(d1.first, d2.first) && qFuzzyCompare(d1.second, d2.second);
}

void QWindowsWindow::checkForScreenChanged(ScreenChangeMode mode)
{
    if ((parent() && !parent()->isForeignWindow()) || QWindowsScreenManager::isSingleScreen())
        return;

    QPlatformScreen *currentScreen = screen();
    auto topLevel = isTopLevel_sys() ? m_data.hwnd : GetAncestor(m_data.hwnd, GA_ROOT);
    const QWindowsScreen *newScreen =
        QWindowsContext::instance()->screenManager().screenForHwnd(topLevel);

    if (newScreen == nullptr || newScreen == currentScreen)
        return;
    // For screens with different DPI: postpone until WM_DPICHANGE
    // Check on currentScreen as it can be 0 when resuming a session (QTBUG-80436).
    const bool changingDpi = !equalDpi(QDpi(savedDpi(), savedDpi()), newScreen->logicalDpi());
    if (mode == FromGeometryChange && currentScreen != nullptr && changingDpi)
        return;

    qCDebug(lcQpaWindow).noquote().nospace() << __FUNCTION__
        << ' ' << window() << " \"" << (currentScreen ? currentScreen->name() : QString())
        << "\"->\"" << newScreen->name() << '"';
    updateFullFrameMargins();
    QWindowSystemInterface::handleWindowScreenChanged<QWindowSystemInterface::SynchronousDelivery>(window(), newScreen->screen());
}

void QWindowsWindow::handleGeometryChange()
{
    const QRect previousGeometry = m_data.geometry;
    m_data.geometry = geometry_sys();
    updateFullFrameMargins();
    QWindowSystemInterface::handleGeometryChange(window(), m_data.geometry);
    // QTBUG-32121: OpenGL/normal windows (with exception of ANGLE
    // which we no longer support in Qt 6) do not receive expose
    // events when shrinking, synthesize.
    if (isExposed()
        && m_data.geometry.size() != previousGeometry.size() // Exclude plain move
        // One dimension grew -> Windows will send expose, no need to synthesize.
        && !(m_data.geometry.width() > previousGeometry.width() || m_data.geometry.height() > previousGeometry.height())) {
        fireFullExpose(true);
    }

    const bool wasSync = testFlag(SynchronousGeometryChangeEvent);
    checkForScreenChanged();

    if (testFlag(SynchronousGeometryChangeEvent))
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

    if (!testFlag(ResizeMoveActive))
        updateRestoreGeometry();

    if (!wasSync)
        clearFlag(SynchronousGeometryChangeEvent);
    qCDebug(lcQpaEvents) << __FUNCTION__ << this << window() << m_data.geometry;
}

void QWindowsBaseWindow::setGeometry_sys(const QRect &rect) const
{
    const QMargins margins = fullFrameMargins();
    const QRect frameGeometry = rect + margins;

    qCDebug(lcQpaWindow) << '>' << __FUNCTION__ << window()
        << "\n from " << geometry_sys() << " frame: "
        << margins << " to " <<rect
        << " new frame: " << frameGeometry;

    bool result = false;
    const HWND hwnd = handle();
    WINDOWPLACEMENT windowPlacement;
    windowPlacement.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &windowPlacement);
    // If the window is hidden and in maximized state or minimized, instead of moving the
    // window, set the normal position of the window.
    if ((windowPlacement.showCmd == SW_MAXIMIZE && !IsWindowVisible(hwnd))
        || windowPlacement.showCmd == SW_SHOWMINIMIZED) {
        windowPlacement.rcNormalPosition =
            RECTfromQRect(frameGeometry.translated(-windowPlacementOffset(hwnd, frameGeometry.topLeft())));
        windowPlacement.showCmd = windowPlacement.showCmd == SW_SHOWMINIMIZED ? SW_SHOWMINIMIZED : SW_HIDE;
        result = SetWindowPlacement(hwnd, &windowPlacement);
    } else {
        int x = frameGeometry.x();
        if (!window()->isTopLevel()) {
            const HWND parentHandle = GetParent(hwnd);
            if (isRtlLayout(parentHandle)) {
                RECT rect;
                GetClientRect(parentHandle, &rect);
                x = rect.right - frameGeometry.width() - x;
            }
        }
        result = MoveWindow(hwnd, x, frameGeometry.y(),
                            frameGeometry.width(), frameGeometry.height(), true);
    }
    qCDebug(lcQpaWindow) << '<' << __FUNCTION__ << window()
        << "\n resulting " << result << geometry_sys();
}

/*!
    Allocates a HDC for the window or returns the temporary one
    obtained from WinAPI BeginPaint within a WM_PAINT event.

    \sa releaseDC()
*/

HDC QWindowsWindow::getDC()
{
    if (!m_hdc) {
        m_hdc = GetDC(handle());
        if (QGuiApplication::layoutDirection() == Qt::RightToLeft)
            SetLayout(m_hdc, 0); // Clear RTL layout
    }
    return m_hdc;
}

/*!
    Releases the HDC for the window or does nothing in
    case it was obtained from WinAPI BeginPaint within a WM_PAINT event.

    \sa getDC()
*/

void QWindowsWindow::releaseDC()
{
    if (m_hdc) {
        ReleaseDC(handle(), m_hdc);
        m_hdc = nullptr;
    }
}

static inline bool isSoftwareGl()
{
#if QT_CONFIG(dynamicgl)
    return QOpenGLStaticContext::opengl32.moduleIsNotOpengl32()
        && QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL;
#else
    return false;
#endif // dynamicgl
}

bool QWindowsWindow::handleWmPaint(HWND hwnd, UINT message,
                                         WPARAM, LPARAM, LRESULT *result)
{
    if (message == WM_ERASEBKGND) { // Backing store - ignored.
        *result = 1;
        return true;
    }
    // QTBUG-75455: Suppress WM_PAINT sent to invisible windows when setting WS_EX_LAYERED
    if (!window()->isVisible() && (GetWindowLongPtr(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED) != 0)
        return false;
    // Ignore invalid update bounding rectangles
    if (!GetUpdateRect(m_data.hwnd, 0, FALSE))
        return false;
    PAINTSTRUCT ps;

    // GL software rendering (QTBUG-58178) with some AMD cards
    // (QTBUG-60527) need InvalidateRect() to suppress artifacts while resizing.
    if (testFlag(OpenGLSurface) && isSoftwareGl())
        InvalidateRect(hwnd, nullptr, false);

    BeginPaint(hwnd, &ps);

    // If the a window is obscured by another window (such as a child window)
    // we still need to send isExposed=true, for compatibility.
    // Our tests depend on it.
    fireExpose(QRegion(qrectFromRECT(ps.rcPaint)), true);
    if (!QWindowsContext::instance()->asyncExpose())
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);

    EndPaint(hwnd, &ps);
    return true;
}

void QWindowsWindow::setWindowTitle(const QString &title)
{
    setWindowTitle_sys(QWindowsWindow::formatWindowTitle(title));
}

void QWindowsWindow::setWindowFlags(Qt::WindowFlags flags)
{
    qCDebug(lcQpaWindow) << '>' << __FUNCTION__ << this << window() << "\n    from: "
        << m_data.flags << "\n    to: " << flags;
    const QRect oldGeometry = geometry();
    if (m_data.flags != flags) {
        m_data.flags = flags;
        if (m_data.hwnd) {
            m_data = setWindowFlags_sys(flags);
            updateDropSite(window()->isTopLevel());
        }
    }
    // When switching to a frameless window, geometry
    // may change without a WM_MOVE. Report change manually.
    // Do not send synchronously as not to clobber the widget
    // geometry in a sequence of setting flags and geometry.
    const QRect newGeometry = geometry_sys();
    if (oldGeometry != newGeometry)
        handleGeometryChange();

    qCDebug(lcQpaWindow) << '<' << __FUNCTION__ << "\n    returns: "
        << m_data.flags << " geometry " << oldGeometry << "->" << newGeometry;
}

QWindowsWindowData QWindowsWindow::setWindowFlags_sys(Qt::WindowFlags wt,
                                                              unsigned flags) const
{
    WindowCreationData creationData;
    creationData.fromWindow(window(), wt, flags);
    creationData.applyWindowFlags(m_data.hwnd);
    creationData.initialize(window(), m_data.hwnd, true, m_opacity);

    QWindowsWindowData result = m_data;
    result.flags = creationData.flags;
    result.embedded = creationData.embedded;
    result.hasFrame = (creationData.style & (WS_DLGFRAME | WS_THICKFRAME))
            && !(creationData.flags & Qt::FramelessWindowHint);
    return result;
}

void QWindowsWindow::handleWindowStateChange(Qt::WindowStates state)
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window()
                 << "\n    from " << m_windowState << " to " << state;
    m_windowState = state;
    QWindowSystemInterface::handleWindowStateChanged(window(), state);
    if (state & Qt::WindowMinimized) {
        handleHidden();
        QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents); // Tell QQuickWindow to stop rendering now.
    } else {
        if (state & Qt::WindowMaximized) {
            WINDOWPLACEMENT windowPlacement{};
            windowPlacement.length = sizeof(WINDOWPLACEMENT);
            GetWindowPlacement(m_data.hwnd, &windowPlacement);
            const RECT geometry = RECTfromQRect(m_data.restoreGeometry);
            windowPlacement.rcNormalPosition = geometry;
            SetWindowPlacement(m_data.hwnd, &windowPlacement);
        }
        // QTBUG-17548: We send expose events when receiving WM_Paint, but for
        // layered windows and transient children, we won't receive any WM_Paint.
        QWindow *w = window();
        bool exposeEventsSent = false;
        if (isLayered()) {
            fireFullExpose();
            exposeEventsSent = true;
        }
        const QWindowList allWindows = QGuiApplication::allWindows();
        for (QWindow *child : allWindows) {
            if (child != w && child->isVisible() && child->transientParent() == w) {
                QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(child);
                if (platformWindow && platformWindow->isLayered()) {
                    platformWindow->fireFullExpose();
                    exposeEventsSent = true;
                }
            }
        }
        if (exposeEventsSent && !QWindowsContext::instance()->asyncExpose())
            QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
    }
}

void QWindowsWindow::updateRestoreGeometry()
{
    m_data.restoreGeometry = normalFrameGeometry(m_data.hwnd);
}

void QWindowsWindow::setWindowState(Qt::WindowStates state)
{
    if (m_data.hwnd) {
        setWindowState_sys(state);
        m_windowState = state;
    }
}

bool QWindowsWindow::isFullScreen_sys() const
{
    const QWindow *w = window();
    if (!w->isTopLevel())
        return false;
    QRect geometry = geometry_sys();
    if (testFlag(HasBorderInFullScreen))
        geometry += QMargins(1, 1, 1, 1);
    QPlatformScreen *screen = screenForGeometry(geometry);
    return screen && geometry == screen->geometry();
}

/*!
    \brief Change the window state.

    \note Window frames change when maximized;
    the top margin shrinks somewhat but that cannot be obtained using
    AdjustWindowRectEx().

    \note Some calls to SetWindowLong require a subsequent call
    to ShowWindow.
*/

void QWindowsWindow::setWindowState_sys(Qt::WindowStates newState)
{
    const Qt::WindowStates oldState = m_windowState;
    if (oldState == newState)
        return;
    qCDebug(lcQpaWindow) << '>' << __FUNCTION__ << this << window()
        << " from " << oldState << " to " << newState;

    const bool visible = isVisible();
    auto stateChange = oldState ^ newState;

    if (stateChange & Qt::WindowFullScreen) {
        if (newState & Qt::WindowFullScreen) {
#ifndef Q_FLATTEN_EXPOSE
            UINT newStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_POPUP;
#else
            UINT newStyle = WS_POPUP;
#endif
            // Save geometry and style to be restored when fullscreen
            // is turned off again, since on Windows, it is not a real
            // Window state but emulated by changing geometry and style.
            if (!m_savedStyle) {
                m_savedStyle = style();
                if ((oldState & Qt::WindowMinimized) || (oldState & Qt::WindowMaximized)) {
                    const QRect nf = normalFrameGeometry(m_data.hwnd);
                    if (nf.isValid())
                        m_savedFrameGeometry = nf;
                } else {
                    m_savedFrameGeometry = frameGeometry_sys();
                }
            }
            if (newState & Qt::WindowMaximized)
                setFlag(MaximizeToFullScreen);
            if (m_savedStyle & WS_SYSMENU)
                newStyle |= WS_SYSMENU;
            if (visible)
                newStyle |= WS_VISIBLE;
            if (testFlag(HasBorderInFullScreen))
                newStyle |= WS_BORDER;
            setStyle(newStyle);
            // Use geometry of QWindow::screen() within creation or the virtual screen the
            // window is in (QTBUG-31166, QTBUG-30724).
            const QScreen *screen = window()->screen();
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            const QRect r = screen ? QHighDpi::toNativePixels(screen->geometry(), window()) : m_savedFrameGeometry;

            if (newState & Qt::WindowMinimized) {
                setMinimizedGeometry(m_data.hwnd, r);
                if (stateChange & Qt::WindowMaximized)
                    setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
            } else {
                const UINT swpf = SWP_FRAMECHANGED | SWP_NOACTIVATE;
                const bool wasSync = testFlag(SynchronousGeometryChangeEvent);
                setFlag(SynchronousGeometryChangeEvent);
                SetWindowPos(m_data.hwnd, HWND_TOP, r.left(), r.top(), r.width(), r.height(), swpf);
                if (!wasSync)
                    clearFlag(SynchronousGeometryChangeEvent);
                clearFlag(MaximizeToFullScreen);
                QWindowSystemInterface::handleGeometryChange(window(), r);
                QWindowSystemInterface::flushWindowSystemEvents(QEventLoop::ExcludeUserInputEvents);
            }
        } else {
            // Restore saved state.
            unsigned newStyle = m_savedStyle ? m_savedStyle : style();
            if (visible)
                newStyle |= WS_VISIBLE;
            setStyle(newStyle);

            const QScreen *screen = window()->screen();
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            // That area of the virtual desktop might not be covered by a screen anymore.
            if (const auto platformScreen = screen->handle()) {
                if (!platformScreen->geometry().intersects(m_savedFrameGeometry))
                    m_savedFrameGeometry.moveTo(platformScreen->geometry().topLeft());
            }

            if (newState & Qt::WindowMinimized) {
                setMinimizedGeometry(m_data.hwnd, m_savedFrameGeometry);
                if (stateChange & Qt::WindowMaximized)
                    setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
            } else {
                UINT swpf = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE;
                if (!m_savedFrameGeometry.isValid())
                    swpf |= SWP_NOSIZE | SWP_NOMOVE;
                const bool wasSync = testFlag(SynchronousGeometryChangeEvent);
                setFlag(SynchronousGeometryChangeEvent);
                // After maximized/fullscreen; the window can be in a maximized state. Clear
                // it before applying the normal geometry.
                if (windowVisibility_sys(m_data.hwnd) == QWindow::Maximized)
                    ShowWindow(m_data.hwnd, SW_SHOWNOACTIVATE);
                SetWindowPos(m_data.hwnd, nullptr, m_savedFrameGeometry.x(), m_savedFrameGeometry.y(),
                             m_savedFrameGeometry.width(), m_savedFrameGeometry.height(), swpf);
                if (!wasSync)
                    clearFlag(SynchronousGeometryChangeEvent);
                // preserve maximized state
                if (visible) {
                    setFlag(WithinMaximize);
                    ShowWindow(m_data.hwnd,
                               (newState & Qt::WindowMaximized) ? SW_MAXIMIZE : SW_SHOWNA);
                    clearFlag(WithinMaximize);
                }
            }
            m_savedStyle = 0;
            m_savedFrameGeometry = QRect();
        }
    } else if ((oldState & Qt::WindowMaximized) != (newState & Qt::WindowMaximized)) {
        if (visible && !(newState & Qt::WindowMinimized)) {
            setFlag(WithinMaximize);
            if (newState & Qt::WindowFullScreen)
                setFlag(MaximizeToFullScreen);
            ShowWindow(m_data.hwnd,
                       (newState & Qt::WindowMaximized) ? SW_MAXIMIZE : SW_SHOWNOACTIVATE);
            clearFlag(WithinMaximize);
            clearFlag(MaximizeToFullScreen);
        } else if (visible && (oldState & newState & Qt::WindowMinimized)) {
            // change of the maximized state while keeping minimized
            setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
        }
    }

    if (stateChange & Qt::WindowMinimized) {
        if (visible) {
            ShowWindow(m_data.hwnd,
                       (newState & Qt::WindowMinimized) ? SW_MINIMIZE :
                       (newState & Qt::WindowMaximized) ? SW_MAXIMIZE : SW_SHOWNORMAL);
            if ((newState & Qt::WindowMinimized) && (stateChange & Qt::WindowMaximized))
                setRestoreMaximizedFlag(m_data.hwnd, newState & Qt::WindowMaximized);
        }
    }
    qCDebug(lcQpaWindow) << '<' << __FUNCTION__ << this << window() << newState;
}

void QWindowsWindow::setStyle(unsigned s) const
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window() << debugWinStyle(s);
    setFlag(WithinSetStyle);
    SetWindowLongPtr(m_data.hwnd, GWL_STYLE, s);
    clearFlag(WithinSetStyle);
}

void QWindowsWindow::setExStyle(unsigned s) const
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window() << debugWinExStyle(s);
    SetWindowLongPtr(m_data.hwnd, GWL_EXSTYLE, s);
}

bool QWindowsWindow::windowEvent(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ApplicationPaletteChange:
        setDarkBorder(QWindowsContext::isDarkMode());
        break;
    case QEvent::WindowBlocked: // Blocked by another modal window.
        setEnabled(false);
        setFlag(BlockedByModal);
        if (hasMouseCapture())
            ReleaseCapture();
        break;
    case QEvent::WindowUnblocked:
        setEnabled(true);
        clearFlag(BlockedByModal);
        break;
    default:
        break;
    }

    return QPlatformWindow::windowEvent(event);
}

void QWindowsWindow::propagateSizeHints()
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window();
}

bool QWindowsWindow::handleGeometryChangingMessage(MSG *message, const QWindow *qWindow, const QMargins &margins)
{
    auto *windowPos = reinterpret_cast<WINDOWPOS *>(message->lParam);

    // Tell Windows to discard the entire contents of the client area, as re-using
    // parts of the client area would lead to jitter during resize.
    windowPos->flags |= SWP_NOCOPYBITS;

    if ((windowPos->flags & SWP_NOZORDER) == 0) {
        if (QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(qWindow)) {
            QWindow *parentWindow = qWindow->parent();
            HWND parentHWND = GetAncestor(windowPos->hwnd, GA_PARENT);
            HWND desktopHWND = GetDesktopWindow();
            platformWindow->m_data.embedded = !parentWindow && parentHWND && (parentHWND != desktopHWND);
        }
        if (qWindow->flags().testFlag(Qt::WindowStaysOnBottomHint))
            windowPos->hwndInsertAfter = HWND_BOTTOM;
    }
    if (!qWindow->isTopLevel()) // Implement hasHeightForWidth().
        return false;
    if (windowPos->flags & SWP_NOSIZE)
        return false;
    const QRect suggestedFrameGeometry(windowPos->x, windowPos->y,
                                       windowPos->cx, windowPos->cy);
    const QRect suggestedGeometry = suggestedFrameGeometry - margins;
    const QRectF correctedGeometryF = QPlatformWindow::closestAcceptableGeometry(qWindow, suggestedGeometry);
    if (!correctedGeometryF.isValid())
        return false;
    const QRect correctedFrameGeometry = correctedGeometryF.toRect() + margins;
    if (correctedFrameGeometry == suggestedFrameGeometry)
        return false;
    windowPos->x = correctedFrameGeometry.left();
    windowPos->y = correctedFrameGeometry.top();
    windowPos->cx = correctedFrameGeometry.width();
    windowPos->cy = correctedFrameGeometry.height();
    return true;
}

bool QWindowsWindow::handleGeometryChanging(MSG *message) const
{
    const QMargins margins = window()->isTopLevel() ? fullFrameMargins() : QMargins();
    return QWindowsWindow::handleGeometryChangingMessage(message, window(), margins);
}

void QWindowsWindow::setFullFrameMargins(const QMargins &newMargins)
{
    if (m_data.flags & Qt::FramelessWindowHint)
        return;
    if (m_data.fullFrameMargins != newMargins) {
        qCDebug(lcQpaWindow) << __FUNCTION__ << window() <<  m_data.fullFrameMargins  << "->" << newMargins;
        m_data.fullFrameMargins = newMargins;
    }
}

void QWindowsWindow::updateFullFrameMargins()
{
    // QTBUG-82580: If a native menu is present, force a WM_NCCALCSIZE.
    if (GetMenu(m_data.hwnd))
        QWindowsContext::forceNcCalcSize(m_data.hwnd);
    else
        calculateFullFrameMargins();
}

void QWindowsWindow::calculateFullFrameMargins()
{
    if (m_data.flags & Qt::FramelessWindowHint)
        return;
    // Normally obtained from WM_NCCALCSIZE. This calculation only works
    // when no native menu is present.
    const auto systemMargins = testFlag(DisableNonClientScaling)
        ? QWindowsGeometryHint::frameOnPrimaryScreen(window(), m_data.hwnd)
        : frameMargins_sys();

    // QTBUG-113736: systemMargins depends on AdjustWindowRectExForDpi. This doesn't take into
    // account possible external modifications to the titlebar, as with ExtendsContentIntoTitleBar()
    // from the Windows App SDK. We can fix this by comparing the WindowRect (which includes the
    // frame) to the ClientRect. If a 'typical' frame is detected, i.e. only the titlebar has been
    // modified, we can safely adjust the frame by deducting the bottom margin to the total Y
    // difference between the two rects, to get the actual size of the titlebar and prevent
    // unwanted client area slicing.

    RECT windowRect{};
    RECT clientRect{};
    GetWindowRect(handle(), &windowRect);
    GetClientRect(handle(), &clientRect);
    const int yDiff = (windowRect.bottom - windowRect.top) - clientRect.bottom;
    const bool typicalFrame = (systemMargins.left() == systemMargins.right())
            && (systemMargins.right() == systemMargins.bottom());

    const QMargins adjustedMargins = typicalFrame ?
          QMargins(systemMargins.left(), yDiff - systemMargins.bottom(),
                   systemMargins.right(), systemMargins.bottom())
            : systemMargins;

    setFullFrameMargins(adjustedMargins + customMargins());
}

QMargins QWindowsWindow::frameMargins() const
{
    QMargins result = fullFrameMargins();
    if (isTopLevel() && m_data.hasFrame)
        result -= invisibleMargins(m_data.hwnd);
    return result;
}

QMargins QWindowsWindow::fullFrameMargins() const
{
    if (m_data.flags & Qt::FramelessWindowHint)
        return {};
    return m_data.fullFrameMargins;
}

void QWindowsWindow::setOpacity(qreal level)
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << level;
    if (!qFuzzyCompare(m_opacity, level)) {
        m_opacity = level;
        if (m_data.hwnd)
            setWindowOpacity(m_data.hwnd, m_data.flags,
                             window()->format().hasAlpha(), testFlag(OpenGLSurface) || testFlag(VulkanSurface) || testFlag(Direct3DSurface),
                             level);
    }
}

static inline HRGN createRectRegion(const QRect &r)
{
    return CreateRectRgn(r.left(), r.top(), r.x() + r.width(), r.y() + r.height());
}

static inline void addRectToWinRegion(const QRect &rect, HRGN *winRegion)
{
    if (const HRGN rectRegion = createRectRegion(rect)) {
        HRGN result = CreateRectRgn(0, 0, 0, 0);
        if (CombineRgn(result, *winRegion, rectRegion, RGN_OR)) {
            DeleteObject(*winRegion);
            *winRegion = result;
        }
        DeleteObject(rectRegion);
    }
}

static HRGN qRegionToWinRegion(const QRegion &region)
{
    auto it = region.begin();
    const auto end = region.end();
    if (it == end)
        return nullptr;
    HRGN hRegion = createRectRegion(*it);
    while (++it != end)
        addRectToWinRegion(*it, &hRegion);
    return hRegion;
}

void QWindowsWindow::setMask(const QRegion &region)
{
    if (region.isEmpty()) {
         SetWindowRgn(m_data.hwnd, nullptr, true);
         return;
    }
    const HRGN winRegion = qRegionToWinRegion(region);

    // Mask is in client area coordinates, so offset it in case we have a frame
    if (window()->isTopLevel()) {
        const QMargins margins = fullFrameMargins();
        OffsetRgn(winRegion, margins.left(), margins.top());
    }

    // SetWindowRgn takes ownership.
    if (!SetWindowRgn(m_data.hwnd, winRegion, true))
        DeleteObject(winRegion);
}

void QWindowsWindow::requestActivateWindow()
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window();
    // 'Active' state handling is based in focus since it needs to work for
    // child windows as well.
    if (m_data.hwnd) {
        const DWORD currentThread = GetCurrentThreadId();
        bool attached = false;
        DWORD foregroundThread = 0;

        // QTBUG-14062, QTBUG-37435: Windows normally only flashes the taskbar entry
        // when activating windows of inactive applications. Attach to the input of the
        // currently active window while setting the foreground window to always activate
        // the window when desired.
        const auto activationBehavior = QWindowsIntegration::instance()->windowActivationBehavior();
        if (QGuiApplication::applicationState() != Qt::ApplicationActive
            && activationBehavior == QWindowsApplication::AlwaysActivateWindow) {
            if (const HWND foregroundWindow = GetForegroundWindow()) {
                foregroundThread = GetWindowThreadProcessId(foregroundWindow, nullptr);
                if (foregroundThread && foregroundThread != currentThread)
                    attached = AttachThreadInput(foregroundThread, currentThread, TRUE) == TRUE;
                if (attached) {
                    if (!window()->flags().testFlag(Qt::WindowStaysOnBottomHint)
                        && !window()->flags().testFlag(Qt::WindowStaysOnTopHint)
                        && window()->type() != Qt::ToolTip) {
                        const UINT swpFlags = SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER;
                        SetWindowPos(m_data.hwnd, HWND_TOPMOST, 0, 0, 0, 0, swpFlags);
                        SetWindowPos(m_data.hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, swpFlags);
                    }
                }
            }
        }
        SetForegroundWindow(m_data.hwnd);
        SetFocus(m_data.hwnd);
        if (attached)
            AttachThreadInput(foregroundThread, currentThread, FALSE);
    }
}

bool QWindowsWindow::setKeyboardGrabEnabled(bool grab)
{
    if (!m_data.hwnd) {
        qWarning("%s: No handle", __FUNCTION__);
        return false;
    }
    qCDebug(lcQpaWindow) << __FUNCTION__ << this << window() << grab;

    QWindowsContext *context = QWindowsContext::instance();
    if (grab) {
        context->setKeyGrabber(window());
    } else {
        if (context->keyGrabber() == window())
            context->setKeyGrabber(nullptr);
    }
    return true;
}

bool QWindowsWindow::setMouseGrabEnabled(bool grab)
{
    qCDebug(lcQpaWindow) << __FUNCTION__ << window() << grab;
    if (!m_data.hwnd) {
        qWarning("%s: No handle", __FUNCTION__);
        return false;
    }
    if (!isVisible() && grab) {
        qWarning("%s: Not setting mouse grab for invisible window %s/'%s'",
                 __FUNCTION__, window()->metaObject()->className(),
                 qPrintable(window()->objectName()));
        return false;
    }
    // release grab or an explicit grab overriding autocapture: Clear flag.
    clearFlag(QWindowsWindow::AutoMouseCapture);
    if (hasMouseCapture() != grab) {
        if (grab) {
            SetCapture(m_data.hwnd);
        } else {
            ReleaseCapture();
        }
    }
    return grab;
}

static inline DWORD edgesToWinOrientation(Qt::Edges edges)
{
    if (edges == Qt::LeftEdge)
        return 0xf001; // SC_SIZELEFT;
    else if (edges == (Qt::RightEdge))
        return 0xf002; // SC_SIZERIGHT
    else if (edges == (Qt::TopEdge))
        return 0xf003; // SC_SIZETOP
    else if (edges == (Qt::TopEdge | Qt::LeftEdge))
        return 0xf004; // SC_SIZETOPLEFT
    else if (edges == (Qt::TopEdge | Qt::RightEdge))
        return 0xf005; // SC_SIZETOPRIGHT
    else if (edges == (Qt::BottomEdge))
        return 0xf006; // SC_SIZEBOTTOM
    else if (edges == (Qt::BottomEdge | Qt::LeftEdge))
        return 0xf007; // SC_SIZEBOTTOMLEFT
    else if (edges == (Qt::BottomEdge | Qt::RightEdge))
        return 0xf008; // SC_SIZEBOTTOMRIGHT

    return 0xf000; // SC_SIZE
}

bool QWindowsWindow::startSystemResize(Qt::Edges edges)
{
    if (Q_UNLIKELY(window()->flags().testFlag(Qt::MSWindowsFixedSizeDialogHint)))
        return false;

    ReleaseCapture();
    PostMessage(m_data.hwnd, WM_SYSCOMMAND, edgesToWinOrientation(edges), 0);
    setFlag(SizeGripOperation);
    return true;
}

bool QWindowsWindow::startSystemMove()
{
    ReleaseCapture();
    PostMessage(m_data.hwnd, WM_SYSCOMMAND, 0xF012 /*SC_DRAGMOVE*/, 0);
    return true;
}

void QWindowsWindow::setFrameStrutEventsEnabled(bool enabled)
{
    if (enabled) {
        setFlag(FrameStrutEventsEnabled);
    } else {
        clearFlag(FrameStrutEventsEnabled);
    }
}

void QWindowsWindow::getSizeHints(MINMAXINFO *mmi) const
{
    QWindowsGeometryHint::applyToMinMaxInfo(window(), fullFrameMargins(), mmi);
    qCDebug(lcQpaWindow) << __FUNCTION__ << window() << *mmi;
}

bool QWindowsWindow::handleNonClientHitTest(const QPoint &globalPos, LRESULT *result) const
{
    // QTBUG-32663, suppress resize cursor for fixed size windows.
    const QWindow *w = window();
    if (!w->isTopLevel() // Task 105852, minimized windows need to respond to user input.
        || (m_windowState != Qt::WindowNoState)
        || !isActive()
        || (m_data.flags & Qt::FramelessWindowHint)) {
        return false;
    }
    const QSize minimumSize = w->minimumSize();
    if (minimumSize.isEmpty())
        return false;
    const QSize maximumSize = w->maximumSize();
    const bool fixedWidth = minimumSize.width() == maximumSize.width();
    const bool fixedHeight = minimumSize.height() == maximumSize.height();
    if (!fixedWidth && !fixedHeight)
        return false;
    const QPoint localPos = w->mapFromGlobal(QHighDpi::fromNativePixels(globalPos, w));
    const QSize size = w->size();
    if (fixedHeight) {
        if (localPos.y() >= size.height()) {
            *result = HTBORDER; // Unspecified border, no resize cursor.
            return true;
        }
        if (localPos.y() < 0) {
            const int topResizeBarPos = invisibleMargins(m_data.hwnd).left() - frameMargins().top();
            if (localPos.y() < topResizeBarPos) {
                *result = HTCAPTION; // Extend caption over top resize bar, let's user move the window.
                return true;
            }
        }
    }
    if (fixedWidth && (localPos.x() < 0 || localPos.x() >= size.width())) {
        *result = HTBORDER; // Unspecified border, no resize cursor.
        return true;
    }
    return false;
}

#ifndef QT_NO_CURSOR
// Return the default cursor (Arrow) from QWindowsCursor's cache.
static inline CursorHandlePtr defaultCursor(const QWindow *w)
{
    if (QScreen *screen = w->screen())
        if (const QPlatformScreen *platformScreen = screen->handle())
            if (QPlatformCursor *cursor = platformScreen->cursor())
                return static_cast<QWindowsCursor *>(cursor)->standardWindowCursor(Qt::ArrowCursor);
    return CursorHandlePtr(new CursorHandle(QWindowsCursor::createCursorFromShape(Qt::ArrowCursor)));
}

// Check whether to apply a new cursor. Either the window in question is
// currently under mouse, or it is the parent of the window under mouse and
// there is no other window with an explicitly set cursor in-between.
static inline bool applyNewCursor(const QWindow *w)
{
    const QWindow *underMouse = QWindowsContext::instance()->windowUnderMouse();
    if (underMouse == w)
        return true;
    for (const QWindow *p = underMouse; p ; p = p->parent()) {
        if (p == w)
            return true;
        const QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(p);
        if (platformWindow && !platformWindow->cursor()->isNull())
            return false;
    }
    return false;
}
#endif // !QT_NO_CURSOR

/*!
    \brief Applies to cursor property set on the window to the global cursor.

    \sa QWindowsCursor
*/

void QWindowsWindow::applyCursor()
{
    if (QWindowsCursor::hasOverrideCursor()) {
        if (isTopLevel())
            QWindowsCursor::enforceOverrideCursor();
        return;
    }
#ifndef QT_NO_CURSOR
    if (m_cursor->isNull()) { // Recurse up to parent with non-null cursor. Set default for toplevel.
        if (const QWindow *p = window()->parent()) {
            if (QWindowsWindow *platformWindow = QWindowsWindow::windowsWindowOf(p))
                platformWindow->applyCursor();
        } else {
            SetCursor(defaultCursor(window())->handle());
        }
    } else {
        SetCursor(m_cursor->handle());
    }
#endif
}

void QWindowsWindow::setCursor(const CursorHandlePtr &c)
{
#ifndef QT_NO_CURSOR
    bool changed = c->handle() != m_cursor->handle();
    // QTBUG-98856: Cursors can get out of sync after restoring override
    // cursors on native windows. Force an update.
    if (testFlag(RestoreOverrideCursor)) {
        clearFlag(RestoreOverrideCursor);
        changed = true;
    }
    if (changed) {
        const bool apply = applyNewCursor(window());
        qCDebug(lcQpaWindow) << window() << __FUNCTION__
            << c->handle() << " doApply=" << apply;
        m_cursor = c;
        if (apply)
            applyCursor();
    }
#endif
}

void QWindowsWindow::setAlertState(bool enabled)
{
    if (isAlertState() == enabled)
        return;
    if (enabled) {
        alertWindow(0);
        setFlag(AlertState);
    } else {
        stopAlertWindow();
        clearFlag(AlertState);
    }
}

void QWindowsWindow::alertWindow(int durationMs)
{
    UINT timeOutMs = GetCaretBlinkTime();
    if (!timeOutMs || timeOutMs == INFINITE)
        timeOutMs = 250;

    FLASHWINFO info;
    info.cbSize = sizeof(info);
    info.hwnd = m_data.hwnd;
    info.dwFlags = FLASHW_TRAY;
    info.dwTimeout = timeOutMs;
    info.uCount = durationMs == 0 ? 10 : UINT(durationMs) / timeOutMs;
    FlashWindowEx(&info);
}

void QWindowsWindow::stopAlertWindow()
{
    FLASHWINFO info;
    info.cbSize = sizeof(info);
    info.hwnd = m_data.hwnd;
    info.dwFlags = FLASHW_STOP;
    info.dwTimeout = 0;
    info.uCount = 0;
    FlashWindowEx(&info);
}

bool QWindowsWindow::isEnabled() const
{
    return (style() & WS_DISABLED) == 0;
}

void QWindowsWindow::setEnabled(bool enabled)
{
    const unsigned oldStyle = style();
    unsigned newStyle = oldStyle;
    if (enabled) {
        newStyle &= ~WS_DISABLED;
    } else {
        newStyle |= WS_DISABLED;
    }
    if (newStyle != oldStyle)
        setStyle(newStyle);
}

static HICON createHIcon(const QIcon &icon, int xSize, int ySize)
{
    if (!icon.isNull()) {
        // QTBUG-90363, request DPR=1 for the title bar.
        const QPixmap pm = icon.pixmap(icon.actualSize(QSize(xSize, ySize)), 1);
        if (!pm.isNull())
            return qt_pixmapToWinHICON(pm);
    }
    return nullptr;
}

void QWindowsWindow::setWindowIcon(const QIcon &icon)
{
    if (m_data.hwnd) {
        destroyIcon();

        m_iconSmall = createHIcon(icon, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
        m_iconBig = createHIcon(icon, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));

        if (m_iconBig) {
            SendMessage(m_data.hwnd, WM_SETICON, 0 /* ICON_SMALL */, LPARAM(m_iconSmall));
            SendMessage(m_data.hwnd, WM_SETICON, 1 /* ICON_BIG */, LPARAM(m_iconBig));
        } else {
            SendMessage(m_data.hwnd, WM_SETICON, 0 /* ICON_SMALL */, LPARAM(m_iconSmall));
            SendMessage(m_data.hwnd, WM_SETICON, 1 /* ICON_BIG */, LPARAM(m_iconSmall));
        }
    }
}

bool QWindowsWindow::isTopLevel() const
{
    return window()->isTopLevel() && !m_data.embedded;
}

enum : WORD {
    DwmwaUseImmersiveDarkMode = 20,
    DwmwaUseImmersiveDarkModeBefore20h1 = 19
};

static bool queryDarkBorder(HWND hwnd)
{
    BOOL result = FALSE;
    const bool ok =
        SUCCEEDED(DwmGetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, &result, sizeof(result)))
        || SUCCEEDED(DwmGetWindowAttribute(hwnd, DwmwaUseImmersiveDarkModeBefore20h1, &result, sizeof(result)));
    if (!ok)
        qCWarning(lcQpaWindow, "%s: Unable to retrieve dark window border setting.", __FUNCTION__);
    return result == TRUE;
}

bool QWindowsWindow::setDarkBorderToWindow(HWND hwnd, bool d)
{
    const BOOL darkBorder = d ? TRUE : FALSE;
    const bool ok =
        SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, &darkBorder, sizeof(darkBorder)))
        || SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkModeBefore20h1, &darkBorder, sizeof(darkBorder)));
    if (!ok)
        qCWarning(lcQpaWindow, "%s: Unable to set %s window border.", __FUNCTION__, d ? "dark" : "light");
    return ok;
}

void QWindowsWindow::setDarkBorder(bool d)
{
    // respect explicit opt-out and incompatible palettes or styles
    d = d && shouldApplyDarkFrame(window());
    if (queryDarkBorder(m_data.hwnd) == d)
        return;

    setDarkBorderToWindow(m_data.hwnd, d);
}

QWindowsMenuBar *QWindowsWindow::menuBar() const
{
    return m_menuBar.data();
}

void QWindowsWindow::setMenuBar(QWindowsMenuBar *mb)
{
    m_menuBar = mb;
}

QMargins QWindowsWindow::customMargins() const
{
    if (m_data.flags & Qt::FramelessWindowHint)
        return {};
    return m_data.customMargins;
}

/*!
    \brief Sets custom margins to be added to the default margins determined by
    the windows style in the handling of the WM_NCCALCSIZE message.

    This is currently used to give the Aero-style QWizard a smaller top margin.
    The property can be set using QPlatformNativeInterface::setWindowProperty() or,
    before platform window creation, by setting a dynamic property
    on the QWindow (see QWindowsIntegration::createPlatformWindow()).
*/

void QWindowsWindow::setCustomMargins(const QMargins &newCustomMargins)
{
    if (m_data.flags & Qt::FramelessWindowHint) {
        qCWarning(lcQpaWindow) << "You should not set custom margins for a frameless window.";
        return;
    }
    if (newCustomMargins != m_data.customMargins) {
        const QMargins oldCustomMargins = m_data.customMargins;
        m_data.customMargins = newCustomMargins;
         // Re-trigger WM_NCALCSIZE with wParam=1 by passing SWP_FRAMECHANGED
        const QRect currentFrameGeometry = frameGeometry_sys();
        const QPoint topLeft = currentFrameGeometry.topLeft();
        QRect newFrame = currentFrameGeometry.marginsRemoved(oldCustomMargins) + m_data.customMargins;
        newFrame.moveTo(topLeft);
        qCDebug(lcQpaWindow) << __FUNCTION__ << oldCustomMargins << "->" << newCustomMargins
            << currentFrameGeometry << "->" << newFrame;
        SetWindowPos(m_data.hwnd, nullptr, newFrame.x(), newFrame.y(), newFrame.width(), newFrame.height(), SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }
}

void *QWindowsWindow::surface(void *nativeConfig, int *err)
{
#if QT_CONFIG(vulkan)
    Q_UNUSED(nativeConfig);
    Q_UNUSED(err);
    if (window()->surfaceType() == QSurface::VulkanSurface) {
        if (!m_vkSurface) {
            QVulkanInstance *inst = window()->vulkanInstance();
            if (inst)
                m_vkSurface = static_cast<QWindowsVulkanInstance *>(inst->handle())->createSurface(handle());
            else
                qWarning("Attempted to create Vulkan surface without an instance; was QWindow::setVulkanInstance() called?");
        }
        // Different semantics for VkSurfaces: the return value is the address,
        // not the value, given that this is a 64-bit handle even on x86.
        return &m_vkSurface;
    }
#elif defined(QT_NO_OPENGL)
    Q_UNUSED(err);
    Q_UNUSED(nativeConfig);
    return nullptr;
#endif
#ifndef QT_NO_OPENGL
    if (!m_surface) {
        if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
            m_surface = staticOpenGLContext->createWindowSurface(m_data.hwnd, nativeConfig, err);
    }

    return m_surface;
#else
    return nullptr;
#endif
}

void QWindowsWindow::invalidateSurface()
{
#if QT_CONFIG(vulkan)
    if (m_vkSurface) {
        QVulkanInstance *inst = window()->vulkanInstance();
        if (inst)
            static_cast<QWindowsVulkanInstance *>(inst->handle())->destroySurface(m_vkSurface);
        m_vkSurface = VK_NULL_HANDLE;
    }
#endif
#ifndef QT_NO_OPENGL
    if (m_surface) {
        if (QWindowsStaticOpenGLContext *staticOpenGLContext = QWindowsIntegration::staticOpenGLContext())
            staticOpenGLContext->destroyWindowSurface(m_surface);
        m_surface = nullptr;
    }
#endif // QT_NO_OPENGL
}

void QWindowsWindow::registerTouchWindow()
{
    if ((QWindowsContext::instance()->systemInfo() & QWindowsContext::SI_SupportsTouch) == 0)
        return;

    // Initially register or re-register to change the flags
    const auto touchTypes = QWindowsIntegration::instance()->touchWindowTouchType();
    if (testFlag(TouchRegistered)) {
        const auto currentTouchTypes = touchWindowTouchTypes_sys();
        if (currentTouchTypes.has_value() && currentTouchTypes.value() == touchTypes)
            return;
    }

    ULONG touchFlags = 0;
    if (touchTypes.testFlag(TouchWindowTouchType::FineTouch))
        touchFlags |= TWF_FINETOUCH;
    if (touchTypes.testFlag(TouchWindowTouchType::WantPalmTouch))
        touchFlags |= TWF_WANTPALM;
    if (RegisterTouchWindow(m_data.hwnd, touchFlags))
        setFlag(TouchRegistered);
    else
        qErrnoWarning("RegisterTouchWindow() failed for window '%s'.", qPrintable(window()->objectName()));
}

void QWindowsWindow::aboutToMakeCurrent()
{
#ifndef QT_NO_OPENGL
    // For RasterGLSurface windows, that become OpenGL windows dynamically, it might be
    // time to set up some GL specifics.  This is particularly important for layered
    // windows (WS_EX_LAYERED due to alpha > 0).
    const bool isCompositing = qt_window_private(window())->compositing;
    if (isCompositing != testFlag(Compositing)) {
        if (isCompositing)
            setFlag(Compositing);
        else
            clearFlag(Compositing);

        updateGLWindowSettings(window(), m_data.hwnd, m_data.flags, m_opacity);
    }
#endif
}

void QWindowsWindow::setHasBorderInFullScreenStatic(QWindow *window, bool border)
{
    if (QPlatformWindow *handle = window->handle())
        static_cast<QWindowsWindow *>(handle)->setHasBorderInFullScreen(border);
    else
        window->setProperty(hasBorderInFullScreenProperty, QVariant(border));
}

void QWindowsWindow::setHasBorderInFullScreenDefault(bool border)
{
    m_borderInFullScreenDefault = border;
}

bool QWindowsWindow::hasBorderInFullScreen() const
{
    return testFlag(HasBorderInFullScreen);
}

void QWindowsWindow::setHasBorderInFullScreen(bool border)
{
    if (hasBorderInFullScreen() == border)
        return;
    if (border)
        setFlag(HasBorderInFullScreen);
    else
        clearFlag(HasBorderInFullScreen);
    // Directly apply the flag in case we are fullscreen.
    if (m_windowState == Qt::WindowFullScreen) {
        LONG_PTR style = GetWindowLongPtr(handle(), GWL_STYLE);
        if (border)
            style |= WS_BORDER;
        else
            style &= ~WS_BORDER;
        SetWindowLongPtr(handle(), GWL_STYLE, style);
    }
}

QString QWindowsWindow::formatWindowTitle(const QString &title)
{
    return QPlatformWindow::formatWindowTitle(title, QStringLiteral(" - "));
}

QT_END_NAMESPACE
