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

#include "qeglfsintegration.h"

#include "qeglfswindow.h"
#include "qeglfswindowsurface.h"

#include "qgenericunixfontdatabase.h"

#include <QtGui/QPlatformWindow>
#include <QtGui/QPlatformWindowFormat>
#include <QtOpenGL/private/qpixmapdata_gl_p.h>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

QEglFSIntegration::QEglFSIntegration()
    : mFontDb(new QGenericUnixFontDatabase())
{
    m_primaryScreen = new QEglFSScreen(EGL_DEFAULT_DISPLAY);

    mScreens.append(m_primaryScreen);
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglIntegration\n");
#endif
}

bool QEglFSIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPixmapData *QEglFSIntegration::createPixmapData(QPixmapData::PixelType type) const
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglIntegration::createPixmapData %d\n", type);
#endif
    return new QGLPixmapData(type);
}

QPlatformWindow *QEglFSIntegration::createPlatformWindow(QWidget *widget, WId winId) const
{
    Q_UNUSED(winId);
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglIntegration::createPlatformWindow %p\n",widget);
#endif
    return new QEglFSWindow(widget, m_primaryScreen);
}


QWindowSurface *QEglFSIntegration::createWindowSurface(QWidget *widget, WId winId) const
{
    Q_UNUSED(winId);

#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglIntegration::createWindowSurface %p\n",widget);
#endif
    return new QEglFSWindowSurface(m_primaryScreen,widget);
}

QPlatformFontDatabase *QEglFSIntegration::fontDatabase() const
{
    return mFontDb;
}

QT_END_NAMESPACE
