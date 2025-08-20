// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNSWINDOWDELEGATE_H
#define QNSWINDOWDELEGATE_H

#include <QtCore/private/qcore_mac_p.h>

#import <AppKit/NSWindow.h>

QT_BEGIN_NAMESPACE
class QCocoaWindow;
QT_END_NAMESPACE

QT_DECLARE_NAMESPACED_OBJC_INTERFACE(QNSWindowDelegate, NSObject <NSWindowDelegate>)

#endif // QNSWINDOWDELEGATE_H
