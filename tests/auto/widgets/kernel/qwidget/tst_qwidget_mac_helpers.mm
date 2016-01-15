/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
