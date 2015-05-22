/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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

#include <private/qt_mac_p.h>
#include <AppKit/AppKit.h>

void click_cocoa_button()
{
    QMacAutoReleasePool pool;
    NSArray *windows = [NSApp windows];
    for (NSWindow *window in windows) {
        // This is NOT how one should do RTTI, but since I don't want to leak the class too much...
        if ([[window delegate] respondsToSelector:@selector(qtFont)]) {
            NSArray *subviews = [[window contentView] subviews];
            for (NSView *view in subviews) {
                if ([view isKindOfClass:[NSButton class]]
                        && [[static_cast<NSButton *>(view) title] isEqualTo:@"OK"]) {
                    [static_cast<NSButton *>(view) performClick:view];
                    [NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined location:NSZeroPoint
                        modifierFlags:0 timestamp:0. windowNumber:0 context:0
                        subtype:SHRT_MAX data1:0 data2:0] atStart:NO];

                    break;
                }
            }
            break;
        }
    }
}
