// Copyright (C) 2016 Samuel Gaist <samuel.gaist@edeltech.ch>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
#include "mycocoaeventfilter.h"

#import <AppKit/AppKit.h>

bool MyCocoaEventFilter::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
    if (eventType == "mac_generic_NSEvent") {
        NSEvent *event = static_cast<NSEvent *>(message);
        if ([event type] == NSKeyDown) {
            // Handle key event
            qDebug() << QString::fromNSString([event characters]);
        }
    }
    return false;
}
//! [0]
