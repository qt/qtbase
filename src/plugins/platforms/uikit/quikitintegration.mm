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

#include "quikitintegration.h"
#include "quikitwindow.h"
#include "quikitwindowsurface.h"
#include "quikitscreen.h"
#include "quikiteventloop.h"

#include <QtGui/QApplication>

#include <private/qpixmap_raster_p.h>

#include <UIKit/UIKit.h>

#include <QtDebug>

QT_BEGIN_NAMESPACE

QUIKitIntegration::QUIKitIntegration()
{
    mScreens << new QUIKitScreen(0);
}

QUIKitIntegration::~QUIKitIntegration()
{
}

QPlatformPixmap *QUIKitIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    return new QRasterPlatformPixmap(type);
}

QPlatformWindow *QUIKitIntegration::createPlatformWindow(QWidget *widget, WId winId) const
{
    Q_UNUSED(winId);
    return new QUIKitWindow(widget);
}

QList<QPlatformScreen *> QUIKitIntegration::screens() const
{
    return mScreens;
}

QWindowSurface *QUIKitIntegration::createWindowSurface(QWidget *widget, WId winId) const
{
    Q_UNUSED(winId);
    return new QUIKitWindowSurface(widget);
}

QPlatformEventLoopIntegration *QUIKitIntegration::createEventLoopIntegration() const
{
    return new QUIKitEventLoop();
}

QPlatformFontDatabase * QUIKitIntegration::fontDatabase() const
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        setenv("QT_QPA_FONTDIR",[[[[NSBundle mainBundle] bundlePath] stringByAppendingPathComponent:@"fonts"] UTF8String],1);
    }
    return QPlatformIntegration::fontDatabase();
}

QT_END_NAMESPACE
