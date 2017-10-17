/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rasterwindow.h"

#include <QtGui>
#include <QtWidgets/QtWidgets>

#include <AppKit/AppKit.h>


@interface ContentView : NSView
@end

@implementation ContentView
- (void)drawRect:(NSRect)dirtyRect {
    [[NSColor whiteColor] setFill];
    NSRectFill(dirtyRect);
}

- (void)cursorUpdate:(NSEvent *)theEvent
{
    Q_UNUSED(theEvent);
    [[NSCursor pointingHandCursor] set];
}
@end

@interface AppDelegate : NSObject <NSApplicationDelegate> {
    QGuiApplication *m_app;
    QWindow *m_window;
}
- (AppDelegate *) initWithArgc:(int)argc argv:(const char **)argv;
- (void) applicationWillFinishLaunching: (NSNotification *)notification;
- (void)applicationWillTerminate:(NSNotification *)notification;
@end


@implementation AppDelegate
- (AppDelegate *) initWithArgc:(int)argc argv:(const char **)argv
{
    m_app = new QGuiApplication(argc, const_cast<char **>(argv));
    return self;
}

- (void) applicationWillFinishLaunching: (NSNotification *)notification
{
    Q_UNUSED(notification);

    // Create the NSWindow
    NSRect frame = NSMakeRect(500, 500, 500, 500);
    NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame
                        styleMask:NSTitledWindowMask |  NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                        backing:NSBackingStoreBuffered
                        defer:NO];

    NSString *title = @"This the NSWindow window";
    [window setTitle:title];
    [window setBackgroundColor:[NSColor blueColor]];

    NSView *contentView = [[[ContentView alloc] initWithFrame:frame] autorelease];
    [contentView addTrackingArea:[[NSTrackingArea alloc] initWithRect:[contentView frame]
            options:NSTrackingActiveInActiveApp | NSTrackingInVisibleRect | NSTrackingCursorUpdate
            owner:contentView userInfo:nil]];

    // Create the QWindow, add its NSView to the content view
    m_window = new RasterWindow;
    m_window->setObjectName("RasterWindow");
    m_window->setCursor(Qt::CrossCursor);
    m_window->setGeometry(QRect(0, 0, 300, 300));

    QWindow *childWindow = new RasterWindow;
    childWindow->setObjectName("RasterWindowChild");
    childWindow->setParent(m_window);
    childWindow->setCursor(Qt::BusyCursor);
    childWindow->setGeometry(50, 50, 100, 100);

    NSTextField *textField = [[NSTextField alloc] initWithFrame:NSMakeRect(10, 10, 80, 25)];
    [(NSView*)childWindow->winId() addSubview:textField];

    [contentView addSubview:reinterpret_cast<NSView *>(m_window->winId())];

    window.contentView = contentView;

    // Show the NSWindow delayed, so that we can verify that Qt picks up the right
    // notifications to expose the window when it does become visible.
    dispatch_async(dispatch_get_main_queue(), ^{
        [window makeKeyAndOrderFront:NSApp];
    });
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
    Q_UNUSED(notification);
    delete m_window;
    delete m_app;
}

@end

int main(int argc, const char *argv[])
{
    // Create NSApplicaiton with delgate
    NSApplication *app =[NSApplication sharedApplication];
    app.delegate = [[AppDelegate alloc] initWithArgc:argc argv:argv];
    return NSApplicationMain (argc, argv);
}



