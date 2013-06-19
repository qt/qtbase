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

#import <Cocoa/Cocoa.h>
#include <QtGui/qpa/qwindowsysteminterface.h>
#include <QtCore/private/qcore_mac_p.h>
#include "qcocoaintrospection.h"

extern int qMain(int argc, char *argv[]);

@interface QCocoaMainWrapper : NSObject

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
- (void)appleEventQuit:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent;
- (void)runUserMain;

@end

@implementation QCocoaMainWrapper

- (void)applicationWillFinishLaunching:(NSNotification *)notification
{
    if ([notification object] != NSApp) // Shouldn't happen AFAIK, but still
        return;

    /*
        From the Cocoa documentation: "A good place to install event handlers
        is in the applicationWillFinishLaunching: method of the application
        delegate. At that point, the Application Kit has installed its default
        event handlers, so if you install a handler for one of the same events,
        it will replace the Application Kit version."
    */

    /*
        If Qt is used as a plugin, we let the 3rd party application handle
        events like quit and open file events. Otherwise, if we install our own
        handlers, we easily end up breaking functionality the 3rd party
        application depends on.
     */
    NSAppleEventManager *eventManager = [NSAppleEventManager sharedAppleEventManager];
    [eventManager setEventHandler:self
                      andSelector:@selector(appleEventQuit:withReplyEvent:)
                    forEventClass:kCoreEventClass
                       andEventID:kAEQuitApplication];
    [eventManager setEventHandler:self
                      andSelector:@selector(getUrl:withReplyEvent:)
                    forEventClass:kInternetEventClass
                       andEventID:kAEGetURL];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    if ([notification object] != NSApp) // Shouldn't happen AFAIK, but still
        return;

    // We schedule the main-redirection for the next eventloop pass so that we
    // can return from this function and let NSApplicationMain finish its job.
    [NSTimer scheduledTimerWithTimeInterval:0 target:self
        selector:@selector(runUserMain) userInfo:nil repeats:NO];
}

- (void)getUrl:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(replyEvent);
    NSString *urlString = [[event paramDescriptorForKeyword:keyDirectObject] stringValue];
    QWindowSystemInterface::handleFileOpenEvent(QCFString::toQString(urlString));
}

- (void)appleEventQuit:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent
{
    Q_UNUSED(event);
    Q_UNUSED(replyEvent);
    [NSApp terminate:self];
}

- (void)runUserMain
{
    NSArray *arguments = [[NSProcessInfo processInfo] arguments];
    int argc = arguments.count;
    char **argv = new char*[argc];
    for (int i = 0; i < argc; ++i) {
        NSString *arg = [arguments objectAtIndex:i];
        argv[i] = reinterpret_cast<char *>(malloc([arg lengthOfBytesUsingEncoding:[NSString defaultCStringEncoding]]));
        strcpy(argv[i], [arg cStringUsingEncoding:[NSString defaultCStringEncoding]]);
    }

    qMain(argc, argv);
    delete[] argv;

    NSAppleEventManager *eventManager = [NSAppleEventManager sharedAppleEventManager];
    [eventManager removeEventHandlerForEventClass:kCoreEventClass andEventID:kAEQuitApplication];
    [eventManager removeEventHandlerForEventClass:kInternetEventClass andEventID:kAEGetURL];

    [NSApp terminate:self];
}

@end

static SEL qt_infoDictionary_original_SEL = @selector(qt_infoDictionary_original);

@implementation NSBundle (QT_MANGLE_NAMESPACE(QCocoaMain))

- (Class)qt_infoDictionary_replacement
{
    if (self == [NSBundle mainBundle]) {
        static NSMutableDictionary *infoDict = nil;
        if (!infoDict) {
            infoDict = [[self performSelector:qt_infoDictionary_original_SEL] mutableCopy];
            [infoDict setValue:@"NSApplication" forKey:@"NSPrincipalClass"];
        }
        return infoDict;
    }

    return [self performSelector:qt_infoDictionary_original_SEL];
}

@end

int main(int argc, char *argv[])
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    QCocoaMainWrapper *mainWrapper = [[QCocoaMainWrapper alloc] init];
    [[NSNotificationCenter defaultCenter]
        addObserver:mainWrapper selector:@selector(applicationWillFinishLaunching:)
        name:NSApplicationWillFinishLaunchingNotification object:nil];
    [[NSNotificationCenter defaultCenter]
        addObserver:mainWrapper selector:@selector(applicationDidFinishLaunching:)
        name:NSApplicationDidFinishLaunchingNotification object:nil];

    NSBundle *mainBundle = [NSBundle mainBundle];
    if (!mainBundle.principalClass) {
        // Since several of the GUI based Qt utilities (e.g., qmlscene) are command
        // line applications, meaning non-bundle applications, we need to make Cocoa
        // believe everything is fine. So we fake the main bundle's dictionary by
        // adding the "NSPrincipalClass" property. So far, this seems to be enough to
        // keep NSApplicationMain() happy and running...
        qt_cocoa_change_implementation([NSBundle class], @selector(infoDictionary),
                [NSBundle class], @selector(qt_infoDictionary_replacement), qt_infoDictionary_original_SEL);
    }

    return NSApplicationMain(argc, (const char **)argv);
}
