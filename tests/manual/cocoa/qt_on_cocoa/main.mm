/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "rasterwindow.h"

#include <QtGui>
#include <QtWidgets/QtWidgets>

#include <Cocoa/Cocoa.h>

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



