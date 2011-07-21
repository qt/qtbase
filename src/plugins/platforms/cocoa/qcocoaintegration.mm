/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qcocoaintegration.h"

#include "qcocoawindow.h"
#include "qcocoabackingstore.h"
#include "qcocoanativeinterface.h"

#include "qcocoaeventdispatcher.h"
#include <QtPlatformSupport/private/qbasicunixfontdatabase_p.h>

QT_BEGIN_NAMESPACE

QCocoaScreen::QCocoaScreen(int screenIndex)
    :QPlatformScreen()
{
    m_screen = [[NSScreen screens] objectAtIndex:screenIndex];
    NSRect rect = [m_screen frame];
    m_geometry = QRect(rect.origin.x,rect.origin.y,rect.size.width,rect.size.height);

    m_format = QImage::Format_ARGB32;

    m_depth = NSBitsPerPixelFromDepth([m_screen depth]);

    const int dpi = 72;
    const qreal inch = 25.4;
    m_physicalSize = QSize(qRound(m_geometry.width() * inch / dpi), qRound(m_geometry.height() *inch / dpi));
}

QCocoaScreen::~QCocoaScreen()
{
}

QCocoaIntegration::QCocoaIntegration()
    : mFontDb(new QBasicUnixFontDatabase())
{
    mPool = new QCocoaAutoReleasePool;

    //Make sure we have a nsapplication :)
    [NSApplication sharedApplication];
//    [[OurApplication alloc] init];

    NSArray *screens = [NSScreen screens];
    for (uint i = 0; i < [screens count]; i++) {
        QCocoaScreen *screen = new QCocoaScreen(i);
        screenAdded(screen);
    }
}

QCocoaIntegration::~QCocoaIntegration()
{
    delete mPool;
}

bool QCocoaIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL : return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}



QPlatformWindow *QCocoaIntegration::createPlatformWindow(QWindow *window) const
{
    return new QCocoaWindow(window);
}

QPlatformGLContext *QCocoaIntegration::createPlatformGLContext(QGuiGLContext *context) const
{
    return new QCocoaGLContext(context->format(), context->shareHandle());
}

QPlatformBackingStore *QCocoaIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QCocoaBackingStore(window);
}

QAbstractEventDispatcher *QCocoaIntegration::createEventDispatcher() const
{
    return new QCocoaEventDispatcher();
}

QPlatformFontDatabase *QCocoaIntegration::fontDatabase() const
{
    return mFontDb;
}

QPlatformNativeInterface *QCocoaIntegration::nativeInterface() const
{
    return new QCocoaNativeInterface();
}

QT_END_NAMESPACE
