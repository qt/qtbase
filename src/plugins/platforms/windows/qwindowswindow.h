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

#ifndef QWINDOWSWINDOW_H
#define QWINDOWSWINDOW_H

#include <QtCore/qt_windows.h>
#include <QtCore/QPointer>
#include "qwindowscursor.h"

#include <qpa/qplatformwindow.h>
#include <QtPlatformHeaders/qwindowswindowfunctions.h>

#if QT_CONFIG(vulkan)
#include "qwindowsvulkaninstance.h"
#endif

QT_BEGIN_NAMESPACE

class QWindowsOleDropTarget;
class QWindowsMenuBar;
class QDebug;

struct QWindowsGeometryHint
{
    QWindowsGeometryHint() {}
    explicit QWindowsGeometryHint(const QWindow *w, const QMargins &customMargins);
    static QMargins frame(DWORD style, DWORD exStyle);
    static bool handleCalculateSize(const QMargins &customMargins, const MSG &msg, LRESULT *result);
    void applyToMinMaxInfo(DWORD style, DWORD exStyle, MINMAXINFO *mmi) const;
    void applyToMinMaxInfo(HWND hwnd, MINMAXINFO *mmi) const;
    bool validSize(const QSize &s) const;

    static inline QPoint mapToGlobal(HWND hwnd, const QPoint &);
    static inline QPoint mapToGlobal(const QWindow *w, const QPoint &);
    static inline QPoint mapFromGlobal(const HWND hwnd, const QPoint &);
    static inline QPoint mapFromGlobal(const QWindow *w, const QPoint &);

    static bool positionIncludesFrame(const QWindow *w);

    QSize minimumSize;
    QSize maximumSize;
    QMargins customMargins;
};

struct QWindowCreationContext
{
    explicit QWindowCreationContext(const QWindow *w,
                                    const QRect &geometryIn, const QRect &geometry,
                                    const QMargins &customMargins,
                                    DWORD style, DWORD exStyle);
    void applyToMinMaxInfo(MINMAXINFO *mmi) const
        { geometryHint.applyToMinMaxInfo(style, exStyle, mmi); }

    QWindowsGeometryHint geometryHint;
    const QWindow *window;
    DWORD style;
    DWORD exStyle;
    QRect requestedGeometryIn; // QWindow scaled
    QRect requestedGeometry; // after QPlatformWindow::initialGeometry()
    QRect obtainedGeometry;
    QMargins margins;
    QMargins customMargins;  // User-defined, additional frame for WM_NCCALCSIZE
    int frameX = CW_USEDEFAULT; // Passed on to CreateWindowEx(), including frame.
    int frameY = CW_USEDEFAULT;
    int frameWidth = CW_USEDEFAULT;
    int frameHeight = CW_USEDEFAULT;
    int menuHeight = 0;
};

struct QWindowsWindowData
{
    Qt::WindowFlags flags;
    QRect geometry;
    QMargins frame; // Do not use directly for windows, see FrameDirty.
    QMargins customMargins; // User-defined, additional frame for NCCALCSIZE
    HWND hwnd = 0;
    bool embedded = false;

    static QWindowsWindowData create(const QWindow *w,
                                     const QWindowsWindowData &parameters,
                                     const QString &title);
};

class QWindowsBaseWindow : public QPlatformWindow
{
public:
    explicit QWindowsBaseWindow(QWindow *window) : QPlatformWindow(window) {}

    WId winId() const override { return WId(handle()); }
    QRect geometry() const override { return geometry_sys(); }
    QMargins frameMargins() const override { return frameMargins_sys(); }
    QPoint mapToGlobal(const QPoint &pos) const override;
    QPoint mapFromGlobal(const QPoint &pos) const override;

    using QPlatformWindow::screenForGeometry;

    virtual HWND handle() const = 0;
    virtual bool isTopLevel() const { return isTopLevel_sys(); }

    unsigned style() const   { return GetWindowLongPtr(handle(), GWL_STYLE); }
    unsigned exStyle() const { return GetWindowLongPtr(handle(), GWL_EXSTYLE); }

    static QWindowsBaseWindow *baseWindowOf(const QWindow *w);
    static HWND handleOf(const QWindow *w);

protected:
    HWND parentHwnd() const { return GetAncestor(handle(), GA_PARENT); }
    bool isTopLevel_sys() const;
    QRect frameGeometry_sys() const;
    QRect geometry_sys() const;
    void setGeometry_sys(const QRect &rect) const;
    QMargins frameMargins_sys() const;
    void hide_sys();
    void raise_sys();
    void lower_sys();
    void setWindowTitle_sys(const QString &title);
};

class QWindowsDesktopWindow : public QWindowsBaseWindow
{
public:
    explicit QWindowsDesktopWindow(QWindow *window)
        : QWindowsBaseWindow(window), m_hwnd(GetDesktopWindow()) {}

    QMargins frameMargins() const override { return QMargins(); }
    bool isTopLevel() const override { return true; }

protected:
    HWND handle() const override { return m_hwnd; }

private:
    const HWND m_hwnd;
};

class QWindowsForeignWindow : public QWindowsBaseWindow
{
public:
    explicit QWindowsForeignWindow(QWindow *window, HWND hwnd);

    void setParent(const QPlatformWindow *window) override;
    void setGeometry(const QRect &rect) override { setGeometry_sys(rect); }
    void setVisible(bool visible) override;
    void raise() override { raise_sys(); }
    void lower() override { lower_sys(); }
    void setWindowTitle(const QString &title) override { setWindowTitle_sys(title); }
    bool isForeignWindow() const override { return true; }

protected:
    HWND handle() const override { return m_hwnd; }

private:
    const HWND m_hwnd;
    DWORD m_topLevelStyle;
};

class QWindowsWindow : public QWindowsBaseWindow
{
public:
    enum Flags
    {
        AutoMouseCapture = 0x1, //! Automatic mouse capture on button press.
        WithinSetParent = 0x2,
        WithinSetGeometry = 0x8,
        OpenGLSurface = 0x10,
        OpenGL_ES2 = 0x20,
        OpenGLDoubleBuffered = 0x40,
        OpenGlPixelFormatInitialized = 0x80,
        BlockedByModal = 0x100,
        SizeGripOperation = 0x200,
        FrameStrutEventsEnabled = 0x400,
        SynchronousGeometryChangeEvent = 0x800,
        WithinSetStyle = 0x1000,
        WithinDestroy = 0x2000,
        TouchRegistered = 0x4000,
        AlertState = 0x8000,
        Exposed = 0x10000,
        WithinCreate = 0x20000,
        WithinMaximize = 0x40000,
        MaximizeToFullScreen = 0x80000,
        Compositing = 0x100000,
        HasBorderInFullScreen = 0x200000,
        WithinDpiChanged = 0x400000,
        VulkanSurface = 0x800000,
        ResizeMoveActive = 0x1000000
    };

    QWindowsWindow(QWindow *window, const QWindowsWindowData &data);
    ~QWindowsWindow();

    void initialize() override;

    using QPlatformWindow::screenForGeometry;

    QSurfaceFormat format() const override { return m_format; }
    void setGeometry(const QRect &rect) override;
    QRect geometry() const override { return m_data.geometry; }
    QRect normalGeometry() const override;

    void setVisible(bool visible) override;
    bool isVisible() const;
    bool isExposed() const override { return testFlag(Exposed); }
    bool isActive() const override;
    bool isAncestorOf(const QPlatformWindow *child) const override;
    bool isEmbedded() const override;
    QPoint mapToGlobal(const QPoint &pos) const override;
    QPoint mapFromGlobal(const QPoint &pos) const override;

    void setWindowFlags(Qt::WindowFlags flags) override;
    void setWindowState(Qt::WindowStates state) override;

    void setParent(const QPlatformWindow *window) override;

    void setWindowTitle(const QString &title) override;
    void raise() override { raise_sys(); }
    void lower() override { lower_sys(); }

    void windowEvent(QEvent *event) override;

    void propagateSizeHints() override;
    static bool handleGeometryChangingMessage(MSG *message, const QWindow *qWindow, const QMargins &marginsDp);
    bool handleGeometryChanging(MSG *message) const;
    QMargins frameMargins() const override;
    void setFrameMargins(const QMargins &newMargins);

    void setOpacity(qreal level) override;
    void setMask(const QRegion &region) override;
    qreal opacity() const { return m_opacity; }
    void requestActivateWindow() override;

    bool setKeyboardGrabEnabled(bool grab) override;
    bool setMouseGrabEnabled(bool grab) override;
    inline bool hasMouseCapture() const { return GetCapture() == m_data.hwnd; }

    bool startSystemResize(const QPoint &pos, Qt::Corner corner) override;
    bool startSystemMove(const QPoint &pos) override;

    void setFrameStrutEventsEnabled(bool enabled) override;
    bool frameStrutEventsEnabled() const override { return testFlag(FrameStrutEventsEnabled); }

    // QWindowsBaseWindow overrides
    HWND handle() const override { return m_data.hwnd; }
    bool isTopLevel() const override;

    QWindowsMenuBar *menuBar() const;
    void setMenuBar(QWindowsMenuBar *mb);

    QMargins customMargins() const { return m_data.customMargins; }
    void setCustomMargins(const QMargins &m);

    void setStyle(unsigned s) const;
    void setExStyle(unsigned s) const;

    bool handleWmPaint(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void handleMoved();
    void handleResized(int wParam);
    void handleHidden();
    void handleCompositionSettingsChanged();

    static QWindowsWindow *windowsWindowOf(const QWindow *w);
    static QWindow *topLevelOf(QWindow *w);
    static inline void *userDataOf(HWND hwnd);
    static inline void setUserDataOf(HWND hwnd, void *ud);

    static bool setWindowLayered(HWND hwnd, Qt::WindowFlags flags, bool hasAlpha, qreal opacity);
    bool isLayered() const;

    HDC getDC();
    void releaseDC();
    void getSizeHints(MINMAXINFO *mmi) const;
    bool handleNonClientHitTest(const QPoint &globalPos, LRESULT *result) const;

#ifndef QT_NO_CURSOR
    CursorHandlePtr cursor() const { return m_cursor; }
#endif
    void setCursor(const CursorHandlePtr &c);
    void applyCursor();

    inline bool testFlag(unsigned f) const  { return (m_flags & f) != 0; }
    inline void setFlag(unsigned f) const   { m_flags |= f; }
    inline void clearFlag(unsigned f) const { m_flags &= ~f; }

    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setWindowIcon(const QIcon &icon) override;

    void *surface(void *nativeConfig, int *err);
    void invalidateSurface() override;
    void aboutToMakeCurrent();

    void setAlertState(bool enabled) override;
    bool isAlertState() const override { return testFlag(AlertState); }
    void alertWindow(int durationMs = 0);
    void stopAlertWindow();

    void checkForScreenChanged();

    static void setTouchWindowTouchTypeStatic(QWindow *window, QWindowsWindowFunctions::TouchWindowTouchTypes touchTypes);
    void registerTouchWindow(QWindowsWindowFunctions::TouchWindowTouchTypes touchTypes = QWindowsWindowFunctions::NormalTouch);
    static void setHasBorderInFullScreenStatic(QWindow *window, bool border);
    void setHasBorderInFullScreen(bool border);
    static QString formatWindowTitle(const QString &title);

    static const char *embeddedNativeParentHandleProperty;
    static const char *hasBorderInFullScreenProperty;

private:
    inline void show_sys() const;
    inline QWindowsWindowData setWindowFlags_sys(Qt::WindowFlags wt, unsigned flags = 0) const;
    inline bool isFullScreen_sys() const;
    inline void setWindowState_sys(Qt::WindowStates newState);
    inline void setParent_sys(const QPlatformWindow *parent);
    inline void updateTransientParent() const;
    void destroyWindow();
    inline bool isDropSiteEnabled() const { return m_dropTarget != 0; }
    void setDropSiteEnabled(bool enabled);
    void updateDropSite(bool topLevel);
    void handleGeometryChange();
    void handleWindowStateChange(Qt::WindowStates state);
    inline void destroyIcon();
    void fireExpose(const QRegion &region, bool force=false);

    mutable QWindowsWindowData m_data;
    QPointer<QWindowsMenuBar> m_menuBar;
    mutable unsigned m_flags = WithinCreate;
    HDC m_hdc = 0;
    Qt::WindowStates m_windowState = Qt::WindowNoState;
    qreal m_opacity = 1;
#ifndef QT_NO_CURSOR
    CursorHandlePtr m_cursor;
#endif
    QWindowsOleDropTarget *m_dropTarget = nullptr;
    unsigned m_savedStyle = 0;
    QRect m_savedFrameGeometry;
    const QSurfaceFormat m_format;
    HICON m_iconSmall = 0;
    HICON m_iconBig = 0;
    void *m_surface = nullptr;

#if QT_CONFIG(vulkan)
    // note: intentionally not using void * in order to avoid breaking x86
    VkSurfaceKHR m_vkSurface = 0;
#endif
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const RECT &r);
QDebug operator<<(QDebug d, const POINT &);
QDebug operator<<(QDebug d, const MINMAXINFO &i);
QDebug operator<<(QDebug d, const NCCALCSIZE_PARAMS &p);
QDebug operator<<(QDebug d, const WINDOWPLACEMENT &);
QDebug operator<<(QDebug d, const WINDOWPOS &);
QDebug operator<<(QDebug d, const GUID &guid);
#endif // !QT_NO_DEBUG_STREAM

// ---------- QWindowsGeometryHint inline functions.
QPoint QWindowsGeometryHint::mapToGlobal(HWND hwnd, const QPoint &qp)
{
    POINT p = { qp.x(), qp.y() };
    ClientToScreen(hwnd, &p);
    return QPoint(p.x, p.y);
}

QPoint QWindowsGeometryHint::mapFromGlobal(const HWND hwnd, const QPoint &qp)
{
    POINT p = { qp.x(), qp.y() };
    ScreenToClient(hwnd, &p);
    return QPoint(p.x, p.y);
}

QPoint QWindowsGeometryHint::mapToGlobal(const QWindow *w, const QPoint &p)
    { return QWindowsGeometryHint::mapToGlobal(QWindowsWindow::handleOf(w), p); }

QPoint QWindowsGeometryHint::mapFromGlobal(const QWindow *w, const QPoint &p)
    { return QWindowsGeometryHint::mapFromGlobal(QWindowsWindow::handleOf(w), p); }


// ---------- QWindowsBaseWindow inline functions.

inline QWindowsWindow *QWindowsWindow::windowsWindowOf(const QWindow *w)
{
    if (!w || !w->handle())
        return nullptr;

    const Qt::WindowType type = w->type();
    if (type == Qt::Desktop || w->handle()->isForeignWindow())
        return nullptr;

    return static_cast<QWindowsWindow *>(w->handle());
}

void *QWindowsWindow::userDataOf(HWND hwnd)
{
    return (void *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

void QWindowsWindow::setUserDataOf(HWND hwnd, void *ud)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(ud));
}

inline void QWindowsWindow::destroyIcon()
{
    if (m_iconBig) {
        DestroyIcon(m_iconBig);
        m_iconBig = 0;
    }
    if (m_iconSmall) {
        DestroyIcon(m_iconSmall);
        m_iconSmall = 0;
    }
}

inline bool QWindowsWindow::isLayered() const
{
    return GetWindowLongPtr(m_data.hwnd, GWL_EXSTYLE) & WS_EX_LAYERED;
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QMargins)

#endif // QWINDOWSWINDOW_H
