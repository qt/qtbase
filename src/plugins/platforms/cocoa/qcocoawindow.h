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
#include "qt_mac_p.h"

QT_FORWARD_DECLARE_CLASS(QCocoaWindow)

QT_BEGIN_NAMESPACE

class QCocoaWindowPointer
{
public:
    void assign(QCocoaWindow *w);
    void clear();

    QCocoaWindow *data() const
    { return watcher.isNull() ? Q_NULLPTR : window; }
    bool isNull() const
    { return watcher.isNull(); }
    operator QCocoaWindow*() const
    { return data(); }
    QCocoaWindow *operator->() const
    { return data(); }
    QCocoaWindow &operator*() const
    { return *data(); }

private:
    QPointer<QObject> watcher;
    QCocoaWindow *window;
};

QT_END_NAMESPACE

@class QT_MANGLE_NAMESPACE(QNSWindowHelper);

@protocol QNSWindowProtocol

@property (nonatomic, readonly) QT_MANGLE_NAMESPACE(QNSWindowHelper) *helper;

- (void)superSendEvent:(NSEvent *)theEvent;
- (void)closeAndRelease;

@end

typedef NSWindow<QNSWindowProtocol> QCocoaNSWindow;

@interface QT_MANGLE_NAMESPACE(QNSWindowHelper) : NSObject
{
    QCocoaNSWindow *_window;
    QCocoaWindowPointer _platformWindow;
    BOOL _grabbingMouse;
    BOOL _releaseOnMouseUp;
}

@property (nonatomic, readonly) QCocoaNSWindow *window;
@property (nonatomic, readonly) QCocoaWindowPointer platformWindow;
@property (nonatomic) BOOL grabbingMouse;
@property (nonatomic) BOOL releaseOnMouseUp;

- (id)initWithNSWindow:(QCocoaNSWindow *)window platformWindow:(QCocoaWindow *)platformWindow;
- (void)handleWindowEvent:(NSEvent *)theEvent;
- (void) clearWindow;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSWindowHelper);

@interface QT_MANGLE_NAMESPACE(QNSWindow) : NSWindow<QNSWindowProtocol>
{
    QNSWindowHelper *_helper;
}

@property (nonatomic, readonly) QNSWindowHelper *helper;

- (id)initWithContentRect:(NSRect)contentRect
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSWindow);

@interface QT_MANGLE_NAMESPACE(QNSPanel) : NSPanel<QNSWindowProtocol>
{
    QNSWindowHelper *_helper;
}

@property (nonatomic, readonly) QNSWindowHelper *helper;

- (id)initWithContentRect:(NSRect)contentRect
      styleMask:(NSUInteger)windowStyle
      qPlatformWindow:(QCocoaWindow *)qpw;

@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSPanel);

@class QT_MANGLE_NAMESPACE(QNSWindowDelegate);

QT_BEGIN_NAMESPACE
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

class QCocoaMenuBar;

class QCocoaWindow : public QPlatformWindow
{
public:
    QCocoaWindow(QWindow *tlw);
    ~QCocoaWindow();

    void setGeometry(const QRect &rect) Q_DECL_OVERRIDE;
    QRect geometry() const Q_DECL_OVERRIDE;
    void setCocoaGeometry(const QRect &rect);
    void clipChildWindows();
    void clipWindow(const NSRect &clipRect);
    void show(bool becauseOfAncestor = false);
    void hide(bool becauseOfAncestor = false);
    void setVisible(bool visible) Q_DECL_OVERRIDE;
    void setWindowFlags(Qt::WindowFlags flags) Q_DECL_OVERRIDE;
    void setWindowState(Qt::WindowState state) Q_DECL_OVERRIDE;
    void setWindowTitle(const QString &title) Q_DECL_OVERRIDE;
    void setWindowFilePath(const QString &filePath) Q_DECL_OVERRIDE;
    void setWindowIcon(const QIcon &icon) Q_DECL_OVERRIDE;
    void setAlertState(bool enabled) Q_DECL_OVERRIDE;
    bool isAlertState() const Q_DECL_OVERRIDE;
    void raise() Q_DECL_OVERRIDE;
    void lower() Q_DECL_OVERRIDE;
    bool isExposed() const Q_DECL_OVERRIDE;
    bool isOpaque() const;
    void propagateSizeHints() Q_DECL_OVERRIDE;
    void setOpacity(qreal level) Q_DECL_OVERRIDE;
    void setMask(const QRegion &region) Q_DECL_OVERRIDE;
    bool setKeyboardGrabEnabled(bool grab) Q_DECL_OVERRIDE;
    bool setMouseGrabEnabled(bool grab) Q_DECL_OVERRIDE;
    QMargins frameMargins() const Q_DECL_OVERRIDE;
    QSurfaceFormat format() const Q_DECL_OVERRIDE;

    void requestActivateWindow() Q_DECL_OVERRIDE;

    WId winId() const Q_DECL_OVERRIDE;
    void setParent(const QPlatformWindow *window) Q_DECL_OVERRIDE;

    NSView *contentView() const;
    void setContentView(NSView *contentView);
    QNSView *qtView() const;
    NSWindow *nativeWindow() const;

    void setEmbeddedInForeignView(bool subwindow);

    void windowWillMove();
    void windowDidMove();
    void windowDidResize();
    void windowDidEndLiveResize();
    bool windowShouldClose();
    bool windowIsPopupType(Qt::WindowType type = Qt::Widget) const;

    void setSynchedWindowStateFromWindow();

    NSInteger windowLevel(Qt::WindowFlags flags);
    NSUInteger windowStyleMask(Qt::WindowFlags flags);
    void setWindowShadow(Qt::WindowFlags flags);
    void setWindowZoomButton(Qt::WindowFlags flags);

#ifndef QT_NO_OPENGL
    void setCurrentContext(QCocoaGLContext *context);
    QCocoaGLContext *currentContext() const;
#endif

    bool setWindowModified(bool modified) Q_DECL_OVERRIDE;

    void setFrameStrutEventsEnabled(bool enabled) Q_DECL_OVERRIDE;
    bool frameStrutEventsEnabled() const Q_DECL_OVERRIDE
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
    void applyContentBorderThickness(NSWindow *window);
    void updateNSToolbar();

    qreal devicePixelRatio() const Q_DECL_OVERRIDE;
    bool isWindowExposable();
    void exposeWindow();
    void obscureWindow();
    void updateExposedGeometry();
    QWindow *childWindowAt(QPoint windowPoint);
    bool shouldRefuseKeyWindowAndFirstResponder();

    static QPoint bottomLeftClippedByNSWindowOffsetStatic(QWindow *window);
    QPoint bottomLeftClippedByNSWindowOffset() const;
protected:
    void recreateWindow(const QPlatformWindow *parentWindow);
    QCocoaNSWindow *createNSWindow();
    void setNSWindow(QCocoaNSWindow *window);

    bool shouldUseNSPanel();

    QRect nativeWindowGeometry() const;
    QCocoaWindow *parentCocoaWindow() const;
    void syncWindowState(Qt::WindowState newState);
    void reinsertChildWindow(QCocoaWindow *child);
    void removeChildWindow(QCocoaWindow *child);

// private:
public: // for QNSView
    friend class QCocoaBackingStore;
    friend class QCocoaNativeInterface;

    void removeMonitor();

    NSView *m_contentView;
    QNSView *m_qtView;
    QCocoaNSWindow *m_nsWindow;
    QCocoaWindowPointer m_forwardWindow;

    // TODO merge to one variable if possible
    bool m_contentViewIsEmbedded; // true if the m_contentView is actually embedded in a "foreign" NSView hiearchy
    bool m_contentViewIsToBeEmbedded; // true if the m_contentView is intended to be embedded in a "foreign" NSView hiearchy

    QCocoaWindow *m_parentCocoaWindow;
    bool m_isNSWindowChild; // this window is a non-top level QWindow with a NSWindow.
    QList<QCocoaWindow *> m_childWindows;

    Qt::WindowFlags m_windowFlags;
    bool m_effectivelyMaximized;
    Qt::WindowState m_synchedWindowState;
    Qt::WindowModality m_windowModality;
    QPointer<QWindow> m_enterLeaveTargetWindow;
    bool m_windowUnderMouse;

    bool m_inConstructor;
    bool m_inSetVisible;
    bool m_inSetGeometry;
    bool m_inSetStyleMask;
#ifndef QT_NO_OPENGL
    QCocoaGLContext *m_glContext;
#endif
    QCocoaMenuBar *m_menubar;
    NSCursor *m_windowCursor;

    bool m_hasModalSession;
    bool m_frameStrutEventsEnabled;
    bool m_geometryUpdateExposeAllowed;
    bool m_isExposed;
    QRect m_exposedGeometry;
    qreal m_exposedDevicePixelRatio;
    int m_registerTouchCount;
    bool m_resizableTransientParent;
    bool m_hiddenByClipping;
    bool m_hiddenByAncestor;

    static const int NoAlertRequest;
    NSInteger m_alertRequest;
    id monitor;

    bool m_drawContentBorderGradient;
    int m_topContentBorderThickness;
    int m_bottomContentBorderThickness;

    // used by showFullScreen in fake mode
    QRect m_normalGeometry;
    Qt::WindowFlags m_oldWindowFlags;
    NSApplicationPresentationOptions m_presentationOptions;

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

    // This object is tracked by QCocoaWindowPointer,
    // preventing the use of dangling pointers.
    QObject sentinel;
};

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H

