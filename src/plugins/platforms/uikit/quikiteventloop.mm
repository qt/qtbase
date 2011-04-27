/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "quikiteventloop.h"
#include "quikitwindow.h"
#include "quikitwindowsurface.h"

#include <UIKit/UIKit.h>

#include <QtGui/QApplication>
#include <QtGui/QWidget>
#include <QtDebug>

@interface QUIKitAppDelegate :  NSObject <UIApplicationDelegate> {
}
@end

@interface EventLoopHelper : NSObject {
    QUIKitEventLoop *mIntegration;
}

- (id)initWithEventLoopIntegration:(QUIKitEventLoop *)integration;

- (void)processEvents;
- (void)processEventsAndSchedule;

@end

@implementation QUIKitAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    Q_UNUSED(launchOptions)
    foreach (QWidget *widget, qApp->topLevelWidgets()) {
        QRect geom = widget->geometry();
        CGRect bar = application.statusBarFrame;
        if (geom.y() <= bar.size.height) {
            geom.setY(bar.size.height);
            widget->setGeometry(geom);
        }
        QUIKitWindow *platformWindow = static_cast<QUIKitWindow *>(widget->platformWindow());
        platformWindow->ensureNativeWindow();
    }
    return YES;
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    Q_UNUSED(application)
    // TODO this isn't called for some reason
    qDebug() << "quit";
    qApp->quit();
}

@end

@implementation EventLoopHelper

- (id)initWithEventLoopIntegration:(QUIKitEventLoop *)integration
{
    if ((self = [self init])) {
        mIntegration = integration;
    }
    return self;
}

- (void)processEvents
{
    QPlatformEventLoopIntegration::processEvents();
}

- (void)processEventsAndSchedule
{
    QPlatformEventLoopIntegration::processEvents();
    qint64 nextTime = mIntegration->nextTimerEvent();
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    NSDate *nextDate = [[NSDate date] dateByAddingTimeInterval:((double)nextTime/1000)];
    [mIntegration->mTimer setFireDate:nextDate];
    [pool release];
}

@end

QT_BEGIN_NAMESPACE

QUIKitEventLoop::QUIKitEventLoop()
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    mInputHandler = new QUIKitSoftwareInputHandler;
    mHelper = [[EventLoopHelper alloc] initWithEventLoopIntegration:this];
    mTimer = [[NSTimer timerWithTimeInterval:0.030 target:mHelper selector:@selector(processEventsAndSchedule) userInfo:nil repeats:YES] retain];
    [pool release];
}

QUIKitEventLoop::~QUIKitEventLoop()
{
    [mTimer release];
    [mHelper release];
    delete mInputHandler;
}

void QUIKitEventLoop::startEventLoop()
{
    qApp->installEventFilter(mInputHandler);
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    [[NSRunLoop currentRunLoop] addTimer:[mTimer autorelease] forMode:NSDefaultRunLoopMode];
    UIApplicationMain(qApp->argc(), qApp->argv(), nil, @"QUIKitAppDelegate");
    [pool release];
}

void QUIKitEventLoop::quitEventLoop()
{

}

void QUIKitEventLoop::qtNeedsToProcessEvents()
{
    [mHelper performSelectorOnMainThread:@selector(processEvents) withObject:nil waitUntilDone:NO];
}

bool QUIKitSoftwareInputHandler::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        QWidget *widget = qobject_cast<QWidget *>(obj);
        if (widget) {
            QUIKitWindow *platformWindow = static_cast<QUIKitWindow *>(widget->platformWindow());
            [platformWindow->nativeView() becomeFirstResponder];
            return true;
        }
    } else if (event->type() == QEvent::CloseSoftwareInputPanel) {
        QWidget *widget = qobject_cast<QWidget *>(obj);
        if (widget) {
            QUIKitWindow *platformWindow = static_cast<QUIKitWindow *>(widget->platformWindow());
            [platformWindow->nativeView() resignFirstResponder];
            return true;
        }
    }
    return false;
}

QT_END_NAMESPACE
