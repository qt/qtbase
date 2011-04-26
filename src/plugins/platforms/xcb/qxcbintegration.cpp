/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbintegration.h"
#include "qxcbconnection.h"
#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbwindowsurface.h"
#include "qxcbnativeinterface.h"

#include <xcb/xcb.h>

#include <private/qpixmap_raster_p.h>

#include "qgenericunixfontdatabase.h"

#include <stdio.h>

#ifdef XCB_USE_EGL
#include <EGL/egl.h>
#endif

QXcbIntegration::QXcbIntegration()
    : m_connection(new QXcbConnection)
{
    foreach (QXcbScreen *screen, m_connection->screens())
        m_screens << screen;

    m_fontDatabase = new QGenericUnixFontDatabase();
    m_nativeInterface = new QXcbNativeInterface;
}

QXcbIntegration::~QXcbIntegration()
{
    delete m_connection;
}

bool QXcbIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return hasOpenGL();
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPixmapData *QXcbIntegration::createPixmapData(QPixmapData::PixelType type) const
{
    return new QRasterPixmapData(type);
}

QPlatformWindow *QXcbIntegration::createPlatformWindow(QWindow *window) const
{
    return new QXcbWindow(window);
}

QWindowSurface *QXcbIntegration::createWindowSurface(QWindow *window, WId winId) const
{
    Q_UNUSED(winId);
    return new QXcbWindowSurface(window->widget());
}

QList<QPlatformScreen *> QXcbIntegration::screens() const
{
    return m_screens;
}

void QXcbIntegration::moveToScreen(QWidget *window, int screen)
{
    Q_UNUSED(window);
    Q_UNUSED(screen);
}

bool QXcbIntegration::isVirtualDesktop()
{
    return false;
}

QPlatformFontDatabase *QXcbIntegration::fontDatabase() const
{
    return m_fontDatabase;
}

QPixmap QXcbIntegration::grabWindow(WId window, int x, int y, int width, int height) const
{
    Q_UNUSED(window);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(width);
    Q_UNUSED(height);
    return QPixmap();
}


bool QXcbIntegration::hasOpenGL() const
{
#if defined(XCB_USE_GLX)
    return true;
#elif defined(XCB_USE_EGL)
    return m_connection->hasEgl();
#elif defined(XCB_USE_DRI2)
    if (m_connection->hasSupportForDri2()) {
        return true;
    }
#endif
    return false;
}

QPlatformNativeInterface * QXcbIntegration::nativeInterface() const
{
    return m_nativeInterface;
}
