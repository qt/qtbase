/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qcocoawindow.h"
#include "qnswindowdelegate.h"
#include "qcocoaautoreleasepool.h"
#include "qcocoaglcontext.h"
#include "qcocoahelpers.h"
#include "qnsview.h"
#include <QtCore/private/qcore_mac_p.h>
#include <qwindow.h>
#include <QWindowSystemInterface>
#include <QPlatformScreen>

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include <QDebug>

@implementation QNSWindow

- (BOOL)canBecomeKeyWindow
{
    // The default implementation returns NO for title-bar less windows,
    // override and return yes here to make sure popup windows such as
    // the combobox popup can become the key window.
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    BOOL canBecomeMain = YES; // By default, windows can become the main window

    // Windows with a transient parent (such as combobox popup windows)
    // cannot become the main window:
    if (m_cocoaPlatformWindow->window()->transientParent())
        canBecomeMain = NO;

    return canBecomeMain;
}


@end

@implementation QNSPanel

- (BOOL)canBecomeKeyWindow
{
    // Most panels can be come the key window. Exceptions are:
    if (m_cocoaPlatformWindow->window()->windowType() == Qt::ToolTip)
        return NO;
    if (m_cocoaPlatformWindow->window()->windowType() == Qt::SplashScreen)
        return NO;
    return YES;
}

@end

QCocoaWindow::QCocoaWindow(QWindow *tlw)
    : QPlatformWindow(tlw)
    , m_nsWindow(0)
    , m_inConstructor(true)
    , m_glContext(0)
{
    QCocoaAutoReleasePool pool;

    m_contentView = [[QNSView alloc] initWithQWindow:tlw platformWindow:this];
    setGeometry(tlw->geometry());

    recreateWindow(parent());

    m_inConstructor = false;
}

QCocoaWindow::~QCocoaWindow()
{
    clearNSWindow(m_nsWindow);
    [m_contentView release];
    [m_nsWindow release];
}

void QCocoaWindow::setGeometry(const QRect &rect)
{
    if (geometry() == rect)
        return;
#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::setGeometry" << this << rect;
#endif
    QPlatformWindow::setGeometry(rect);
    setCocoaGeometry(rect);
}

void QCocoaWindow::setCocoaGeometry(const QRect &rect)
{
    if (m_nsWindow) {
        NSRect bounds = qt_mac_flipRect(rect, window());
        [m_nsWindow setContentSize : bounds.size];
        [m_nsWindow setFrameOrigin : bounds.origin];
    } else {
        [m_contentView setFrame : NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height())];
    }
}

void QCocoaWindow::setVisible(bool visible)
{
    QCocoaAutoReleasePool pool;
#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::setVisible" << window() << visible;
#endif
    if (visible) {
        if (window()->transientParent()) {
            // The parent window might have moved while this window was hidden,
            // update the window geometry if there is a parent.
            setGeometry(window()->geometry());

            // Register popup windows so that the parent window can
            // close them when needed.
            if (window()->windowType() == Qt::Popup) {
                // qDebug() << "transientParent and popup" << window()->windowType() << Qt::Popup << (window()->windowType() & Qt::Popup);

                QCocoaWindow *parentCocoaWindow = static_cast<QCocoaWindow *>(window()->transientParent()->handle());
                parentCocoaWindow->m_activePopupWindow = window();
            }

        }

        // Make sure the QWindow has a frame ready before we show the NSWindow.
        QWindowSystemInterface::handleSynchronousExposeEvent(window(), QRect(QPoint(), geometry().size()));

        if (m_nsWindow) {
            if ([m_nsWindow canBecomeKeyWindow])
                [m_nsWindow makeKeyAndOrderFront:nil];
            else
                [m_nsWindow orderFront: nil];
        }
    } else {
        // qDebug() << "close" << this;
        if (m_nsWindow)
            [m_nsWindow orderOut:m_nsWindow];
    }
}

Qt::WindowFlags QCocoaWindow::setWindowFlags(Qt::WindowFlags flags)
{
    m_windowFlags = flags;
    return m_windowFlags;
}

void QCocoaWindow::setWindowTitle(const QString &title)
{
    QCocoaAutoReleasePool pool;
    if (!m_nsWindow)
        return;

    CFStringRef windowTitle = QCFString::toCFStringRef(title);
    [m_nsWindow setTitle: const_cast<NSString *>(reinterpret_cast<const NSString *>(windowTitle))];
    CFRelease(windowTitle);
}

void QCocoaWindow::raise()
{
    //qDebug() << "raise" << this;
    // ### handle spaces (see Qt 4 raise_sys in qwidget_mac.mm)
    if (!m_nsWindow)
        return;
    if ([m_nsWindow isVisible])
        [m_nsWindow orderFront: m_nsWindow];
}

void QCocoaWindow::lower()
{
    if (!m_nsWindow)
        return;
    if ([m_nsWindow isVisible])
        [m_nsWindow orderBack: m_nsWindow];
}

void QCocoaWindow::propagateSizeHints()
{
    QCocoaAutoReleasePool pool;
    if (!m_nsWindow)
        return;

    [m_nsWindow setMinSize : qt_mac_toNSSize(window()->minimumSize())];
    [m_nsWindow setMaxSize : qt_mac_toNSSize(window()->maximumSize())];

#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::propagateSizeHints" << this;
    qDebug() << "     min/max " << window()->minimumSize() << window()->maximumSize();
    qDebug() << "     basesize" << window()->baseSize();
    qDebug() << "     geometry" << geometry();
#endif

    if (!window()->sizeIncrement().isNull())
        [m_nsWindow setResizeIncrements : qt_mac_toNSSize(window()->sizeIncrement())];

    QRect rect = geometry();
    QSize baseSize = window()->baseSize();
    if (!baseSize.isNull() && baseSize.isValid()) {
        [m_nsWindow setFrame:NSMakeRect(rect.x(), rect.y(), baseSize.width(), baseSize.height()) display:YES];
    }
}

void QCocoaWindow::setOpacity(qreal level)
{
    if (m_nsWindow)
        [m_nsWindow setAlphaValue:level];
}

bool QCocoaWindow::setKeyboardGrabEnabled(bool grab)
{
    if (!m_nsWindow)
        return false;

    if (grab && ![m_nsWindow isKeyWindow])
        [m_nsWindow makeKeyWindow];
    else if (!grab && [m_nsWindow isKeyWindow])
        [m_nsWindow resignKeyWindow];
    return true;
}

bool QCocoaWindow::setMouseGrabEnabled(bool grab)
{
    if (!m_nsWindow)
        return false;

    if (grab && ![m_nsWindow isKeyWindow])
        [m_nsWindow makeKeyWindow];
    else if (!grab && [m_nsWindow isKeyWindow])
        [m_nsWindow resignKeyWindow];
    return true;
}

WId QCocoaWindow::winId() const
{
    return WId(m_contentView);
}

void QCocoaWindow::setParent(const QPlatformWindow *parentWindow)
{
    // recreate the window for compatibility
    recreateWindow(parentWindow);
    setCocoaGeometry(geometry());
}

NSView *QCocoaWindow::contentView() const
{
    return m_contentView;
}

void QCocoaWindow::windowWillMove()
{
    // Close any open popups on window move
    if (m_activePopupWindow) {
        QWindowSystemInterface::handleSynchronousCloseEvent(m_activePopupWindow);
        m_activePopupWindow = 0;
    }
}

void QCocoaWindow::windowDidMove()
{
    [m_contentView updateGeometry];
}

void QCocoaWindow::windowDidResize()
{
    if (!m_nsWindow)
        return;

    NSRect rect = [[m_nsWindow contentView]frame];
    // Call setFrameSize which will trigger a frameDidChangeNotification on QNSView.
    [[m_nsWindow contentView] setFrameSize:rect.size];
}

void QCocoaWindow::windowWillClose()
{
    QWindowSystemInterface::handleSynchronousCloseEvent(window());
}

bool QCocoaWindow::windowIsPopupType() const
{
    Qt::WindowType type = window()->windowType();
    if (type == Qt::Tool)
        return false; // Qt::Tool has the Popup bit set but isn't, at least on Mac.

    return ((type & Qt::Popup) == Qt::Popup);
}

void QCocoaWindow::setCurrentContext(QCocoaGLContext *context)
{
    m_glContext = context;
}

QCocoaGLContext *QCocoaWindow::currentContext() const
{
    return m_glContext;
}

void QCocoaWindow::recreateWindow(const QPlatformWindow *parentWindow)
{
    // Remove current window (if any)
    if (m_nsWindow) {
        clearNSWindow(m_nsWindow);
        [m_nsWindow close];
        [m_nsWindow release];
        m_nsWindow = 0;
    }

    if (!parentWindow) {
        // Create a new NSWindow if this is a top-level window.
        m_nsWindow = createNSWindow();
        setNSWindow(m_nsWindow);
    } else {
        // Child windows have no NSWindow, link the NSViews instead.
        const QCocoaWindow *parentCococaWindow = static_cast<const QCocoaWindow *>(parentWindow);
        [parentCococaWindow->m_contentView addSubview : m_contentView];
    }
}

NSWindow * QCocoaWindow::createNSWindow()
{
    QCocoaAutoReleasePool pool;

    NSRect frame = qt_mac_flipRect(window()->geometry(), window());

    Qt::WindowType type = window()->windowType();
    Qt::WindowFlags flags = window()->windowFlags();

    NSUInteger styleMask;
    NSWindow *createdWindow = 0;
    NSInteger windowLevel = -1;

    if (type == Qt::Tool) {
        windowLevel = NSFloatingWindowLevel;
    } else if ((type & Qt::Popup) == Qt::Popup) {
        // styleMask = NSBorderlessWindowMask;
        windowLevel = NSPopUpMenuWindowLevel;

        // Popup should be in at least the same level as its parent.
        const QWindow * const transientParent = window()->transientParent();
        const QCocoaWindow * const transientParentWindow = transientParent ? static_cast<QCocoaWindow *>(transientParent->handle()) : 0;
        if (transientParentWindow)
            windowLevel = qMax([transientParentWindow->m_nsWindow level], windowLevel);
    }

    // StayOnTop window should appear above Tool windows.
    if (flags & Qt::WindowStaysOnTopHint)
        windowLevel = NSPopUpMenuWindowLevel;
    // Tooltips should appear above StayOnTop windows.
    if (type == Qt::ToolTip)
        windowLevel = NSScreenSaverWindowLevel;
    // All other types are Normal level.
    if (windowLevel == -1)
        windowLevel = NSNormalWindowLevel;

    // Use NSPanel for popup-type windows. (Popup, Tool, ToolTip, SplashScreen)
    if ((type & Qt::Popup) == Qt::Popup) {
        if (windowIsPopupType()) {
            styleMask = NSBorderlessWindowMask;
        } else {
            styleMask = (NSUtilityWindowMask | NSResizableWindowMask | NSClosableWindowMask |
                         NSMiniaturizableWindowMask | NSTitledWindowMask);
        }

        QNSPanel *window;
        window  = [[QNSPanel alloc] initWithContentRect:frame
                                         styleMask: styleMask
                                         backing:NSBackingStoreBuffered
                                         defer:NO]; // Deferring window creation breaks OpenGL (the GL context is set up
                                                    // before the window is shown and needs a proper window.).
        [window setHasShadow:YES];
        window->m_cocoaPlatformWindow = this;
        createdWindow = window;
    } else {
        styleMask = (NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSTitledWindowMask);
        QNSWindow *window;
        window  = [[QNSWindow alloc] initWithContentRect:frame
                                         styleMask: styleMask
                                         backing:NSBackingStoreBuffered
                                         defer:NO]; // Deferring window creation breaks OpenGL (the GL context is set up
                                                    // before the window is shown and needs a proper window.).
        window->m_cocoaPlatformWindow = this;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        // All windows with the WindowMaximizeButtonHint set also get a full-screen button.
        if (flags & Qt::WindowMaximizeButtonHint)
            [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    }
#endif

        createdWindow = window;
    }

    [createdWindow setLevel:windowLevel];

    return createdWindow;
}

void QCocoaWindow::setNSWindow(NSWindow *window)
{
    QNSWindowDelegate *delegate = [[QNSWindowDelegate alloc] initWithQCocoaWindow:this];
    [window setDelegate:delegate];
    [window setAcceptsMouseMovedEvents:YES];

    // Prevent Cocoa from releasing the window on close. Qt
    // handles the close event asynchronously and we want to
    // make sure that m_nsWindow stays valid until the
    // QCocoaWindow is deleted by Qt.
    [window setReleasedWhenClosed : NO];

    [[NSNotificationCenter defaultCenter] addObserver:m_contentView
                                          selector:@selector(windowDidBecomeKey)
                                          name:NSWindowDidBecomeKeyNotification
                                          object:m_nsWindow];

    [[NSNotificationCenter defaultCenter] addObserver:m_contentView
                                          selector:@selector(windowDidResignKey)
                                          name:NSWindowDidResignKeyNotification
                                          object:m_nsWindow];

    [[NSNotificationCenter defaultCenter] addObserver:m_contentView
                                          selector:@selector(windowDidBecomeMain)
                                          name:NSWindowDidBecomeMainNotification
                                          object:m_nsWindow];

    [[NSNotificationCenter defaultCenter] addObserver:m_contentView
                                          selector:@selector(windowDidResignMain)
                                          name:NSWindowDidResignMainNotification
                                          object:m_nsWindow];


    // ### Accept touch events by default.
    // Beware that enabling touch events has a negative impact on the overall performance.
    // We probably need a QWindowSystemInterface API to enable/disable touch events.
    [m_contentView setAcceptsTouchEvents:YES];

    [window setContentView:m_contentView];
}

void QCocoaWindow::clearNSWindow(NSWindow *window)
{
    [window setDelegate:nil];
    [[NSNotificationCenter defaultCenter] removeObserver:m_contentView];
}

// Returns the current global screen geometry for the nswindow associated with this window.
QRect QCocoaWindow::windowGeometry() const
{
    if (!m_nsWindow)
        return geometry();

    NSRect rect = [m_nsWindow frame];
    QPlatformScreen *onScreen = QPlatformScreen::platformScreenForWindow(window());
    int flippedY = onScreen->geometry().height() - rect.origin.y - rect.size.height;  // account for nswindow inverted y.
    QRect qRect = QRect(rect.origin.x, flippedY, rect.size.width, rect.size.height);
    return qRect;
}

// Returns a pointer to the parent QCocoaWindow for this window, or 0 if there is none.
QCocoaWindow *QCocoaWindow::parentCocoaWindow() const
{
    if (window() && window()->transientParent()) {
        return static_cast<QCocoaWindow*>(window()->transientParent()->handle());
    }
    return 0;
}

