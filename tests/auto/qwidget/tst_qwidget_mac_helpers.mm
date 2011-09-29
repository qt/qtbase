/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "tst_qwidget_mac_helpers.h"
#include <private/qt_mac_p.h>
#include <private/qt_cocoa_helpers_mac_p.h>


QString nativeWindowTitle(QWidget *window, Qt::WindowState state)
{
    OSWindowRef windowRef = qt_mac_window_for(window);
    QCFString macTitle;
    if (state == Qt::WindowMinimized) {
        macTitle = reinterpret_cast<CFStringRef>([[windowRef miniwindowTitle] retain]);
    } else {
        macTitle = reinterpret_cast<CFStringRef>([[windowRef title] retain]);
    }
    return macTitle;
}

bool nativeWindowModified(QWidget *widget)
{
    return [qt_mac_window_for(widget) isDocumentEdited];
}

bool testAndRelease(const WId view)
{
    if ([id(view) retainCount] != 2)
        return false;
    [id(view) release];
    [id(view) release];
    return true;
}

WidgetViewPair createAndRetain(QWidget * const parent)
{
    QWidget * const widget = new QWidget(parent);
    const WId view = widget->winId();
    // Retain twice so we can safely call retainCount even if the retain count
    // is off by one because of a double release.
    [id(view) retain];
    [id(view) retain];
    return qMakePair(widget, view);
}

