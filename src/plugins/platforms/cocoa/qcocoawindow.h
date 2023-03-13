// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAWINDOW_H
#define QCOCOAWINDOW_H

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformwindow_p.h>
#include <QRect>
#include <QPointer>

#ifndef QT_NO_OPENGL
#include "qcocoaglcontext.h"
#endif
#include "qnsview.h"
#include "qnswindow.h"

#if QT_CONFIG(vulkan)
#include <MoltenVK/mvk_vulkan.h>
#endif

#include <QHash>

Q_FORWARD_DECLARE_OBJC_CLASS(NSWindow);
Q_FORWARD_DECLARE_OBJC_CLASS(NSView);
Q_FORWARD_DECLARE_OBJC_CLASS(NSCursor);

#if !defined(__OBJC__)
using NSInteger = long;
using NSUInteger = unsigned long;
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
#endif

// QCocoaWindow
//
// QCocoaWindow is an NSView (not an NSWindow!) in the sense
// that it relies on an NSView for all event handling and
// graphics output and does not require an NSWindow, except for
// the window-related functions like setWindowTitle.
//
// As a consequence of this it is possible to embed the QCocoaWindow
// in an NSView hierarchy by getting a pointer to the "backing"
// NSView and not calling QCocoaWindow::show():
//
// QWindow *qtWindow = new MyWindow();
// qtWindow->create();
// QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
// NSView *qtView = (NSView *)platformNativeInterface->nativeResourceForWindow("nsview", qtWindow);
// [parentView addSubview:qtView];
//
// See the qt_on_cocoa manual tests for a working example, located
// in tests/manual/cocoa at the time of writing.

#ifdef Q_MOC_RUN
#define Q_NOTIFICATION_HANDLER(notification) Q_INVOKABLE Q_COCOA_NOTIFICATION_##notification
#else
#define Q_NOTIFICATION_HANDLER(notification)
#define Q_NOTIFICATION_PREFIX QT_STRINGIFY2(Q_COCOA_NOTIFICATION_)
#endif

class QCocoaMenuBar;

class QCocoaWindow : public QObject, public QPlatformWindow,
    public QNativeInterface::Private::QCocoaWindow
{
    Q_OBJECT
public:
    QCocoaWindow(QWindow *tlw, WId nativeHandle = 0);
    ~QCocoaWindow();

    void initialize() override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
    QRect normalGeometry() const override;
    void setCocoaGeometry(const QRect &rect);

    void setVisible(bool visible) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    void setWindowState(Qt::WindowStates state) override;
    void setWindowTitle(const QString &title) override;
    void setWindowFilePath(const QString &filePath) override;
    void setWindowIcon(const QIcon &icon) override;
    void setAlertState(bool enabled) override;
    bool isAlertState() const override;
    void raise() override;
    void lower() override;
    bool isExposed() const override;
    bool isEmbedded() const override;
    bool isOpaque() const;
    void propagateSizeHints() override;
    void setOpacity(qreal level) override;
    void setMask(const QRegion &region) override;
    bool setKeyboardGrabEnabled(bool grab) override;
    bool setMouseGrabEnabled(bool grab) override;
    QMargins frameMargins() const override;
    QSurfaceFormat format() const override;

    bool isForeignWindow() const override;

    void requestUpdate() override;
    bool updatesWithDisplayLink() const;
    void deliverUpdateRequest() override;

    void requestActivateWindow() override;

    WId winId() const override;
    void setParent(const QPlatformWindow *window) override;

    NSView *view() const;
    NSWindow *nativeWindow() const;

    void setEmbeddedInForeignView();

    Q_NOTIFICATION_HANDLER(NSViewFrameDidChangeNotification) void viewDidChangeFrame();
    Q_NOTIFICATION_HANDLER(NSViewGlobalFrameDidChangeNotification) void viewDidChangeGlobalFrame();

    Q_NOTIFICATION_HANDLER(NSWindowDidMoveNotification) void windowDidMove();
    Q_NOTIFICATION_HANDLER(NSWindowDidResizeNotification) void windowDidResize();
    Q_NOTIFICATION_HANDLER(NSWindowDidEndLiveResizeNotification) void windowDidEndLiveResize();
    Q_NOTIFICATION_HANDLER(NSWindowDidBecomeKeyNotification) void windowDidBecomeKey();
    Q_NOTIFICATION_HANDLER(NSWindowDidResignKeyNotification) void windowDidResignKey();
    Q_NOTIFICATION_HANDLER(NSWindowDidMiniaturizeNotification) void windowDidMiniaturize();
    Q_NOTIFICATION_HANDLER(NSWindowDidDeminiaturizeNotification) void windowDidDeminiaturize();
    Q_NOTIFICATION_HANDLER(NSWindowWillEnterFullScreenNotification) void windowWillEnterFullScreen();
    Q_NOTIFICATION_HANDLER(NSWindowDidEnterFullScreenNotification) void windowDidEnterFullScreen();
    Q_NOTIFICATION_HANDLER(NSWindowWillExitFullScreenNotification) void windowWillExitFullScreen();
    Q_NOTIFICATION_HANDLER(NSWindowDidExitFullScreenNotification) void windowDidExitFullScreen();
    Q_NOTIFICATION_HANDLER(NSWindowDidOrderOnScreenAndFinishAnimatingNotification) void windowDidOrderOnScreen();
    Q_NOTIFICATION_HANDLER(NSWindowDidOrderOffScreenNotification) void windowDidOrderOffScreen();
    Q_NOTIFICATION_HANDLER(NSWindowDidChangeOcclusionStateNotification) void windowDidChangeOcclusionState();
    Q_NOTIFICATION_HANDLER(NSWindowDidChangeScreenNotification) void windowDidChangeScreen();

    void windowWillZoom();

    bool windowShouldClose();
    bool windowIsPopupType(Qt::WindowType type = Qt::Widget) const;

    NSInteger windowLevel(Qt::WindowFlags flags);
    NSUInteger windowStyleMask(Qt::WindowFlags flags);
    void updateTitleBarButtons(Qt::WindowFlags flags);
    bool isFixedSize() const;

    bool setWindowModified(bool modified) override;

    void setFrameStrutEventsEnabled(bool enabled) override;
    bool frameStrutEventsEnabled() const override
        { return m_frameStrutEventsEnabled; }

    void setMenubar(QCocoaMenuBar *mb);
    QCocoaMenuBar *menubar() const;

    void setWindowCursor(NSCursor *cursor);

    void registerTouch(bool enable);

    void registerContentBorderArea(quintptr identifier, int upper, int lower);
    void setContentBorderAreaEnabled(quintptr identifier, bool enable);
    void setContentBorderEnabled(bool enable) override;
    bool testContentBorderAreaPosition(int position) const;
    void applyContentBorderThickness(NSWindow *window = nullptr);

    qreal devicePixelRatio() const override;
    QWindow *childWindowAt(QPoint windowPoint);
    bool shouldRefuseKeyWindowAndFirstResponder();

    bool windowEvent(QEvent *event) override;

    QPoint bottomLeftClippedByNSWindowOffset() const override;

    void updateNormalGeometry();

    enum RecreationReason {
        RecreationNotNeeded = 0,
        ParentChanged = 0x1,
        MissingWindow = 0x2,
        WindowModalityChanged = 0x4,
        ContentViewChanged = 0x10,
        PanelChanged = 0x20,
    };
    Q_DECLARE_FLAGS(RecreationReasons, RecreationReason)
    Q_FLAG(RecreationReasons)

protected:
    void recreateWindowIfNeeded();
    QCocoaNSWindow *createNSWindow(bool shouldBePanel);

    Qt::WindowStates windowState() const;
    void applyWindowState(Qt::WindowStates newState);
    void toggleMaximized();
    void toggleFullScreen();
    bool isTransitioningToFullScreen() const;

    bool startSystemMove() override;

// private:
public: // for QNSView
    friend class QCocoaBackingStore;
    friend class QCocoaNativeInterface;

    bool isContentView() const;

    bool alwaysShowToolWindow() const;

    enum HandleFlags {
        NoHandleFlags = 0,
        HandleUnconditionally = 1
    };

    void handleGeometryChange();
    void handleWindowStateChanged(HandleFlags flags = NoHandleFlags);
    void handleExposeEvent(const QRegion &region);

    static void closeAllPopups();
    static void setupPopupMonitor();
    static void removePopupMonitor();

    NSView *m_view = nil;
    QCocoaNSWindow *m_nsWindow = nil;

    Qt::WindowStates m_lastReportedWindowState = Qt::WindowNoState;
    Qt::WindowModality m_windowModality = Qt::NonModal;

    static QPointer<QCocoaWindow> s_windowUnderMouse;

    bool m_initialized = false;
    bool m_inSetVisible = false;
    bool m_inSetGeometry = false;
    bool m_inSetStyleMask = false;

    QCocoaMenuBar *m_menubar = nullptr;

    bool m_frameStrutEventsEnabled = false;
    QRect m_exposedRect;
    QRect m_normalGeometry;
    int m_registerTouchCount = 0;
    bool m_resizableTransientParent = false;

    static const int NoAlertRequest;
    NSInteger m_alertRequest = NoAlertRequest;

    bool m_drawContentBorderGradient = false;

    struct BorderRange {
        BorderRange(quintptr i, int u, int l) : identifier(i), upper(u), lower(l) { }
        quintptr identifier;
        int upper;
        int lower;
        bool operator<(BorderRange const& right) const {
              return upper < right.upper;
        }
    };
    QHash<quintptr, BorderRange> m_contentBorderAreas; // identifier -> uppper/lower
    QHash<quintptr, bool> m_enabledContentBorderAreas; // identifier -> enabled state (true/false)

    static inline id s_globalMouseMonitor = 0;
    static inline id s_applicationActivationObserver = 0;

#if QT_CONFIG(vulkan)
    VkSurfaceKHR m_vulkanSurface = nullptr;
#endif
};

extern const NSNotificationName QCocoaWindowWillReleaseQNSViewNotification;

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaWindow *window);
#endif

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H

