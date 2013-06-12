/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOCOAWINDOW_H
#define QCOCOAWINDOW_H

#include <Cocoa/Cocoa.h>

#include <qpa/qplatformwindow.h>
#include <QRect>

#include "qcocoaglcontext.h"
#include "qnsview.h"

QT_FORWARD_DECLARE_CLASS(QCocoaWindow)

@interface QNSWindow : NSWindow {
    @public QCocoaWindow *m_cocoaPlatformWindow;
}

- (void)clearPlatformWindow;
- (BOOL)canBecomeKeyWindow;
@end

@interface QNSPanel : NSPanel {
    @public QT_PREPEND_NAMESPACE(QCocoaWindow) *m_cocoaPlatformWindow;
}
- (void)clearPlatformWindow;
- (BOOL)canBecomeKeyWindow;
@end

@class QNSWindowDelegate;

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

    void setGeometry(const QRect &rect);
    void setCocoaGeometry(const QRect &rect);
    void setVisible(bool visible);
    void setWindowFlags(Qt::WindowFlags flags);
    void setWindowState(Qt::WindowState state);
    void setWindowTitle(const QString &title);
    void setWindowFilePath(const QString &filePath);
    void setWindowIcon(const QIcon &icon);
    void setAlertState(bool enabled);
    bool isAlertState() const;
    void raise();
    void lower();
    bool isExposed() const;
    bool isOpaque() const;
    void propagateSizeHints();
    void setOpacity(qreal level);
    void setMask(const QRegion &region);
    bool setKeyboardGrabEnabled(bool grab);
    bool setMouseGrabEnabled(bool grab);
    QMargins frameMargins() const;
    QSurfaceFormat format() const;

    void requestActivateWindow();

    WId winId() const;
    void setParent(const QPlatformWindow *window);

    NSView *contentView() const;
    void setContentView(NSView *contentView);

    void setEmbeddedInForeignView(bool subwindow);

    void windowWillMove();
    void windowDidMove();
    void windowDidResize();
    bool windowShouldClose();
    bool windowIsPopupType(Qt::WindowType type = Qt::Widget) const;

    void setSynchedWindowStateFromWindow();

    NSInteger windowLevel(Qt::WindowFlags flags);
    NSUInteger windowStyleMask(Qt::WindowFlags flags);
    void setWindowShadow(Qt::WindowFlags flags);

    void setCurrentContext(QCocoaGLContext *context);
    QCocoaGLContext *currentContext() const;

    bool setWindowModified(bool modified) Q_DECL_OVERRIDE;

    void setFrameStrutEventsEnabled(bool enabled);
    bool frameStrutEventsEnabled() const
        { return m_frameStrutEventsEnabled; }

    void setMenubar(QCocoaMenuBar *mb);
    QCocoaMenuBar *menubar() const;

    void registerTouch(bool enable);

    qreal devicePixelRatio() const;
    void exposeWindow();
    void obscureWindow();
    QWindow *childWindowAt(QPoint windowPoint);
protected:
    // NSWindow handling. The QCocoaWindow/QNSView can either be displayed
    // in an existing NSWindow or in one created by Qt.
    void recreateWindow(const QPlatformWindow *parentWindow);
    NSWindow *createNSWindow();
    void setNSWindow(NSWindow *window);
    void clearNSWindow(NSWindow *window);

    QRect windowGeometry() const;
    QCocoaWindow *parentCocoaWindow() const;
    void syncWindowState(Qt::WindowState newState);

// private:
public: // for QNSView
    friend class QCocoaBackingStore;
    friend class QCocoaNativeInterface;

    NSView *m_contentView;
    QNSView *m_qtView;
    NSWindow *m_nsWindow;

    // TODO merge to one variable if possible
    bool m_contentViewIsEmbedded; // true if the m_contentView is actually embedded in a "foreign" NSView hiearchy
    bool m_contentViewIsToBeEmbedded; // true if the m_contentView is intended to be embedded in a "foreign" NSView hiearchy

    QNSWindowDelegate *m_nsWindowDelegate;
    Qt::WindowFlags m_windowFlags;
    Qt::WindowState m_synchedWindowState;
    Qt::WindowModality m_windowModality;
    QPointer<QWindow> m_activePopupWindow;
    QPointer<QWindow> m_underMouseWindow;

    bool m_inConstructor;
    QCocoaGLContext *m_glContext;
    QCocoaMenuBar *m_menubar;

    bool m_hasModalSession;
    bool m_frameStrutEventsEnabled;
    bool m_isExposed;
    int m_registerTouchCount;

    static const int NoAlertRequest;
    NSInteger m_alertRequest;
};

QT_END_NAMESPACE

#endif // QCOCOAWINDOW_H

