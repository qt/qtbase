// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
