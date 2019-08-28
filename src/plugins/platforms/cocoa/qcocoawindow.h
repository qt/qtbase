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

#ifndef QCOCOAWINDOW_H
#define QCOCOAWINDOW_H

#include <AppKit/AppKit.h>

#include <qpa/qplatformwindow.h>
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

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
class QDebug;
#endif

// QCocoaWindow
//
// QCocoaWindow is an NSView (not an NSWindow!) in the sense
// that it relies on a NSView for all event handling and
// graphics output and does not require a NSWindow, except for
// for the window-related functions like setWindowTitle.
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

class QCocoaWindow : public QObject, public QPlatformWindow
{
    Q_OBJECT
public:
    QCocoaWindow(QWindow *tlw, WId nativeHandle = 0);
    ~QCocoaWindow();

    void initialize() override;

    void setGeometry(const QRect &rect) override;
    QRect geometry() const override;
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

    Q_NOTIFICATION_HANDLER(NSWindowWillMoveNotification) void windowWillMove();
    Q_NOTIFICATION_HANDLER(NSWindowDidMoveNotification) void windowDidMove();
    Q_NOTIFICATION_HANDLER(NSWindowDidResizeNotification) void windowDidResize();
    Q_NOTIFICATION_HANDLER(NSWindowDidEndLiveResizeNotification) void windowDidEndLiveResize();
    Q_NOTIFICATION_HANDLER(NSWindowDidBecomeKeyNotification) void windowDidBecomeKey();
    Q_NOTIFICATION_HANDLER(NSWindowDidResignKeyNotification) void windowDidResignKey();
    Q_NOTIFICATION_HANDLER(NSWindowWillMiniaturizeNotification) void windowWillMiniaturize();
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
    Q_NOTIFICATION_HANDLER(NSWindowWillCloseNotification) void windowWillClose();

    bool windowShouldClose();
    bool windowIsPopupType(Qt::WindowType type = Qt::Widget) const;

    NSInteger windowLevel(Qt::WindowFlags flags);
    NSUInteger windowStyleMask(Qt::WindowFlags flags);
    void setWindowZoomButton(Qt::WindowFlags flags);

    bool setWindowModified(bool modified) override;

    void setFrameStrutEventsEnabled(bool enabled) override;
    bool frameStrutEventsEnabled() const override
        { return m_frameStrutEventsEnabled; }

    void setMenubar(QCocoaMenuBar *mb);
    QCocoaMenuBar *menubar() const;

    void setWindowCursor(NSCursor *cursor);

    void registerTouch(bool enable);
    void setContentBorderThickness(int topThickness, int bottomThickness);
    void registerContentBorderArea(quintptr identifier, int upper, int lower);
    void setContentBorderAreaEnabled(quintptr identifier, bool enable);
    void setContentBorderEnabled(bool enable);
    bool testContentBorderAreaPosition(int position) const;
    void applyContentBorderThickness(NSWindow *window = nullptr);
    void updateNSToolbar();

    qreal devicePixelRatio() const override;
    QWindow *childWindowAt(QPoint windowPoint);
    bool shouldRefuseKeyWindowAndFirstResponder();

    static QPoint bottomLeftClippedByNSWindowOffsetStatic(QWindow *window);
    QPoint bottomLeftClippedByNSWindowOffset() const;

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

    Qt::WindowState windowState() const;
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
    void removeMonitor();

    enum HandleFlags {
        NoHandleFlags = 0,
        HandleUnconditionally = 1
    };

    void handleGeometryChange();
    void handleWindowStateChanged(HandleFlags flags = NoHandleFlags);
    void handleExposeEvent(const QRegion &region);

    NSView *m_view;
    QCocoaNSWindow *m_nsWindow;

    Qt::WindowStates m_lastReportedWindowState;
    Qt::WindowModality m_windowModality;
    QPointer<QWindow> m_enterLeaveTargetWindow;
    bool m_windowUnderMouse;

    bool m_initialized;
    bool m_inSetVisible;
    bool m_inSetGeometry;
    bool m_inSetStyleMask;
    QCocoaMenuBar *m_menubar;

    bool m_needsInvalidateShadow;

    bool m_frameStrutEventsEnabled;
    QRect m_exposedRect;
    int m_registerTouchCount;
    bool m_resizableTransientParent;

    static const int NoAlertRequest;
    NSInteger m_alertRequest;
    id monitor;

    bool m_drawContentBorderGradient;
    int m_topContentBorderThickness;
    int m_bottomContentBorderThickness;

    struct BorderRange {
        BorderRange(quintptr i, int u, int l) : identifier(i), upper(u), lower(l) { }
        quintptr identifier;
        int upper;
        int lower;
        bool operator<(BorderRange const& right) const {
              return upper < right.upper;
        }
    };
    QHash<quintptr, BorderRange> m_contentBorderAreas; // identifer -> uppper/lower
    QHash<quintptr, bool> m_enabledContentBorderAreas; // identifer -> enabled state (true/false)

#if QT_CONFIG(vulkan)
    VkSurfaceKHR m_vulkanSurface = nullptr;
#endif
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaWindow *window);
#endif

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H

