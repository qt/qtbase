/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgraphicssystem_vglite.h"
#include "qwindowsurface_vglite.h"
#include <QtOpenVG/qplatformpixmap_vg_p.h>
#include <QtGui/private/qegl_p.h>
#include <QtCore/qdebug.h>
#ifdef OPENVG_USBHP_INIT
extern "C" {
#include <linuxusbhp.h>
};
#endif

QT_BEGIN_NAMESPACE

QVGLiteGraphicsSystem::QVGLiteGraphicsSystem()
    : w(0), h(0), d(0), dw(0), dh(0), physWidth(0), physHeight(0),
      surface(0), context(0), rootWindow(0),
      screenFormat(QImage::Format_RGB16), preservedSwap(false)
{
#ifdef OPENVG_USBHP_INIT
    initLibrary();
#endif

    // The graphics system is also the screen definition.
    mScreens.append(this);

    QString displaySpec = QString::fromLatin1(qgetenv("QWS_DISPLAY"));
    QStringList displayArgs = displaySpec.split(QLatin1Char(':'));

    // Initialize EGL and create the global EGL context.
    context = qt_vg_create_context(0);
    if (!context) {
        qFatal("QVGLiteGraphicsSystem: could not initialize EGL");
        return;
    }

    // Get the root window handle to use.  Default to zero.
    QRegExp winidRx(QLatin1String("winid=?(\\d+)"));
    int winidIdx = displayArgs.indexOf(winidRx);
    int handle = 0;
    if (winidIdx >= 0) {
        winidRx.exactMatch(displayArgs.at(winidIdx));
        handle = winidRx.cap(1).toInt();
    }

    // Create a full-screen window based on the native handle.
    // If the context is premultiplied, the window should be too.
    QEglProperties props;
#ifdef EGL_VG_ALPHA_FORMAT_PRE_BIT
    EGLint surfaceType = 0;
    if (context->configAttrib(EGL_SURFACE_TYPE, &surfaceType) &&
        (surfaceType & EGL_VG_ALPHA_FORMAT_PRE_BIT) != 0)
        props.setValue(EGL_VG_ALPHA_FORMAT, EGL_VG_ALPHA_FORMAT_PRE);
#endif
    rootWindow = eglCreateWindowSurface
        (context->display(), context->config(),
         (EGLNativeWindowType)handle, props.properties());
    if (rootWindow == EGL_NO_SURFACE) {
        delete context;
        context = 0;
        qFatal("QVGLiteGraphicsSystem: could not create full-screen window");
        return;
    }

    // Try to turn on preserved swap behaviour on the root window.
    // This will allow us to optimize compositing to focus on just
    // the screen region that has changed.  Otherwise we must
    // re-composite the entire screen every frame.
#if !defined(QVG_NO_PRESERVED_SWAP)
    eglGetError();  // Clear error state first.
    eglSurfaceAttrib(context->display(), rootWindow,
                     EGL_SWAP_BEHAVIOR, EGL_BUFFER_PRESERVED);
    preservedSwap = (eglGetError() == EGL_SUCCESS);
#else
    preservedSwap = false;
#endif

    // Fetch the root window properties.
    eglQuerySurface(context->display(), rootWindow, EGL_WIDTH, &w);
    eglQuerySurface(context->display(), rootWindow, EGL_HEIGHT, &h);
    screenFormat = qt_vg_config_to_image_format(context);
    switch (screenFormat) {
        case QImage::Format_ARGB32_Premultiplied:
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        default:
            d = 32;
            break;
        case QImage::Format_RGB16:
        case QImage::Format_ARGB4444_Premultiplied:
            d = 16;
            break;
    }
    dw = w;
    dh = h;
    qDebug("screen size: %dx%dx%d", w, h, d);

    // Handle display physical size spec.  From qscreenlinuxfb_qws.cpp.
    QRegExp mmWidthRx(QLatin1String("mmWidth=?(\\d+)"));
    int dimIdxW = displayArgs.indexOf(mmWidthRx);
    QRegExp mmHeightRx(QLatin1String("mmHeight=?(\\d+)"));
    int dimIdxH = displayArgs.indexOf(mmHeightRx);
    if (dimIdxW >= 0) {
        mmWidthRx.exactMatch(displayArgs.at(dimIdxW));
        physWidth = mmWidthRx.cap(1).toInt();
        if (dimIdxH < 0)
            physHeight = dh*physWidth/dw;
    }
    if (dimIdxH >= 0) {
        mmHeightRx.exactMatch(displayArgs.at(dimIdxH));
        physHeight = mmHeightRx.cap(1).toInt();
        if (dimIdxW < 0)
            physWidth = dw*physHeight/dh;
    }
    if (dimIdxW < 0 && dimIdxH < 0) {
        const int dpi = 72;
        physWidth = qRound(dw * 25.4 / dpi);
        physHeight = qRound(dh * 25.4 / dpi);
    }
}

QVGLiteGraphicsSystem::~QVGLiteGraphicsSystem()
{
}

QPlatformPixmap *QVGLiteGraphicsSystem::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
#if !defined(QVGLite_NO_SINGLE_CONTEXT) && !defined(QVGLite_NO_PIXMAP_DATA)
    // Pixmaps can use QVGLitePlatformPixmap; bitmaps must use raster.
    if (type == QPlatformPixmap::PixmapType)
        return new QVGPlatformPixmap(type);
    else
        return new QRasterPlatformPixmap(type);
#else
    return new QRasterPlatformPixmap(type);
#endif
}

QWindowSurface *QVGLiteGraphicsSystem::createWindowSurface(QWidget *widget) const
{
    if (widget->windowType() == Qt::Desktop)
        return 0;   // Don't create an explicit window surface for the destkop.
    if (surface) {
        qWarning() << "QVGLiteGraphicsSystem: only one window surface "
                      "is supported at a time";
        return 0;
    }
    surface = new QVGLiteWindowSurface
        (const_cast<QVGLiteGraphicsSystem *>(this), widget);
    return surface;
}

QT_END_NAMESPACE
