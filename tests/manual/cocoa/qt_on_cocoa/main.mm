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

    // Create the QWindow, use its NSView as the content view
    m_window = new RasterWindow();
    [window setContentView:reinterpret_cast<NSView *>(m_window->winId())];

    // Show the NSWindow
    [window makeKeyAndOrderFront:NSApp];
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



