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
#include "qcocoawindow.h"
#include "qcocoaintegration.h"
#include "qnswindowdelegate.h"
#include "qcocoaautoreleasepool.h"
#include "qcocoaeventdispatcher.h"
#include "qcocoaglcontext.h"
#include "qcocoahelpers.h"
#include "qnsview.h"
#include <QtCore/qfileinfo.h>
#include <QtCore/private/qcore_mac_p.h>
#include <qwindow.h>
#include <private/qwindow_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatformscreen.h>

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include <QDebug>

enum {
    defaultWindowWidth = 160,
    defaultWindowHeight = 160
};

static bool isMouseEvent(NSEvent *ev)
{
    switch ([ev type]) {
    case NSLeftMouseDown:
    case NSLeftMouseUp:
    case NSRightMouseDown:
    case NSRightMouseUp:
    case NSMouseMoved:
    case NSLeftMouseDragged:
    case NSRightMouseDragged:
        return true;
    default:
        return false;
    }
}

@interface NSWindow (CocoaWindowCategory)
- (void) clearPlatformWindow;
- (NSRect) legacyConvertRectFromScreen:(NSRect) rect;
@end

@implementation NSWindow (CocoaWindowCategory)
- (void) clearPlatformWindow
{
}

- (NSRect) legacyConvertRectFromScreen:(NSRect) rect
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        return [self convertRectFromScreen: rect];
    }
#endif
    NSRect r = rect;
    r.origin = [self convertScreenToBase:rect.origin];
    return r;
}
@end

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
    if (!m_cocoaPlatformWindow || m_cocoaPlatformWindow->window()->transientParent())
        canBecomeMain = NO;

    return canBecomeMain;
}

- (void) sendEvent: (NSEvent*) theEvent
{
    [super sendEvent: theEvent];

    if (!m_cocoaPlatformWindow)
        return;

    if (m_cocoaPlatformWindow->frameStrutEventsEnabled() && isMouseEvent(theEvent)) {
        NSPoint loc = [theEvent locationInWindow];
        NSRect windowFrame = [self legacyConvertRectFromScreen:[self frame]];
        NSRect contentFrame = [[self contentView] frame];
        if (NSMouseInRect(loc, windowFrame, NO) &&
            !NSMouseInRect(loc, contentFrame, NO))
        {
            QNSView *contentView = (QNSView *) m_cocoaPlatformWindow->contentView();
            [contentView handleFrameStrutMouseEvent: theEvent];
        }
    }
}

- (void)clearPlatformWindow
{
    m_cocoaPlatformWindow = 0;
}

@end

@implementation QNSPanel

- (BOOL)canBecomeKeyWindow
{
    // Only tool or dialog windows should become key:
    if (m_cocoaPlatformWindow
        && (m_cocoaPlatformWindow->window()->type() == Qt::Tool || m_cocoaPlatformWindow->window()->type() == Qt::Dialog))
        return YES;
    return NO;
}

- (void) sendEvent: (NSEvent*) theEvent
{
    [super sendEvent: theEvent];

    if (!m_cocoaPlatformWindow)
        return;

    if (m_cocoaPlatformWindow->frameStrutEventsEnabled() && isMouseEvent(theEvent)) {
        NSPoint loc = [theEvent locationInWindow];
        NSRect windowFrame = [self legacyConvertRectFromScreen:[self frame]];
        NSRect contentFrame = [[self contentView] frame];
        if (NSMouseInRect(loc, windowFrame, NO) &&
            !NSMouseInRect(loc, contentFrame, NO))
        {
            QNSView *contentView = (QNSView *) m_cocoaPlatformWindow->contentView();
            [contentView handleFrameStrutMouseEvent: theEvent];
        }
    }
}

- (void)clearPlatformWindow
{
    m_cocoaPlatformWindow = 0;
}

@end

const int QCocoaWindow::NoAlertRequest = -1;

QCocoaWindow::QCocoaWindow(QWindow *tlw)
    : QPlatformWindow(tlw)
    , m_contentView(nil)
    , m_qtView(nil)
    , m_nsWindow(0)
    , m_contentViewIsEmbedded(false)
    , m_contentViewIsToBeEmbedded(false)
    , m_nsWindowDelegate(0)
    , m_synchedWindowState(Qt::WindowActive)
    , m_windowModality(Qt::NonModal)
    , m_inConstructor(true)
    , m_glContext(0)
    , m_menubar(0)
    , m_hasModalSession(false)
    , m_frameStrutEventsEnabled(false)
    , m_isExposed(false)
    , m_registerTouchCount(0)
    , m_alertRequest(NoAlertRequest)
{
#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::QCocoaWindow" << this;
#endif
    QCocoaAutoReleasePool pool;

    if (tlw->type() == Qt::ForeignWindow) {
        NSView *foreignView = (NSView *)WId(tlw->property("_q_foreignWinId").value<WId>());
        setContentView(foreignView);
    } else {
        m_qtView = [[QNSView alloc] initWithQWindow:tlw platformWindow:this];
        m_contentView = m_qtView;
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        // Enable high-dpi OpenGL for retina displays. Enabling has the side
        // effect that Cocoa will start calling glViewport(0, 0, width, height),
        // overriding any glViewport calls in application code. This is usually not a
        // problem, except if the appilcation wants to have a "custom" viewport.
        // (like the hellogl example)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7
            && tlw->surfaceType() == QSurface::OpenGLSurface)
            [m_contentView setWantsBestResolutionOpenGLSurface:YES];
#endif
    }
    setGeometry(tlw->geometry());
    recreateWindow(parent());
    tlw->setGeometry(geometry());
    if (tlw->isTopLevel())
        setWindowIcon(tlw->icon());
    m_inConstructor = false;
}

QCocoaWindow::~QCocoaWindow()
{
#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::~QCocoaWindow" << this;
#endif

    QCocoaAutoReleasePool pool;
    clearNSWindow(m_nsWindow);
    if (parent())
        [m_contentView removeFromSuperview];
    [m_contentView release];
    [m_nsWindow release];
    [m_nsWindowDelegate release];
}

QSurfaceFormat QCocoaWindow::format() const
{
    return window()->requestedFormat();
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
    QCocoaAutoReleasePool pool;

    if (m_contentViewIsEmbedded)
        return;

    if (m_nsWindow) {
        NSRect bounds = qt_mac_flipRect(rect, window());
        [m_nsWindow setFrame:[m_nsWindow frameRectForContentRect:bounds] display:YES animate:NO];
    } else {
        [m_contentView setFrame : NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height())];
    }
}

void QCocoaWindow::setVisible(bool visible)
{
    QCocoaAutoReleasePool pool;
    QCocoaWindow *parentCocoaWindow = 0;
    if (window()->transientParent())
        parentCocoaWindow = static_cast<QCocoaWindow *>(window()->transientParent()->handle());
#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::setVisible" << window() << visible;
#endif
    if (visible) {
        // We need to recreate if the modality has changed as the style mask will need updating
        if (m_windowModality != window()->modality())
            recreateWindow(parent());
        if (parentCocoaWindow) {
            // The parent window might have moved while this window was hidden,
            // update the window geometry if there is a parent.
            setGeometry(window()->geometry());

            // Register popup windows so that the parent window can
            // close them when needed.
            if (window()->type() == Qt::Popup) {
                // qDebug() << "transientParent and popup" << window()->type() << Qt::Popup << (window()->type() & Qt::Popup);
                parentCocoaWindow->m_activePopupWindow = window();
                // QTBUG-30266: a window should not be resizable while a transient popup is open
                // Since this isn't a native popup, the window manager doesn't close the popup when you click outside
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
                if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7
                    && !([parentCocoaWindow->m_nsWindow styleMask] & NSFullScreenWindowMask))
#endif
                [parentCocoaWindow->m_nsWindow setStyleMask:
                    (parentCocoaWindow->windowStyleMask(parentCocoaWindow->m_windowFlags) & ~NSResizableWindowMask)];
            }

        }

        // This call is here to handle initial window show correctly:
        // - top-level windows need to have backing store content ready when the
        //   window is shown, sendin the expose event here makes that more likely.
        // - QNSViews for child windows are initialy not hidden and won't get the
        //   viewDidUnhide message.
        exposeWindow();

        if (m_nsWindow) {
            QWindowSystemInterface::flushWindowSystemEvents();

            // setWindowState might have been called while the window was hidden and
            // will not change the NSWindow state in that case. Sync up here:
            syncWindowState(window()->windowState());

            if (window()->windowState() != Qt::WindowMinimized) {
                if ((window()->modality() == Qt::WindowModal
                     || window()->type() == Qt::Sheet)
                        && parentCocoaWindow) {
                    // show the window as a sheet
                    [NSApp beginSheet:m_nsWindow modalForWindow:parentCocoaWindow->m_nsWindow modalDelegate:nil didEndSelector:nil contextInfo:nil];
                } else if (window()->modality() != Qt::NonModal) {
                    // show the window as application modal
                    QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
                    Q_ASSERT(cocoaEventDispatcher != 0);
                    QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));
                    cocoaEventDispatcherPrivate->beginModalSession(window());
                    m_hasModalSession = true;
                } else if ([m_nsWindow canBecomeKeyWindow]) {
                    [m_nsWindow makeKeyAndOrderFront:nil];
                } else {
                    [m_nsWindow orderFront: nil];
                }

                // We want the events to properly reach the popup, dialog, and tool
                if ((window()->type() == Qt::Popup || window()->type() == Qt::Dialog || window()->type() == Qt::Tool)
                    && [m_nsWindow isKindOfClass:[NSPanel class]]) {
                    [(NSPanel *)m_nsWindow setWorksWhenModal:YES];
                }
            }
        } else {
            [m_contentView setHidden:NO];
        }
    } else {
        // qDebug() << "close" << this;
        if (m_nsWindow) {
            if (m_hasModalSession) {
                QCocoaEventDispatcher *cocoaEventDispatcher = qobject_cast<QCocoaEventDispatcher *>(QGuiApplication::instance()->eventDispatcher());
                Q_ASSERT(cocoaEventDispatcher != 0);
                QCocoaEventDispatcherPrivate *cocoaEventDispatcherPrivate = static_cast<QCocoaEventDispatcherPrivate *>(QObjectPrivate::get(cocoaEventDispatcher));
                cocoaEventDispatcherPrivate->endModalSession(window());
                m_hasModalSession = false;

                [m_nsWindow orderOut:m_nsWindow];
                if (m_nsWindow == [NSApp keyWindow] && !cocoaEventDispatcherPrivate->currentModalSession()) {
                    // Probably because we call runModalSession: outside [NSApp run] in QCocoaEventDispatcher
                    // (e.g., when show()-ing a modal QDialog instead of exec()-ing it), it can happen that
                    // the current NSWindow is still key after being ordered out. Then, after checking we
                    // don't have any other modal session left, it's safe to make the main window key again.
                    NSWindow *mainWindow = [NSApp mainWindow];
                    if (mainWindow && [mainWindow canBecomeKeyWindow])
                        [mainWindow makeKeyWindow];
                }
            } else {
                if ([m_nsWindow isSheet])
                    [NSApp endSheet:m_nsWindow];
                [m_nsWindow orderOut:m_nsWindow];
            }
        } else {
            [m_contentView setHidden:YES];
        }
        if (parentCocoaWindow && window()->type() == Qt::Popup
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
            && QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7
            && !([parentCocoaWindow->m_nsWindow styleMask] & NSFullScreenWindowMask)
#endif
           )
            // QTBUG-30266: a window should not be resizable while a transient popup is open
            [parentCocoaWindow->m_nsWindow setStyleMask:parentCocoaWindow->windowStyleMask(parentCocoaWindow->m_windowFlags)];
    }
}

NSInteger QCocoaWindow::windowLevel(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));

    NSInteger windowLevel = NSNormalWindowLevel;

    if (type == Qt::Tool)
        windowLevel = NSFloatingWindowLevel;
    else if ((type & Qt::Popup) == Qt::Popup)
        windowLevel = NSPopUpMenuWindowLevel;

    // StayOnTop window should appear above Tool windows.
    if (flags & Qt::WindowStaysOnTopHint)
        windowLevel = NSPopUpMenuWindowLevel;
    // Tooltips should appear above StayOnTop windows.
    if (type == Qt::ToolTip)
        windowLevel = NSScreenSaverWindowLevel;

    // A window should be in at least the same level as its parent.
    const QWindow * const transientParent = window()->transientParent();
    const QCocoaWindow * const transientParentWindow = transientParent ? static_cast<QCocoaWindow *>(transientParent->handle()) : 0;
    if (transientParentWindow)
        windowLevel = qMax([transientParentWindow->m_nsWindow level], windowLevel);

    return windowLevel;
}

NSUInteger QCocoaWindow::windowStyleMask(Qt::WindowFlags flags)
{
    Qt::WindowType type = static_cast<Qt::WindowType>(int(flags & Qt::WindowType_Mask));
    NSInteger styleMask = NSBorderlessWindowMask;

    if ((type & Qt::Popup) == Qt::Popup) {
        if (!windowIsPopupType(type) && !(flags & Qt::FramelessWindowHint))
            styleMask = (NSUtilityWindowMask | NSResizableWindowMask | NSClosableWindowMask |
                         NSMiniaturizableWindowMask | NSTitledWindowMask);
    } else {
        // Filter flags for supported properties
        flags &= Qt::WindowType_Mask | Qt::FramelessWindowHint | Qt::WindowTitleHint |
                 Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint;
        if (flags == Qt::Window) {
            styleMask = (NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSTitledWindowMask);
        } else if ((flags & Qt::Dialog) == Qt::Dialog) {
            if (window()->modality() == Qt::NonModal)
                styleMask = NSResizableWindowMask | NSClosableWindowMask | NSTitledWindowMask;
            else
                styleMask = NSResizableWindowMask | NSTitledWindowMask;
        } else if (!(flags & Qt::FramelessWindowHint)) {
            if ((flags & Qt::Dialog) || (flags & Qt::WindowMaximizeButtonHint))
                styleMask |= NSResizableWindowMask;
            if (flags & Qt::WindowTitleHint)
                styleMask |= NSTitledWindowMask;
            if (flags & Qt::WindowCloseButtonHint)
                styleMask |= NSClosableWindowMask;
            if (flags & Qt::WindowMinimizeButtonHint)
                styleMask |= NSMiniaturizableWindowMask;
        }
    }

#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug("windowStyleMask of '%s': flags %X -> styleMask %lX", qPrintable(window()->title()), (int)flags, styleMask);
#endif
    return styleMask;
}

void QCocoaWindow::setWindowShadow(Qt::WindowFlags flags)
{
    bool keepShadow = !(flags & Qt::NoDropShadowWindowHint);
    [m_nsWindow setHasShadow:(keepShadow ? YES : NO)];
}

void QCocoaWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (m_nsWindow) {
        NSUInteger styleMask = windowStyleMask(flags);
        NSInteger level = this->windowLevel(flags);
        [m_nsWindow setStyleMask:styleMask];
        [m_nsWindow setLevel:level];
        setWindowShadow(flags);
    }

    m_windowFlags = flags;
}

void QCocoaWindow::setWindowState(Qt::WindowState state)
{
    if (window()->isVisible())
        syncWindowState(state);  // Window state set for hidden windows take effect when show() is called.
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

void QCocoaWindow::setWindowFilePath(const QString &filePath)
{
    QCocoaAutoReleasePool pool;
    if (!m_nsWindow)
        return;

    QFileInfo fi(filePath);
    [m_nsWindow setRepresentedFilename: fi.exists() ? QCFString::toNSString(filePath) : @""];
}

void QCocoaWindow::setWindowIcon(const QIcon &icon)
{
    QCocoaAutoReleasePool pool;

    NSButton *iconButton = [m_nsWindow standardWindowButton:NSWindowDocumentIconButton];
    if (iconButton == nil) {
        if (icon.isNull())
            return;
        NSString *title = QCFString::toNSString(window()->title());
        [m_nsWindow setRepresentedURL:[NSURL fileURLWithPath:title]];
        iconButton = [m_nsWindow standardWindowButton:NSWindowDocumentIconButton];
    }
    if (icon.isNull()) {
        [iconButton setImage:nil];
    } else {
        QPixmap pixmap = icon.pixmap(QSize(22, 22));
        NSImage *image = static_cast<NSImage *>(qt_mac_create_nsimage(pixmap));
        [iconButton setImage:image];
        [image release];
    }
}

void QCocoaWindow::setAlertState(bool enabled)
{
    if (m_alertRequest == NoAlertRequest && enabled) {
        m_alertRequest = [NSApp requestUserAttention:NSCriticalRequest];
    } else if (m_alertRequest != NoAlertRequest && !enabled) {
        [NSApp cancelUserAttentionRequest:m_alertRequest];
        m_alertRequest = NoAlertRequest;
    }
}

bool QCocoaWindow::isAlertState() const
{
    return m_alertRequest != NoAlertRequest;
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

bool QCocoaWindow::isExposed() const
{
    return m_isExposed;
}

bool QCocoaWindow::isOpaque() const
{
    bool translucent = (window()->format().alphaBufferSize() > 0
                        || window()->opacity() < 1
                        || (m_qtView && [m_qtView hasMask]));
    return !translucent;
}

void QCocoaWindow::propagateSizeHints()
{
    QCocoaAutoReleasePool pool;
    if (!m_nsWindow)
        return;

#ifdef QT_COCOA_ENABLE_WINDOW_DEBUG
    qDebug() << "QCocoaWindow::propagateSizeHints" << this;
    qDebug() << "     min/max " << window()->minimumSize() << window()->maximumSize();
    qDebug() << "size increment" << window()->sizeIncrement();
    qDebug() << "     basesize" << window()->baseSize();
    qDebug() << "     geometry" << geometry();
#endif

    // Set the minimum content size.
    const QSize minimumSize = window()->minimumSize();
    if (!minimumSize.isValid()) // minimumSize is (-1, -1) when not set. Make that (0, 0) for Cocoa.
        [m_nsWindow setContentMinSize : NSMakeSize(0.0, 0.0)];
    [m_nsWindow setContentMinSize : NSMakeSize(minimumSize.width(), minimumSize.height())];

    // Set the maximum content size.
    const QSize maximumSize = window()->maximumSize();
    [m_nsWindow setContentMaxSize : NSMakeSize(maximumSize.width(), maximumSize.height())];

    // sizeIncrement is observed to take values of (-1, -1) and (0, 0) for windows that should be
    // resizable and that have no specific size increment set. Cocoa expects (1.0, 1.0) in this case.
    if (!window()->sizeIncrement().isEmpty())
        [m_nsWindow setResizeIncrements : qt_mac_toNSSize(window()->sizeIncrement())];
    else
        [m_nsWindow setResizeIncrements : NSMakeSize(1.0, 1.0)];

    QRect rect = geometry();
    QSize baseSize = window()->baseSize();
    if (!baseSize.isNull() && baseSize.isValid()) {
        [m_nsWindow setFrame:NSMakeRect(rect.x(), rect.y(), baseSize.width(), baseSize.height()) display:YES];
    }
}

void QCocoaWindow::setOpacity(qreal level)
{
    if (m_nsWindow) {
        [m_nsWindow setAlphaValue:level];
        [m_nsWindow setOpaque: isOpaque()];
    }
}

void QCocoaWindow::setMask(const QRegion &region)
{
    if (m_nsWindow)
        [m_nsWindow setBackgroundColor:[NSColor clearColor]];

    [m_qtView setMaskRegion:&region];
    [m_nsWindow setOpaque: isOpaque()];
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

void QCocoaWindow::setContentView(NSView *contentView)
{
    // Remove and release the previous content view
    [m_contentView removeFromSuperview];
    [m_contentView release];

    // Insert and retain the new content view
    [contentView retain];
    m_contentView = contentView;
    m_qtView = 0; // The new content view is not a QNSView.
    recreateWindow(parent()); // Adds the content view to parent NSView
}

void QCocoaWindow::setEmbeddedInForeignView(bool embedded)
{
    m_contentViewIsToBeEmbedded = embedded;
    recreateWindow(0); // destroy what was already created
}

void QCocoaWindow::windowWillMove()
{
    // Close any open popups on window move
    if (m_activePopupWindow) {
        QWindowSystemInterface::handleCloseEvent(m_activePopupWindow);
        QWindowSystemInterface::flushWindowSystemEvents();
        m_activePopupWindow = 0;
    }
}

void QCocoaWindow::windowDidMove()
{
    [m_qtView updateGeometry];
}

void QCocoaWindow::windowDidResize()
{
    if (!m_nsWindow)
        return;

    [m_qtView updateGeometry];
}

bool QCocoaWindow::windowShouldClose()
{
    bool accepted = false;
    QWindowSystemInterface::handleCloseEvent(window(), &accepted);
    QWindowSystemInterface::flushWindowSystemEvents();
    return accepted;
}

void QCocoaWindow::setSynchedWindowStateFromWindow()
{
    if (QWindow *w = window())
        m_synchedWindowState = w->windowState();
}

bool QCocoaWindow::windowIsPopupType(Qt::WindowType type) const
{
    if (type == Qt::Widget)
        type = window()->type();
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
        [m_nsWindowDelegate release];
        m_nsWindowDelegate = 0;
    }

    if (m_contentViewIsToBeEmbedded) {
        // An embedded window doesn't have its own NSWindow.
    } else if (!parentWindow) {
        // Create a new NSWindow if this is a top-level window.
        m_nsWindow = createNSWindow();
        setNSWindow(m_nsWindow);

        // QPlatformWindow subclasses must sync up with QWindow on creation:
        propagateSizeHints();
        setWindowFlags(window()->flags());
        setWindowTitle(window()->title());
        setWindowState(window()->windowState());
    } else {
        // Child windows have no NSWindow, link the NSViews instead.
        const QCocoaWindow *parentCococaWindow = static_cast<const QCocoaWindow *>(parentWindow);
        [parentCococaWindow->m_contentView addSubview : m_contentView];
        QRect rect = window()->geometry();
        NSRect frame = NSMakeRect(rect.x(), rect.y(), rect.width(), rect.height());
        [m_contentView setFrame:frame];
    }

    const qreal opacity = qt_window_private(window())->opacity;
    if (!qFuzzyCompare(opacity, qreal(1.0)))
        setOpacity(opacity);
}

void QCocoaWindow::requestActivateWindow()
{
    NSWindow *window = [m_contentView window];
    [ window makeFirstResponder : m_contentView ];
    [ window makeKeyWindow ];
}

NSWindow * QCocoaWindow::createNSWindow()
{
    QCocoaAutoReleasePool pool;

    QRect rect = initialGeometry(window(), window()->geometry(), defaultWindowWidth, defaultWindowHeight);
    NSRect frame = qt_mac_flipRect(rect, window());

    Qt::WindowType type = window()->type();
    Qt::WindowFlags flags = window()->flags();

    NSUInteger styleMask = windowStyleMask(flags);
    NSWindow *createdWindow = 0;

    // Use NSPanel for popup-type windows. (Popup, Tool, ToolTip, SplashScreen)
    // and dialogs
    if ((type & Qt::Popup) == Qt::Popup || (type & Qt::Dialog) == Qt::Dialog) {
        QNSPanel *window;
        window  = [[QNSPanel alloc] initWithContentRect:frame
                                         styleMask: styleMask
                                         backing:NSBackingStoreBuffered
                                         defer:NO]; // Deferring window creation breaks OpenGL (the GL context is set up
                                                    // before the window is shown and needs a proper window.).
        if ((type & Qt::Popup) == Qt::Popup)
            [window setHasShadow:YES];
        else {
             setWindowShadow(flags);
             [window setHidesOnDeactivate: NO];
        }

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
            // Make popup winows show on the same desktop as the parent full-screen window.
            [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];

            if ((type & Qt::Popup) == Qt::Popup)
                [window setAnimationBehavior:NSWindowAnimationBehaviorUtilityWindow];
        }
#endif
        window->m_cocoaPlatformWindow = this;
        createdWindow = window;
    } else {
        QNSWindow *window;
        window  = [[QNSWindow alloc] initWithContentRect:frame
                                         styleMask: styleMask
                                         backing:NSBackingStoreBuffered
                                         defer:NO]; // Deferring window creation breaks OpenGL (the GL context is set up
                                                    // before the window is shown and needs a proper window.).
        window->m_cocoaPlatformWindow = this;
        setWindowShadow(flags);

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
            if (flags & Qt::WindowFullscreenButtonHint)
                [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
        }
#endif

        createdWindow = window;
    }

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if ([createdWindow respondsToSelector:@selector(setRestorable:)])
        [createdWindow setRestorable: NO];
#endif

    NSInteger level = windowLevel(flags);
    [createdWindow setLevel:level];

    if (window()->format().alphaBufferSize() > 0) {
        [createdWindow setBackgroundColor:[NSColor clearColor]];
        [createdWindow setOpaque:NO];
    }

    m_windowModality = window()->modality();
    return createdWindow;
}

void QCocoaWindow::setNSWindow(NSWindow *window)
{
    m_nsWindowDelegate = [[QNSWindowDelegate alloc] initWithQCocoaWindow:this];
    [window setDelegate:m_nsWindowDelegate];

    // Prevent Cocoa from releasing the window on close. Qt
    // handles the close event asynchronously and we want to
    // make sure that m_nsWindow stays valid until the
    // QCocoaWindow is deleted by Qt.
    [window setReleasedWhenClosed : NO];


    [[NSNotificationCenter defaultCenter] addObserver:m_contentView
                                          selector:@selector(windowNotification:)
                                          name:nil // Get all notifications
                                          object:m_nsWindow];

    [m_contentView setPostsFrameChangedNotifications: NO];
    [window setContentView:m_contentView];
    [m_contentView setPostsFrameChangedNotifications: YES];
}

void QCocoaWindow::clearNSWindow(NSWindow *window)
{
    [window setContentView:nil];
    [window setDelegate:nil];
    [window clearPlatformWindow];
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

// Syncs the NSWindow minimize/maximize/fullscreen state with the current QWindow state
void QCocoaWindow::syncWindowState(Qt::WindowState newState)
{
    if (!m_nsWindow)
        return;

    // if content view width or height is 0 then the window animations will crash so
    // do nothing except set the new state
    NSRect contentRect = [contentView() frame];
    if (contentRect.size.width <= 0 || contentRect.size.height <= 0) {
        qWarning() << Q_FUNC_INFO << "invalid window content view size, check your window geometry";
        m_synchedWindowState = newState;
        return;
    }

    if ((m_synchedWindowState & Qt::WindowMaximized) != (newState & Qt::WindowMaximized)) {
        [m_nsWindow performZoom : m_nsWindow]; // toggles
    }

    if ((m_synchedWindowState & Qt::WindowMinimized) != (newState & Qt::WindowMinimized)) {
        if (newState & Qt::WindowMinimized) {
            [m_nsWindow performMiniaturize : m_nsWindow];
        } else {
            [m_nsWindow deminiaturize : m_nsWindow];
        }
    }

    if ((m_synchedWindowState & Qt::WindowFullScreen) != (newState & Qt::WindowFullScreen)) {
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
        if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
            [m_nsWindow toggleFullScreen : m_nsWindow];
        } else {
            // TODO: "normal" fullscreen
        }
#endif
    }

    // New state is now the current synched state
    m_synchedWindowState = newState;
}

bool QCocoaWindow::setWindowModified(bool modified)
{
    if (!m_nsWindow)
        return false;
    [m_nsWindow setDocumentEdited:(modified?YES:NO)];
    return true;
}

void QCocoaWindow::setMenubar(QCocoaMenuBar *mb)
{
    m_menubar = mb;
}

QCocoaMenuBar *QCocoaWindow::menubar() const
{
    return m_menubar;
}

void QCocoaWindow::registerTouch(bool enable)
{
    m_registerTouchCount += enable ? 1 : -1;
    if (enable && m_registerTouchCount == 1)
        [m_contentView setAcceptsTouchEvents:YES];
    else if (m_registerTouchCount == 0)
        [m_contentView setAcceptsTouchEvents:NO];
}

qreal QCocoaWindow::devicePixelRatio() const
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_7
    if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_7) {
        return qreal([[m_contentView window] backingScaleFactor]);
    } else
#endif
    {
        return 1.0;
    }
}

void QCocoaWindow::exposeWindow()
{
    if (!m_isExposed) {
        m_isExposed = true;
        QWindowSystemInterface::handleExposeEvent(window(), QRegion(geometry()));
    }
}

void QCocoaWindow::obscureWindow()
{
    if (m_isExposed) {
        m_isExposed = false;
        QWindowSystemInterface::handleExposeEvent(window(), QRegion());
    }
}

QWindow *QCocoaWindow::childWindowAt(QPoint windowPoint)
{
    QWindow *targetWindow = window();
    foreach (QObject *child, targetWindow->children())
        if (QWindow *childWindow = qobject_cast<QWindow *>(child))
            if (QPlatformWindow *handle = childWindow->handle())
                if (handle->isExposed() && childWindow->geometry().contains(windowPoint))
                    targetWindow = static_cast<QCocoaWindow*>(handle)->childWindowAt(windowPoint - childWindow->position());

    return targetWindow;
}

QMargins QCocoaWindow::frameMargins() const
{
    NSRect frameW = [m_nsWindow frame];
    NSRect frameC = [m_nsWindow contentRectForFrameRect:frameW];

    return QMargins(frameW.origin.x - frameC.origin.x,
        (frameW.origin.y + frameW.size.height) - (frameC.origin.y + frameC.size.height),
        (frameW.origin.x + frameW.size.width) - (frameC.origin.x + frameC.size.width),
        frameC.origin.y - frameW.origin.y);
}

void QCocoaWindow::setFrameStrutEventsEnabled(bool enabled)
{
    m_frameStrutEventsEnabled = enabled;
}
