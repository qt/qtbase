/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the documentation of the Qt Toolkit.
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

#include <private/qt_mac_p.h>
#include <AppKit/AppKit.h>

void click_cocoa_button()
{
    QMacCocoaAutoReleasePool pool;
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
