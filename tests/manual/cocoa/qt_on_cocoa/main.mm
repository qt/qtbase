/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtGui>
#include <QtDeclarative>

#include <QtWidgets/QtWidgets>
#include <private/qwidgetwindow_qpa_p.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

#include <QtGui/QPixmap>

#include "window.h"

#include <Cocoa/Cocoa.h>


@interface FilledView : NSView
{

}
@end


@implementation FilledView

- (void)drawRect:(NSRect)dirtyRect {
    // set any NSColor for filling, say white:
    [[NSColor redColor] setFill];
    NSRectFill(dirtyRect);
}

@end

@interface QtMacToolbarDelegate : NSObject <NSToolbarDelegate>
{
@public
    NSToolbar *toolbar;
}

- (id)init;
- (NSToolbarItem *) toolbar: (NSToolbar *)toolbar itemForItemIdentifier: (NSString *) itemIdent willBeInsertedIntoToolbar:(BOOL) willBeInserted;
- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar*)tb;
- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar*)toolbar;
- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar;
@end

@implementation QtMacToolbarDelegate

- (id)init
{
    self = [super init];
    if (self) {
    }
    return self;
}

- (void)dealloc
{
    [super dealloc];
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar*)tb
{
    Q_UNUSED(tb);
    NSMutableArray *array = [[[NSMutableArray alloc] init] autorelease];
//    [array addObject : NSToolbarPrintItemIdentifier];
//    [array addObject : NSToolbarShowColorsItemIdentifier];
    [array addObject : @"filledView"];
    return array;
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar*)tb
{
    Q_UNUSED(tb);
    NSMutableArray *array = [[[NSMutableArray alloc] init] autorelease];
//    [array addObject : NSToolbarPrintItemIdentifier];
//    [array addObject : NSToolbarShowColorsItemIdentifier];
    [array addObject : @"filledView"];
    return array;
}

- (NSArray *)toolbarSelectableItemIdentifiers: (NSToolbar *)tb
{
    Q_UNUSED(tb);
    NSMutableArray *array = [[[NSMutableArray alloc] init] autorelease];
    return array;
}

- (IBAction)itemClicked:(id)sender
{

}

- (NSToolbarItem *) toolbar: (NSToolbar *)tb itemForItemIdentifier: (NSString *) itemIdentifier willBeInsertedIntoToolbar:(BOOL) willBeInserted
{
    Q_UNUSED(tb);
    Q_UNUSED(willBeInserted);
    //const QString identifier = toQString(itemIdentifier);
    //NSToolbarItem *toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
    //return toolbarItem;

    //NSToolbarItem *toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
    NSToolbarItem *toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier: itemIdentifier] autorelease];
    FilledView *theView = [[FilledView alloc] init];
    [toolbarItem setView : theView];
    [toolbarItem setMinSize : NSMakeSize(400, 40)];
    [toolbarItem setMaxSize : NSMakeSize(4000, 40)];
    return toolbarItem;
}
@end

@interface WindowAndViewAndQtCreator : NSObject {}
- (void)createWindowAndViewAndQt;
@end

@implementation WindowAndViewAndQtCreator
- (void)createWindowAndViewAndQt {

    // Create the window
    NSRect frame = NSMakeRect(500, 500, 500, 500);
    NSWindow* window  = [[NSWindow alloc] initWithContentRect:frame
                        styleMask:NSTitledWindowMask |  NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask
                        backing:NSBackingStoreBuffered
                        defer:NO];

    NSString *title = @"This the NSWindow window";
    [window setTitle:title];

    [window setBackgroundColor:[NSColor blueColor]];

    // Create a tool bar, set Qt delegate
    NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier : @"foobartoolbar"];
    QtMacToolbarDelegate *delegate = [[QtMacToolbarDelegate alloc] init];
    [toolbar setDelegate : delegate];
    [window setToolbar : toolbar];

    // Create the QWindow, don't show it.
    Window *qtWindow = new Window();
    qtWindow->create();

    //QSGView *qtWindow = new QSGView();
    //qtWindow->setSource(QUrl::fromLocalFile("/Users/msorvig/code/qt5/qtdeclarative/examples/declarative/samegame/samegame.qml"));
 //   qtWindow->setWindowFlags(Qt::WindowType(13)); // 13: NativeEmbeddedWindow

    // Get the nsview from the QWindow, set it as the content view
    // on the NSWindow created above.
    QPlatformNativeInterface *platformNativeInterface = QGuiApplication::platformNativeInterface();
    NSView *qtView = (NSView *)platformNativeInterface->nativeResourceForWindow("nsview", qtWindow);
    [window setContentView:qtView];
    [window makeKeyAndOrderFront:NSApp];
}
@end

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // fake NSApplicationMain() implementation follows:
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];

    // schedule call to create the UI.
    WindowAndViewAndQtCreator *windowAndViewAndQtCreator= [WindowAndViewAndQtCreator alloc];
    [NSTimer scheduledTimerWithTimeInterval:0 target:windowAndViewAndQtCreator selector:@selector(createWindowAndViewAndQt) userInfo:nil repeats:NO];

    [(NSApplication *)NSApp run];
    [NSApp release];
    [pool release];
    exit(0);
    return 0;
}



