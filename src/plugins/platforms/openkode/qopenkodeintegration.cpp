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

#include "qopenkodeintegration.h"
#include "qopenkodewindow.h"
#include "qopenkodeeventloopintegration.h"

#include <QtOpenGL/private/qpixmapdata_gl_p.h>
#include <QtOpenGL/private/qwindowsurface_gl_p.h>

#include <QtGui/private/qpixmap_raster_p.h>

#include <QtCore/qdebug.h>
#include <QtCore/qthread.h>
#include <QtCore/qfile.h>

#include "qgenericunixfontdatabase.h"

#include <KD/kd.h>
#include <KD/NV_display.h>
#include <KD/NV_initialize.h>

#include <EGL/egl.h>

#include "GLES2/gl2ext.h"

QT_BEGIN_NAMESPACE

QOpenKODEScreen::QOpenKODEScreen(KDDisplayNV *kdDisplay,  KDDesktopNV *kdDesktop)
    : mIsFullScreen(false)
{
    qDebug() << "QOpenKODEScreen::QOpenKODEIntegrationScreen()";

    KDboolean enabled = KD_TRUE;
    kdSetDisplayPropertybvNV(kdDisplay,
                             KD_DISPLAYPROPERTY_ENABLED_NV,
                             &enabled);
    KDboolean power = KD_DISPLAY_POWER_ON;
    kdSetDisplayPropertyivNV(kdDisplay,
                             KD_DISPLAYPROPERTY_POWER_NV,
                             &power);

    kdSetDisplayPropertycvNV(kdDisplay,
                             KD_DISPLAYPROPERTY_DESKTOP_NAME_NV,
                             KD_DEFAULT_DESKTOP_NV);

    KDDisplayModeNV mode;
    if (kdGetDisplayModeNV(kdDisplay, &mode)) {
        qErrnoWarning(kdGetError(), "Could not get display mode");
        return;
    }

    qDebug() << " - display mode " << mode.width << "x" << mode.height << " refresh " << mode.refresh;

    KDint desktopSize[] = { mode.width, mode.height };

    if (kdSetDesktopPropertyivNV(kdDesktop, KD_DESKTOPPROPERTY_SIZE_NV, desktopSize)) {
        qErrnoWarning(kdGetError(), "Could not set desktop size");
        return;
    }

    // Once we've set up the desktop and display we don't need them anymore
    kdReleaseDisplayNV(kdDisplay);
    kdReleaseDesktopNV(kdDesktop);

    mEglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEglDisplay == EGL_NO_DISPLAY) {
        qErrnoWarning("EGL failed to obtain display");
    }

    /* Initialize EGL display */
    EGLBoolean rvbool = eglInitialize(mEglDisplay, 0, 0);
    if (!rvbool) {
        qErrnoWarning("EGL failed to initialize display");
    }

//    cursor = new QOpenKODECursor(this);

    mGeometry = QRect(0, 0, mode.width, mode.height);
    mDepth = 24;
    mFormat = QImage::Format_RGB32;


}

QOpenKODEIntegration::QOpenKODEIntegration()
    : mEventLoopIntegration(0)
    , mFontDb(new QGenericUnixFontDatabase())
    , mMainGlContext(0)
{
    if (kdInitializeNV() == KD_ENOTINITIALIZED) {
        qFatal("Did not manage to initialize openkode");
    }

    KDDisplaySystemNV *kdDisplaySystem = kdCreateDisplaySystemSnapshotNV(this);
    KDint32 displayCount = 0;
    kdGetDisplaySystemPropertyivNV(kdDisplaySystem, KD_DISPLAYPROPERTY_COUNT_NV, 0, &displayCount);

    for (int i = 0; i < displayCount; i++) {
        KDchar *displayName = 0;
        KDsize displayNameLength = 0;
        kdGetDisplaySystemPropertycvNV(kdDisplaySystem,KD_DISPLAYPROPERTY_NAME_NV,i,0,&displayNameLength);
        if (!displayNameLength)
            continue;
        displayName = new KDchar[displayNameLength];
        kdGetDisplaySystemPropertycvNV(kdDisplaySystem,KD_DISPLAYPROPERTY_NAME_NV,i,displayName,&displayNameLength);

        KDDisplayNV *display = kdGetDisplayNV(displayName,this);
        if (!display || display == (void*)-1) {
            qErrnoWarning(kdGetError(), "Could not obtain KDDisplayNV pointer");
            return;
        }
        if (displayNameLength)
            delete displayName;

        KDchar *desktopName = 0;
        KDsize desktopNameLength = 0;
        bool openkodeImpDoesNotFail = false;
        if (openkodeImpDoesNotFail) {
            qDebug() << "printing desktopname";
            kdGetDisplayPropertycvNV(display,KD_DISPLAYPROPERTY_DESKTOP_NAME_NV,desktopName,&desktopNameLength);
            if (desktopNameLength) {
                desktopName = new KDchar[desktopNameLength];
                kdGetDisplayPropertycvNV(display,KD_DISPLAYPROPERTY_DESKTOP_NAME_NV,desktopName,&desktopNameLength);
            } else {
                desktopName =  KD_DEFAULT_DESKTOP_NV;
            }
        } else {
            desktopName = KD_DEFAULT_DESKTOP_NV;
        }

        KDDesktopNV *desktop = kdGetDesktopNV(desktopName,this);
        if (!desktop || desktop == (void*)-1) {
            qErrnoWarning(kdGetError(), "Could not obtain KDDesktopNV pointer");
            kdReleaseDisplayNV(display);
            return;
        }
        if (desktopNameLength)
            delete desktopName;

        QOpenKODEScreen *screen = new QOpenKODEScreen(display,desktop);
        mScreens.append(screen);
    }
}

QOpenKODEIntegration::~QOpenKODEIntegration()
{
    delete mEventLoopIntegration;
    delete mFontDb;
}


bool QOpenKODEIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPixmapData *QOpenKODEIntegration::createPixmapData(QPixmapData::PixelType type) const
{
    return new QGLPixmapData(type);
}

QPlatformWindow *QOpenKODEIntegration::createPlatformWindow(QWidget *tlw, WId ) const
{
    return new QOpenKODEWindow(tlw);
}

QWindowSurface *QOpenKODEIntegration::createWindowSurface(QWidget *widget, WId) const
{
    QWindowSurface *returnSurface = 0;
    switch (widget->platformWindowFormat().windowApi()) {

    case QPlatformWindowFormat::Raster:
    case QPlatformWindowFormat::OpenGL:
        returnSurface = new QGLWindowSurface(widget);
        break;

    case QPlatformWindowFormat::OpenVG:
//        returnSurface = new QVGWindowSurface(widget);
//        break;

    default:
        returnSurface = new QGLWindowSurface(widget);
        break;
    }

    return returnSurface;
}

QPlatformEventLoopIntegration *QOpenKODEIntegration::createEventLoopIntegration() const
{
    if (!mEventLoopIntegration) {
        QOpenKODEIntegration *that = const_cast<QOpenKODEIntegration *>(this);
        that->mEventLoopIntegration = new QOpenKODEEventLoopIntegration;
    }
    return mEventLoopIntegration;
}

QPlatformFontDatabase *QOpenKODEIntegration::fontDatabase() const
{
    return mFontDb;
}


QT_END_NAMESPACE
