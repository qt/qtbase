// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// some versions of CALayer.h use 'slots' as an identifier
#define QT_NO_KEYWORDS

#include "tst_qwidget_mac_helpers.h"
#include <QApplication>
#include <qpa/qplatformnativeinterface.h>
#include <private/qcore_mac_p.h>

#include <AppKit/AppKit.h>

QString nativeWindowTitle(QWidget *window, Qt::WindowState state)
{
    QWindow *qwindow = window->windowHandle();
    NSWindow *nswindow = (NSWindow *) qApp->platformNativeInterface()->nativeResourceForWindow("nswindow", qwindow);
    QCFString macTitle;
    if (state == Qt::WindowMinimized) {
        macTitle = reinterpret_cast<CFStringRef>([[nswindow miniwindowTitle] retain]);
    } else {
        macTitle = reinterpret_cast<CFStringRef>([[nswindow title] retain]);
    }
    return macTitle;
}

bool nativeWindowModified(QWidget *widget)
{
    QWindow *qwindow = widget->windowHandle();
    NSWindow *nswindow = (NSWindow *) qApp->platformNativeInterface()->nativeResourceForWindow("nswindow", qwindow);
    return [nswindow isDocumentEdited];
}
