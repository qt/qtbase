// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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

@interface AppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation AppDelegate {
    QGuiApplication *m_app;
    QWindow *m_window;
}

- (instancetype)initWithArgc:(int)argc argv:(const char **)argv
{
    if ((self = [self init])) {
        m_app = new QGuiApplication(argc, const_cast<char **>(argv));
    }
    return self;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    Q_UNUSED(notification);

    // Create the NSWindow
    NSRect frame = NSMakeRect(500, 500, 500, 500);
    NSWindow *window  = [[NSWindow alloc] initWithContentRect:frame
                        styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
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
    [reinterpret_cast<NSView *>(childWindow->winId()) addSubview:textField];

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
    NSApplication *app = [NSApplication sharedApplication];
    app.delegate = [[AppDelegate alloc] initWithArgc:argc argv:argv];
    return NSApplicationMain(argc, argv);
}
