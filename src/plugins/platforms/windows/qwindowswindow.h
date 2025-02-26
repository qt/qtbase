// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSWINDOW_H
#define QWINDOWSWINDOW_H

#include <QtCore/qt_windows.h>
#include <QtCore/qpointer.h>
#include "qwindowsapplication.h"
#include "qwindowscursor.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>

#if QT_CONFIG(vulkan)
#include "qwindowsvulkaninstance.h"
#endif

#include <optional>

QT_BEGIN_NAMESPACE

class QWindowsOleDropTarget;
class QWindowsMenuBar;
class QDebug;

struct QWindowsGeometryHint
{
    static QMargins frameOnPrimaryScreen(const QWindow *w, DWORD style, DWORD exStyle);
    static QMargins frameOnPrimaryScreen(const QWindow *w, HWND hwnd);
    static QMargins frame(const QWindow *w, DWORD style, DWORD exStyle, qreal dpi);
    static QMargins frame(const QWindow *w, HWND hwnd, DWORD style, DWORD exStyle);
    static QMargins frame(const QWindow *w, HWND hwnd);
    static QMargins frame(const QWindow *w, const QRect &geometry,
                          DWORD style, DWORD exStyle);
    static bool handleCalculateSize(const QWindow *window, const QMargins &customMargins, const MSG &msg, LRESULT *result);
    static void applyToMinMaxInfo(const QWindow *w, const QScreen *screen,
                                  const QMargins &margins, MINMAXINFO *mmi);
    static void applyToMinMaxInfo(const QWindow *w, const QMargins &margins,
                                  MINMAXINFO *mmi);
    static void frameSizeConstraints(const QWindow *w, const QScreen *screen,
                                     const QMargins &margins,
                                     QSize *minimumSize, QSize *maximumSize);
    static inline QPoint mapToGlobal(HWND hwnd, const QPoint &);
    static inline QPoint mapToGlobal(const QWindow *w, const QPoint &);
    static inline QPoint mapFromGlobal(const HWND hwnd, const QPoint &);
    static inline QPoint mapFromGlobal(const QWindow *w, const QPoint &);

    static bool positionIncludesFrame(const QWindow *w);
};

struct QWindowCreationContext
{
    explicit QWindowCreationContext(const QWindow *w, const QScreen *s,
                                    const QRect &geometryIn, const QRect &geometry,
                                    const QMargins &customMargins,
                                    DWORD style, DWORD exStyle);
    void applyToMinMaxInfo(MINMAXINFO *mmi) const;

    const QWindow *window;
    // The screen to use to scale size constraints, etc. Might differ from the
    // screen of the window after QPlatformWindow::initialGeometry() (QTBUG-77307).
    const QScreen *screen;
    QRect requestedGeometryIn; // QWindow scaled
    QRect requestedGeometry; // after QPlatformWindow::initialGeometry()
    QPoint obtainedPos;
    QSize obtainedSize;
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
    QRect restoreGeometry;
    QMargins fullFrameMargins; // Do not use directly for windows, see FrameDirty.
    QMargins customMargins;    // User-defined, additional frame for NCCALCSIZE
    HWND hwnd = nullptr;
    HWND hwndTitlebar = nullptr;
    bool embedded = false;
    bool hasFrame = false;

    static QWindowsWindowData create(const QWindow *w,
                                     const QWindowsWindowData &parameters,
                                     const QString &title);
};

class QWindowsBaseWindow : public QPlatformWindow,
                           public QNativeInterface::Private::QWindowsWindow
{
    Q_DISABLE_COPY_MOVE(QWindowsBaseWindow)
public:
    using TouchWindowTouchType = QNativeInterface::Private::QWindowsApplication::TouchWindowTouchType;
    using TouchWindowTouchTypes = QNativeInterface::Private::QWindowsApplication::TouchWindowTouchTypes;

    explicit QWindowsBaseWindow(QWindow *window) : QPlatformWindow(window) {}

    WId winId() const override { return WId(handle()); }
    QRect geometry() const override { return geometry_sys(); }
    QMargins frameMargins() const override { return fullFrameMargins(); }
    QPoint mapToGlobal(const QPoint &pos) const override;
    QPoint mapFromGlobal(const QPoint &pos) const override;
    virtual QMargins fullFrameMargins() const { return frameMargins_sys(); }

    void setHasBorderInFullScreen(bool border) override;
    bool hasBorderInFullScreen() const override;

    QMargins customMargins() const override;
    void setCustomMargins(const QMargins &margins) override;

    using QPlatformWindow::screenForGeometry;

    virtual HWND handle() const = 0;
    virtual bool isTopLevel() const { return isTopLevel_sys(); }

    unsigned style() const   { return GetWindowLongPtr(handle(), GWL_STYLE); }
    unsigned exStyle() const { return GetWindowLongPtr(handle(), GWL_EXSTYLE); }
    static bool isRtlLayout(HWND hwnd);

    static QWindowsBaseWindow *baseWindowOf(const QWindow *w);
    static HWND handleOf(const QWindow *w);

    bool windowEvent(QEvent *event) override;

protected:
    HWND parentHwnd() const { return GetAncestor(handle(), GA_PARENT); }
    bool isTopLevel_sys() const;
    inline bool hasMaximumHeight() const;
    inline bool hasMaximumWidth() const;
    inline bool hasMaximumSize() const;
    QRect frameGeometry_sys() const;
    QRect geometry_sys() const;
    void setGeometry_sys(const QRect &rect) const;
    QMargins frameMargins_sys() const;
    std::optional<TouchWindowTouchTypes> touchWindowTouchTypes_sys() const;
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
    ~QWindowsForeignWindow();

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
        VulkanSurface = 0x400000,
        ResizeMoveActive = 0x800000,
        DisableNonClientScaling = 0x1000000,
        Direct3DSurface = 0x2000000,
        RestoreOverrideCursor = 0x4000000
    };

    QWindowsWindow(QWindow *window, const QWindowsWindowData &data);
    ~QWindowsWindow() override;

    void initialize() override;

    using QPlatformWindow::screenForGeometry;

    QSurfaceFormat format() const override;
    void setGeometry(const QRect &rect) override;
    QRect geometry() const override { return m_data.geometry; }
    QRect normalGeometry() const override;
    QMargins safeAreaMargins() const override;
    QRect restoreGeometry() const { return m_data.restoreGeometry; }
    void updateRestoreGeometry();

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
    QString windowTitle() const override;
    void raise() override { raise_sys(); }
    void lower() override { lower_sys(); }

    bool windowEvent(QEvent *event) override;

    void propagateSizeHints() override;
    static bool handleGeometryChangingMessage(MSG *message, const QWindow *qWindow, const QMargins &marginsDp);
    bool handleGeometryChanging(MSG *message) const;
    QMargins frameMargins() const override;
    QMargins fullFrameMargins() const override;
    void setFullFrameMargins(const QMargins &newMargins);
    void updateFullFrameMargins();

    void setOpacity(qreal level) override;
    void setMask(const QRegion &region) override;
    qreal opacity() const { return m_opacity; }
    void requestActivateWindow() override;

    bool setKeyboardGrabEnabled(bool grab) override;
    bool setMouseGrabEnabled(bool grab) override;
    inline bool hasMouseCapture() const { return GetCapture() == m_data.hwnd; }

    bool startSystemResize(Qt::Edges edges) override;
    bool startSystemMove() override;

    void setFrameStrutEventsEnabled(bool enabled) override;
    bool frameStrutEventsEnabled() const override { return testFlag(FrameStrutEventsEnabled); }

    // QWindowsBaseWindow overrides
    HWND handle() const override { return m_data.hwnd; }
    bool isTopLevel() const override;

    static bool setDarkBorderToWindow(HWND hwnd, bool d);
    void setDarkBorder(bool d);

    QWindowsMenuBar *menuBar() const;
    void setMenuBar(QWindowsMenuBar *mb);

    QMargins customMargins() const override;
    void setCustomMargins(const QMargins &m) override;

    void setStyle(unsigned s) const;
    void setExStyle(unsigned s) const;

    bool handleWmPaint(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result);

    void handleMoved();
    void handleResized(int wParam, LPARAM lParam);
    void handleHidden();
    void handleCompositionSettingsChanged();
    void handleDpiScaledSize(WPARAM wParam, LPARAM lParam, LRESULT *result);
    void handleDpiChanged(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void handleDpiChangedAfterParent(HWND hwnd);

    static void displayChanged();
    static void settingsChanged();
    static QScreen *forcedScreenForGLWindow(const QWindow *w);
    static QWindowsWindow *windowsWindowOf(const QWindow *w);
    static QWindow *topLevelOf(QWindow *w);
    static inline void *userDataOf(HWND hwnd);
    static inline void setUserDataOf(HWND hwnd, void *ud);

    static bool hasNoNativeFrame(HWND hwnd, Qt::WindowFlags flags);
    static bool setWindowLayered(HWND hwnd, Qt::WindowFlags flags, bool hasAlpha, qreal opacity);
    bool isLayered() const;

    HDC getDC();
    void releaseDC();
    void getSizeHints(MINMAXINFO *mmi) const;
    bool handleNonClientHitTest(const QPoint &globalPos, LRESULT *result) const;
    bool handleNonClientActivate(LRESULT *result) const;
    void updateCustomTitlebar();

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

    void setAlertState(bool enabled) override;
    bool isAlertState() const override { return testFlag(AlertState); }
    void alertWindow(int durationMs = 0);
    void stopAlertWindow();

    enum ScreenChangeMode { FromGeometryChange, FromDpiChange, FromScreenAdded };
    void checkForScreenChanged(ScreenChangeMode mode = FromGeometryChange);

    void registerTouchWindow();
    static void setHasBorderInFullScreenStatic(QWindow *window, bool border);
    static void setHasBorderInFullScreenDefault(bool border);
    void setHasBorderInFullScreen(bool border) override;
    bool hasBorderInFullScreen() const override;
    static QString formatWindowTitle(const QString &title);

    static const char *embeddedNativeParentHandleProperty;
    static const char *hasBorderInFullScreenProperty;

    void setSavedDpi(int dpi) { m_savedDpi = dpi; }
    int savedDpi() const { return m_savedDpi; }
    qreal dpiRelativeScale(const UINT dpi) const;

    bool isClientAreaExpanded() const { return m_data.flags.testFlag(Qt::ExpandedClientAreaHint); }

    void requestUpdate() override;

private:
    inline void show_sys() const;
    inline QWindowsWindowData setWindowFlags_sys(Qt::WindowFlags wt, unsigned flags = 0) const;
    inline bool isFullScreen_sys() const;
    inline void setWindowState_sys(Qt::WindowStates newState);
    inline void setParent_sys(const QPlatformWindow *parent);
    inline void updateTransientParent() const;
    void destroyWindow();
    inline bool isDropSiteEnabled() const { return m_dropTarget != nullptr; }
    void setDropSiteEnabled(bool enabled);
    void updateDropSite(bool topLevel);
    void handleGeometryChange();
    void handleWindowStateChange(Qt::WindowStates state);
    inline void destroyIcon();
    void fireExpose(const QRegion &region, bool force=false);
    void fireFullExpose(bool force=false);
    void calculateFullFrameMargins();
    void correctWindowPlacement(WINDOWPLACEMENT &windowPlacement);

    mutable QWindowsWindowData m_data;
    QPointer<QWindowsMenuBar> m_menuBar;
    mutable unsigned m_flags = WithinCreate;
    HDC m_hdc = nullptr;
    Qt::WindowStates m_windowState = Qt::WindowNoState;
    bool m_windowWasArranged = false;
    QString m_windowTitle;
    qreal m_opacity = 1;
#ifndef QT_NO_CURSOR
    CursorHandlePtr m_cursor;
#endif
    QWindowsOleDropTarget *m_dropTarget = nullptr;
    unsigned m_savedStyle = 0;
    QRect m_savedFrameGeometry;
    HICON m_iconSmall = nullptr;
    HICON m_iconBig = nullptr;
    void *m_surface = nullptr;
    int m_savedDpi = 96;

    static bool m_screenForGLInitialized;

#if QT_CONFIG(vulkan)
    // note: intentionally not using void * in order to avoid breaking x86
    VkSurfaceKHR m_vkSurface = VK_NULL_HANDLE;
#endif
    static bool m_borderInFullScreenDefault;
    static bool m_inSetgeometry;

    qsizetype m_vsyncServiceCallbackId = 0;
    QAtomicInt m_vsyncUpdatePending;
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

static inline void clientToScreen(HWND hwnd, POINT *wP)
{
    if (QWindowsBaseWindow::isRtlLayout(hwnd)) {
        RECT clientArea;
        GetClientRect(hwnd, &clientArea);
        wP->x = clientArea.right - wP->x;
    }
    ClientToScreen(hwnd, wP);
}

static inline void screenToClient(HWND hwnd, POINT *wP)
{
    ScreenToClient(hwnd, wP);
    if (QWindowsBaseWindow::isRtlLayout(hwnd)) {
        RECT clientArea;
        GetClientRect(hwnd, &clientArea);
        wP->x = clientArea.right - wP->x;
    }
}

// ---------- QWindowsGeometryHint inline functions.
QPoint QWindowsGeometryHint::mapToGlobal(HWND hwnd, const QPoint &qp)
{
    POINT p = { qp.x(), qp.y() };
    clientToScreen(hwnd, &p);
    return QPoint(p.x, p.y);
}

QPoint QWindowsGeometryHint::mapFromGlobal(const HWND hwnd, const QPoint &qp)
{
    POINT p = { qp.x(), qp.y() };
    screenToClient(hwnd, &p);
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
    return reinterpret_cast<void *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void QWindowsWindow::setUserDataOf(HWND hwnd, void *ud)
{
    SetWindowLongPtr(hwnd, GWLP_USERDATA, LONG_PTR(ud));
}

inline void QWindowsWindow::destroyIcon()
{
    if (m_iconBig) {
        DestroyIcon(m_iconBig);
        m_iconBig = nullptr;
    }
    if (m_iconSmall) {
        DestroyIcon(m_iconSmall);
        m_iconSmall = nullptr;
    }
}

inline bool QWindowsWindow::isLayered() const
{
    return GetWindowLongPtr(m_data.hwnd, GWL_EXSTYLE) & WS_EX_LAYERED;
}

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QMargins)

#endif // QWINDOWSWINDOW_H
