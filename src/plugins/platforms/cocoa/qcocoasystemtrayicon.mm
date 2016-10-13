/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2012 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Christoph Schleifenbaum <christoph.schleifenbaum@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** Copyright (c) 2007-2008, Apple, Inc.
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
**   * Redistributions of source code must retain the above copyright notice,
**     this list of conditions and the following disclaimer.
**
**   * Redistributions in binary form must reproduce the above copyright notice,
**     this list of conditions and the following disclaimer in the documentation
**     and/or other materials provided with the distribution.
**
**   * Neither the name of Apple, Inc. nor the names of its contributors
**     may be used to endorse or promote products derived from this software
**     without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/

#define QT_MAC_SYSTEMTRAY_USE_GROWL

#include "qcocoasystemtrayicon.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <qtemporaryfile.h>
#include <qimagewriter.h>
#include <qdebug.h>

#include "qcocoamenu.h"

#include "qt_mac_p.h"
#include "qcocoahelpers.h"
#include <QtGui/private/qcoregraphics_p.h>

#import <AppKit/AppKit.h>

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QNSMenu);
@class QT_MANGLE_NAMESPACE(QNSImageView);

@interface QT_MANGLE_NAMESPACE(QNSStatusItem) : NSObject <NSUserNotificationCenterDelegate>
{
@public
    QCocoaSystemTrayIcon *systray;
    NSStatusItem *item;
    QCocoaMenu *menu;
    QIcon icon;
    QT_MANGLE_NAMESPACE(QNSImageView) *imageCell;
}
-(id)initWithSysTray:(QCocoaSystemTrayIcon *)systray;
-(void)dealloc;
-(NSStatusItem*)item;
-(QRectF)geometry;
- (void)triggerSelector:(id)sender button:(Qt::MouseButton)mouseButton;
- (void)doubleClickSelector:(id)sender;
- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification;
- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification;
@end

@interface QT_MANGLE_NAMESPACE(QNSImageView) : NSImageView {
    BOOL down;
    QT_MANGLE_NAMESPACE(QNSStatusItem) *parent;
}
-(id)initWithParent:(QT_MANGLE_NAMESPACE(QNSStatusItem)*)myParent;
-(void)menuTrackingDone:(NSNotification*)notification;
-(void)mousePressed:(NSEvent *)mouseEvent button:(Qt::MouseButton)mouseButton;
@end

@interface QT_MANGLE_NAMESPACE(QNSMenu) : NSMenu <NSMenuDelegate> {
    QPlatformMenu *qmenu;
}
-(QPlatformMenu*)menu;
-(id)initWithQMenu:(QPlatformMenu*)qmenu;
@end

QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSStatusItem);
QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSImageView);
QT_NAMESPACE_ALIAS_OBJC_CLASS(QNSMenu);

QT_BEGIN_NAMESPACE
class QSystemTrayIconSys
{
public:
    QSystemTrayIconSys(QCocoaSystemTrayIcon *sys) {
        item = [[QNSStatusItem alloc] initWithSysTray:sys];
        [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:item];
    }
    ~QSystemTrayIconSys() {
        [[[item item] view] setHidden: YES];
        [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:nil];
        [item release];
    }
    QNSStatusItem *item;
};

void QCocoaSystemTrayIcon::init()
{
    if (!m_sys)
        m_sys = new QSystemTrayIconSys(this);
}

QRect QCocoaSystemTrayIcon::geometry() const
{
    if (!m_sys)
        return QRect();

    const QRectF geom = [m_sys->item geometry];
    if (!geom.isNull())
        return geom.toRect();
    else
        return QRect();
}

void QCocoaSystemTrayIcon::cleanup()
{
    delete m_sys;
    m_sys = 0;
}

static bool heightCompareFunction (QSize a, QSize b) { return (a.height() < b.height()); }
static QList<QSize> sortByHeight(const QList<QSize> &sizes)
{
    QList<QSize> sorted = sizes;
    std::sort(sorted.begin(), sorted.end(), heightCompareFunction);
    return sorted;
}

void QCocoaSystemTrayIcon::updateIcon(const QIcon &icon)
{
    if (!m_sys)
        return;

    m_sys->item->icon = icon;

    // The reccomended maximum title bar icon height is 18 points
    // (device independent pixels). The menu height on past and
    // current OS X versions is 22 points. Provide some future-proofing
    // by deriving the icon height from the menu height.
    const int padding = 4;
    const int menuHeight = [[NSStatusBar systemStatusBar] thickness];
    const int maxImageHeight = menuHeight - padding;

    // Select pixmap based on the device pixel height. Ideally we would use
    // the devicePixelRatio of the target screen, but that value is not
    // known until draw time. Use qApp->devicePixelRatio, which returns the
    // devicePixelRatio for the "best" screen on the system.
    qreal devicePixelRatio = qApp->devicePixelRatio();
    const int maxPixmapHeight = maxImageHeight * devicePixelRatio;
    QSize selectedSize;
    Q_FOREACH (const QSize& size, sortByHeight(icon.availableSizes())) {
        // Select a pixmap based on the height. We want the largest pixmap
        // with a height smaller or equal to maxPixmapHeight. The pixmap
        // may rectangular; assume it has a reasonable size. If there is
        // not suitable pixmap use the smallest one the icon can provide.
        if (size.height() <= maxPixmapHeight) {
            selectedSize = size;
        } else {
            if (!selectedSize.isValid())
                selectedSize = size;
            break;
        }
    }

    // Handle SVG icons, which do not return anything for availableSizes().
    if (!selectedSize.isValid())
        selectedSize = icon.actualSize(QSize(maxPixmapHeight, maxPixmapHeight));

    QPixmap pixmap = icon.pixmap(selectedSize);

    // Draw a low-resolution icon if there is not enough pixels for a retina
    // icon. This prevents showing a small icon on retina displays.
    if (devicePixelRatio > 1.0 && selectedSize.height() < maxPixmapHeight / 2)
        devicePixelRatio = 1.0;

    // Scale large pixmaps to fit the available menu bar area.
    if (pixmap.height() > maxPixmapHeight)
        pixmap = pixmap.scaledToHeight(maxPixmapHeight, Qt::SmoothTransformation);

    // The icon will be stretched over the full height of the menu bar
    // therefore we create a second pixmap which has the full height
    QSize fullHeightSize(!pixmap.isNull() ? pixmap.width():
                                            menuHeight * devicePixelRatio,
                         menuHeight * devicePixelRatio);
    QPixmap fullHeightPixmap(fullHeightSize);
    fullHeightPixmap.fill(Qt::transparent);
    if (!pixmap.isNull()) {
        QPainter p(&fullHeightPixmap);
        QRect r = pixmap.rect();
        r.moveCenter(fullHeightPixmap.rect().center());
        p.drawPixmap(r, pixmap);
    }

    NSImage *nsimage = static_cast<NSImage *>(qt_mac_create_nsimage(fullHeightPixmap));
    [nsimage setTemplate:icon.isMask()];
    [(NSImageView*)[[m_sys->item item] view] setImage: nsimage];
    [nsimage release];
}

void QCocoaSystemTrayIcon::updateMenu(QPlatformMenu *menu)
{
    if (!m_sys)
        return;

    m_sys->item->menu = static_cast<QCocoaMenu *>(menu);
    if (menu && [m_sys->item->menu->nsMenu() numberOfItems] > 0) {
        [[m_sys->item item] setHighlightMode:YES];
    } else {
        [[m_sys->item item] setHighlightMode:NO];
    }
}

void QCocoaSystemTrayIcon::updateToolTip(const QString &toolTip)
{
    if (!m_sys)
        return;
    [[[m_sys->item item] view] setToolTip:toolTip.toNSString()];
}

bool QCocoaSystemTrayIcon::isSystemTrayAvailable() const
{
    return true;
}

bool QCocoaSystemTrayIcon::supportsMessages() const
{
    return true;
}

void QCocoaSystemTrayIcon::showMessage(const QString &title, const QString &message,
                                       const QIcon& icon, MessageIcon, int)
{
    Q_UNUSED(icon);
    if (!m_sys)
        return;

    NSUserNotification *notification = [[NSUserNotification alloc] init];
    notification.title = [NSString stringWithUTF8String:title.toUtf8().data()];
    notification.informativeText = [NSString stringWithUTF8String:message.toUtf8().data()];

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
}
QT_END_NAMESPACE

@implementation NSStatusItem (Qt)
@end

@implementation QNSImageView
-(id)initWithParent:(QNSStatusItem*)myParent {
    self = [super init];
    parent = myParent;
    down = NO;
    return self;
}

-(void)menuTrackingDone:(NSNotification*)notification
{
    Q_UNUSED(notification);
    down = NO;

    [self setNeedsDisplay:YES];
}

-(void)mousePressed:(NSEvent *)mouseEvent button:(Qt::MouseButton)mouseButton
{
    down = YES;
    int clickCount = [mouseEvent clickCount];
    [self setNeedsDisplay:YES];

    if (clickCount == 2) {
        [self menuTrackingDone:nil];
        [parent doubleClickSelector:self];
    } else {
        [parent triggerSelector:self button:mouseButton];
    }
}

-(void)mouseDown:(NSEvent *)mouseEvent
{
    [self mousePressed:mouseEvent button:Qt::LeftButton];
}

-(void)mouseUp:(NSEvent *)mouseEvent
{
    Q_UNUSED(mouseEvent);
    [self menuTrackingDone:nil];
}

- (void)rightMouseDown:(NSEvent *)mouseEvent
{
    [self mousePressed:mouseEvent button:Qt::RightButton];
}

-(void)rightMouseUp:(NSEvent *)mouseEvent
{
    Q_UNUSED(mouseEvent);
    [self menuTrackingDone:nil];
}

- (void)otherMouseDown:(NSEvent *)mouseEvent
{
    [self mousePressed:mouseEvent button:cocoaButton2QtButton([mouseEvent buttonNumber])];
}

-(void)otherMouseUp:(NSEvent *)mouseEvent
{
    Q_UNUSED(mouseEvent);
    [self menuTrackingDone:nil];
}

-(void)drawRect:(NSRect)rect {
    [[parent item] drawStatusBarBackgroundInRect:rect withHighlight:down];
    [super drawRect:rect];
}
@end

@implementation QNSStatusItem

-(id)initWithSysTray:(QCocoaSystemTrayIcon *)sys
{
    self = [super init];
    if (self) {
        item = [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
        menu = 0;
        systray = sys;
        imageCell = [[QNSImageView alloc] initWithParent:self];
        [item setView: imageCell];
    }
    return self;
}

-(void)dealloc {
    [[NSStatusBar systemStatusBar] removeStatusItem:item];
    [[NSNotificationCenter defaultCenter] removeObserver:imageCell];
    [imageCell release];
    [item release];
    [super dealloc];

}

-(NSStatusItem*)item {
    return item;
}
-(QRectF)geometry {
    if (NSWindow *window = [[item view] window]) {
        NSRect screenRect = [[window screen] frame];
        NSRect windowRect = [window frame];
        return QRectF(windowRect.origin.x, screenRect.size.height-windowRect.origin.y-windowRect.size.height, windowRect.size.width, windowRect.size.height);
    }
    return QRectF();
}

- (void)triggerSelector:(id)sender button:(Qt::MouseButton)mouseButton {
    Q_UNUSED(sender);
    if (!systray)
        return;

    if (mouseButton == Qt::MidButton)
        emit systray->activated(QPlatformSystemTrayIcon::MiddleClick);
    else
        emit systray->activated(QPlatformSystemTrayIcon::Trigger);

    if (menu) {
        NSMenu *m = menu->nsMenu();
        [[NSNotificationCenter defaultCenter] addObserver:imageCell
         selector:@selector(menuTrackingDone:)
             name:NSMenuDidEndTrackingNotification
                 object:m];
        [item popUpStatusItemMenu: m];
    }
}

- (void)doubleClickSelector:(id)sender {
    Q_UNUSED(sender);
    if (!systray)
        return;
    emit systray->activated(QPlatformSystemTrayIcon::DoubleClick);
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification {
    Q_UNUSED(center);
    Q_UNUSED(notification);
    return YES;
}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification {
    Q_UNUSED(center);
    Q_UNUSED(notification);
    emit systray->messageClicked();
}

@end

class QSystemTrayIconQMenu : public QPlatformMenu
{
public:
    void doAboutToShow() { emit aboutToShow(); }
private:
    QSystemTrayIconQMenu();
};

@implementation QNSMenu
-(id)initWithQMenu:(QPlatformMenu*)qm {
    self = [super init];
    if (self) {
        self->qmenu = qm;
        [self setDelegate:self];
    }
    return self;
}
-(QPlatformMenu*)menu {
    return qmenu;
}
@end

#endif // QT_NO_SYSTEMTRAYICON
