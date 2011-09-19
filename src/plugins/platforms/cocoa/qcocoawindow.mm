/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qcocoawindow.h"
#include "qnswindowdelegate.h"
#include "qcocoaautoreleasepool.h"
#include "qcocoaglcontext.h"
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
    return YES;
}

- (BOOL)canBecomeMainWindow
{
    return YES;
}

@end

QCocoaWindow::QCocoaWindow(QWindow *tlw)
    : QPlatformWindow(tlw)
    , m_windowAttributes(0)
    , m_windowClass(0)
    , m_glContext(0)
{
    QCocoaAutoReleasePool pool;

    determineWindowClass();
    m_nsWindow = createWindow();

    QNSWindowDelegate *delegate = [[QNSWindowDelegate alloc] initWithQCocoaWindow:this];
    [m_nsWindow setDelegate:delegate];
    [m_nsWindow setAcceptsMouseMovedEvents:YES];

    // Prevent Cocoa from releasing the window on close. Qt
    // handles the close event asynchronously and we want to
    // make sure that m_nsWindow stays valid until the
    // QCocoaWindow is deleted by Qt.
    [m_nsWindow setReleasedWhenClosed : NO];

    m_contentView = [[QNSView alloc] initWithQWindow:tlw];

    // ### Accept touch events by default.
    // Beware that enabling touch events has a negative impact on the overall performance.
    // We probably need a QWindowSystemInterface API to enable/disable touch events.
    [m_contentView setAcceptsTouchEvents:YES];

    setGeometry(tlw->geometry());

    [m_nsWindow setContentView:m_contentView];
}

QCocoaWindow::~QCocoaWindow()
{
    [m_nsWindow release];
}

void QCocoaWindow::setGeometry(const QRect &rect)
{
    QPlatformWindow::setGeometry(rect);

    NSRect bounds = globalGeometry(rect);
    [[m_nsWindow contentView]setFrameSize:bounds.size];
    [m_nsWindow setContentSize : bounds.size];
    [m_nsWindow setFrameOrigin : bounds.origin];

    if (m_glContext)
        m_glContext->update();
}

void QCocoaWindow::setVisible(bool visible)
{
    if (visible) {
        // The parent window might have moved while this window was hidden,
        // update the window geometry if there is a parent.
        if (window()->transientParent())
            setGeometry(window()->geometry());

        // Make sure the QWindow has a frame ready before we show the NSWindow.
        QWindowSystemInterface::handleSynchronousExposeEvent(window(), QRect(QPoint(), geometry().size()));

        [m_nsWindow makeKeyAndOrderFront:nil];
    } else {
        [m_nsWindow orderOut:nil];
    }
}

void QCocoaWindow::setWindowTitle(const QString &title)
{
    CFStringRef windowTitle = QCFString::toCFStringRef(title);
    [m_nsWindow setTitle: const_cast<NSString *>(reinterpret_cast<const NSString *>(windowTitle))];
    CFRelease(windowTitle);
}

void QCocoaWindow::raise()
{
    // ### handle spaces (see Qt 4 raise_sys in qwidget_mac.mm)
    [m_nsWindow orderFront: m_nsWindow];
}

void QCocoaWindow::lower()
{
    [m_nsWindow orderFront: m_nsWindow];
}

WId QCocoaWindow::winId() const
{
    return WId(m_nsWindow);
}

NSView *QCocoaWindow::contentView() const
{
    return [m_nsWindow contentView];
}

void QCocoaWindow::windowDidMove()
{
    if (m_glContext)
        m_glContext->update();
}

void QCocoaWindow::windowDidResize()
{
    if (m_glContext)
        m_glContext->update();

    NSRect rect = [[m_nsWindow contentView]frame];
    QRect geo(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height);
    QWindowSystemInterface::handleSynchronousGeometryChange(window(), geo);
}


void QCocoaWindow::windowWillClose()
{
    QWindowSystemInterface::handleCloseEvent(window());
}

void QCocoaWindow::setCurrentContext(QCocoaGLContext *context)
{
    m_glContext = context;
}

QCocoaGLContext *QCocoaWindow::currentContext() const
{
    return m_glContext;
}

/*
    Determine the window class based on the window type and
    window flags, and widget attr Sets m_windowAttributes
    and m_windowClass.
*/
void QCocoaWindow::determineWindowClass()
{
    Qt::WindowType type = window()->windowType();
    Qt::WindowFlags flags = window()->windowFlags();

    const bool popup = (type == Qt::Popup);

    if (type == Qt::ToolTip || type == Qt::SplashScreen || popup)
        flags |= Qt::FramelessWindowHint;

    m_windowClass = kSheetWindowClass;

    if (popup || type == Qt::SplashScreen)
        m_windowClass = kModalWindowClass;
    else if (type == Qt::ToolTip)
        m_windowClass = kHelpWindowClass;
    else if (type == Qt::Tool)
        m_windowClass = kFloatingWindowClass;
    else
        m_windowClass = kDocumentWindowClass;

    m_windowAttributes = (kWindowCompositingAttribute | kWindowStandardHandlerAttribute);

//    if(qt_mac_is_macsheet(window())) {
//        m_windowClass = kSheetWindowClass;
//    } else

    {
        // Shift things around a bit to get the correct window class based on the presence
        // (or lack) of the border.

        bool customize = flags & Qt::CustomizeWindowHint;
        bool framelessWindow = (flags & Qt::FramelessWindowHint || (customize && !(flags & Qt::WindowTitleHint)));
        if (framelessWindow) {
            if (m_windowClass == kDocumentWindowClass) {
                m_windowAttributes |= kWindowNoTitleBarAttribute;
            } else if (m_windowClass == kFloatingWindowClass) {
                m_windowAttributes |= kWindowNoTitleBarAttribute;
            } else if (m_windowClass  == kMovableModalWindowClass) {
                m_windowClass = kModalWindowClass;
            }
        } else {
            m_windowAttributes |= NSTitledWindowMask;
            if (m_windowClass != kModalWindowClass)
                m_windowAttributes |= NSResizableWindowMask;
        }

        // Only add extra decorations (well, buttons) for widgets that can have them
        // and have an actual border we can put them on.

        if(m_windowClass != kModalWindowClass && m_windowClass != kMovableModalWindowClass
                && m_windowClass != kSheetWindowClass && m_windowClass != kPlainWindowClass
                && !framelessWindow && m_windowClass != kDrawerWindowClass
                && m_windowClass != kHelpWindowClass) {
            if (flags & Qt::WindowMinimizeButtonHint)
                m_windowAttributes |= NSMiniaturizableWindowMask;
            if (flags & Qt::WindowSystemMenuHint || flags & Qt::WindowCloseButtonHint)
                m_windowAttributes |= NSClosableWindowMask;
        } else {
            // Clear these hints so that we aren't call them on invalid windows
            flags &= ~(Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint
                       | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);
        }

    }

    if((popup || type == Qt::Tool) && !window()->isModal())
        m_windowAttributes |= kWindowHideOnSuspendAttribute;
    m_windowAttributes |= kWindowLiveResizeAttribute;
}

/*

*/
QNSWindow * QCocoaWindow::createWindow()
{
    // Determine if we need to add in our "custom window" attribute. Cocoa is rather clever
    // in deciding if we need the maximize button or not (i.e., it's resizeable, so you
    // must need a maximize button). So, the only buttons we have control over are the
    // close and minimize buttons. If someone wants to customize and NOT have the maximize
    // button, then we have to do our hack. We only do it for these cases because otherwise
    // the window looks different when activated. This "QtMacCustomizeWindow" attribute is
    // intruding on a public space and WILL BREAK in the future.
    // One can hope that there is a more public API available by that time.
/*
    Qt::WindowFlags flags = widget ? widget->windowFlags() : Qt::WindowFlags(0);
    if ((flags & Qt::CustomizeWindowHint)) {
        if ((flags & (Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint
                      | Qt::WindowMinimizeButtonHint | Qt::WindowTitleHint))
            && !(flags & Qt::WindowMaximizeButtonHint))
            wattr |= QtMacCustomizeWindow;
    }
*/
    NSRect frame = globalGeometry(window()->geometry());
    QCocoaAutoReleasePool pool;
    QNSWindow *window;

    switch (m_windowClass) {
    case kMovableModalWindowClass:
    case kModalWindowClass:
    case kSheetWindowClass:
    case kFloatingWindowClass:
    case kOverlayWindowClass:
    case kHelpWindowClass: {
        NSPanel *panel;

        BOOL needFloating = NO;
        BOOL worksWhenModal = (this->window()->windowType() == Qt::Popup);

        // Add in the extra flags if necessary.
        switch (m_windowClass) {
        case kSheetWindowClass:
            m_windowAttributes |= NSDocModalWindowMask;
            break;
        case kFloatingWindowClass:
        case kHelpWindowClass:
            needFloating = YES;
            m_windowAttributes |= NSUtilityWindowMask;
            break;
        default:
            break;
        }

        panel = [[NSPanel alloc] initWithContentRect:frame
                                   styleMask:m_windowAttributes
                                   backing:NSBackingStoreBuffered
                                   defer:NO]; // see window case below
//  ### crashes
//        [panel setFloatingPanel:needFloating];
//        [panel setWorksWhenModal:worksWhenModal];
        window = static_cast<NSWindow *>(panel);
        break;
    }
    default:
        window  = [[QNSWindow alloc] initWithContentRect:frame
                                            styleMask:(NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSTitledWindowMask)
                                            backing:NSBackingStoreBuffered
                                            defer:NO]; // Deferring window creation breaks OpenGL (the GL context is set up
                                                       // before the window is shown and needs a proper window.).
        break;
    }

    //qt_syncCocoaTitleBarButtons(window, widget);
    return window;
}

// Calculate the global screen geometry for the given local geometry, which
// might be in the parent window coordinate system.
NSRect QCocoaWindow::globalGeometry(const QRect localGeometry) const
{
    QRect finalGeometry = localGeometry;

    if (QCocoaWindow *parent = parentCocoaWindow()) {
        QRect parentGeometry = parent->windowGeometry();
        finalGeometry.adjust(parentGeometry.x(), parentGeometry.y(), parentGeometry.x(), parentGeometry.y());

        // Qt child window geometry assumes that the origin is at the
        // top-left of the content area of the parent window. The title
        // bar is not a part of this contet area, but is still included
        // in the NSWindow height. Move the child window down to acccount
        // for this if the parent window has a title bar.
        const int titlebarHeight = 22;
        if (!(window()->windowFlags() & Qt::FramelessWindowHint))
            finalGeometry.adjust(0, titlebarHeight, 0, titlebarHeight);
    }

    // The final "y invert" to get OS X global geometry:
    QPlatformScreen *onScreen = QPlatformScreen::platformScreenForWindow(window());
    int flippedY = onScreen->geometry().height() - finalGeometry.y() - finalGeometry.height();
    return NSMakeRect(finalGeometry.x(), flippedY, finalGeometry.width(), finalGeometry.height());
}

// Returns the current global screen geometry for the nswindow accociated with this window.
QRect QCocoaWindow::windowGeometry() const
{
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

