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

#include "qcocoasystemtrayicon.h"

#ifndef QT_NO_SYSTEMTRAYICON

#include <qtemporaryfile.h>
#include <qimagewriter.h>
#include <qdebug.h>

#include <QtCore/private/qcore_mac_p.h>

#include "qcocoamenu.h"

#include "qcocoahelpers.h"
#include "qcocoaintegration.h"
#include "qcocoascreen.h"
#include <QtGui/private/qcoregraphics_p.h>

#import <AppKit/AppKit.h>

QT_BEGIN_NAMESPACE

void QCocoaSystemTrayIcon::init()
{
    m_statusItem = [[NSStatusBar.systemStatusBar statusItemWithLength:NSSquareStatusItemLength] retain];

    m_delegate = [[QStatusItemDelegate alloc] initWithSysTray:this];

    m_statusItem.button.target = m_delegate;
    m_statusItem.button.action = @selector(statusItemClicked);
    [m_statusItem.button sendActionOn:NSEventMaskLeftMouseUp | NSEventMaskRightMouseUp | NSEventMaskOtherMouseUp];
}

void QCocoaSystemTrayIcon::cleanup()
{
    NSUserNotificationCenter *center = NSUserNotificationCenter.defaultUserNotificationCenter;
    if (center.delegate == m_delegate)
        center.delegate = nil;

    [NSStatusBar.systemStatusBar removeStatusItem:m_statusItem];
    [m_statusItem release];
    m_statusItem = nil;

    [m_delegate release];
    m_delegate = nil;

    m_menu = nullptr;
}

QRect QCocoaSystemTrayIcon::geometry() const
{
    if (!m_statusItem)
        return QRect();

    if (NSWindow *window = m_statusItem.button.window) {
        if (QCocoaScreen *screen = QCocoaScreen::get(window.screen))
            return screen->mapFromNative(window.frame).toRect();
    }

    return QRect();
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
    if (!m_statusItem)
        return;

    // The recommended maximum title bar icon height is 18 points
    // (device independent pixels). The menu height on past and
    // current OS X versions is 22 points. Provide some future-proofing
    // by deriving the icon height from the menu height.
    const int padding = 4;
    const int menuHeight = NSStatusBar.systemStatusBar.thickness;
    const int maxImageHeight = menuHeight - padding;

    // Select pixmap based on the device pixel height. Ideally we would use
    // the devicePixelRatio of the target screen, but that value is not
    // known until draw time. Use qApp->devicePixelRatio, which returns the
    // devicePixelRatio for the "best" screen on the system.
    qreal devicePixelRatio = qApp->devicePixelRatio();
    const int maxPixmapHeight = maxImageHeight * devicePixelRatio;
    QSize selectedSize;
    for (const QSize& size : sortByHeight(icon.availableSizes())) {
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

    auto *nsimage = [NSImage imageFromQImage:fullHeightPixmap.toImage()];
    [nsimage setTemplate:icon.isMask()];
    m_statusItem.button.image = nsimage;
    m_statusItem.button.imageScaling = NSImageScaleProportionallyDown;
}

void QCocoaSystemTrayIcon::updateMenu(QPlatformMenu *menu)
{
    // We don't set the menu property of the NSStatusItem here,
    // as that would prevent us from receiving the action for the
    // click, and we wouldn't be able to emit the activated signal.
    // Instead we show the menu manually when the status item is
    // clicked.
    m_menu = static_cast<QCocoaMenu *>(menu);
}

void QCocoaSystemTrayIcon::updateToolTip(const QString &toolTip)
{
    if (!m_statusItem)
        return;

    m_statusItem.button.toolTip = toolTip.toNSString();
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
                                       const QIcon& icon, MessageIcon, int msecs)
{
    if (!m_statusItem)
        return;

    auto *notification = [[NSUserNotification alloc] init];
    notification.title = title.toNSString();
    notification.informativeText = message.toNSString();
    notification.contentImage = [NSImage imageFromQIcon:icon];

    NSUserNotificationCenter *center = NSUserNotificationCenter.defaultUserNotificationCenter;
    center.delegate = m_delegate;

    [center deliverNotification:[notification autorelease]];

    if (msecs) {
        NSTimeInterval timeout = msecs / 1000.0;
        [center performSelector:@selector(removeDeliveredNotification:) withObject:notification afterDelay:timeout];
    }
}

void QCocoaSystemTrayIcon::statusItemClicked()
{
    auto *mouseEvent = NSApp.currentEvent;

    auto activationReason = QPlatformSystemTrayIcon::Unknown;

    if (mouseEvent.clickCount == 2) {
        activationReason = QPlatformSystemTrayIcon::DoubleClick;
    } else {
        auto mouseButton = cocoaButton2QtButton(mouseEvent);
        if (mouseButton == Qt::MiddleButton)
            activationReason = QPlatformSystemTrayIcon::MiddleClick;
        else if (mouseButton == Qt::RightButton)
            activationReason = QPlatformSystemTrayIcon::Context;
        else
            activationReason = QPlatformSystemTrayIcon::Trigger;
    }

    emit activated(activationReason);

    if (NSMenu *menu = m_menu ? m_menu->nsMenu() : nil)
        [m_statusItem popUpStatusItemMenu:menu];
}

QT_END_NAMESPACE

@implementation QStatusItemDelegate

- (instancetype)initWithSysTray:(QCocoaSystemTrayIcon *)platformSystemTray
{
    if ((self = [super init]))
        self.platformSystemTray = platformSystemTray;

    return self;
}

- (void)dealloc
{
    self.platformSystemTray = nullptr;
    [super dealloc];
}

- (void)statusItemClicked
{
    self.platformSystemTray->statusItemClicked();
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    Q_UNUSED(center);
    Q_UNUSED(notification);
    return YES;
}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    [center removeDeliveredNotification:notification];
    emit self.platformSystemTray->messageClicked();
}

@end

#endif // QT_NO_SYSTEMTRAYICON
