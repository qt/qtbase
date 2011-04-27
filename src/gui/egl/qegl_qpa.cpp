/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <QtGui/qpaintdevice.h>
#include <QtGui/qpixmap.h>
#include <QtGui/qwidget.h>
#include "qeglcontext_p.h"

#if !defined(QT_NO_EGL)

#include <QtGui/private/qgraphicssystem_p.h>
#include <QtGui/private/qapplication_p.h>
#include <QtGui/qdesktopwidget.h>

QT_BEGIN_NAMESPACE

EGLNativeDisplayType QEgl::nativeDisplay()
{
    return EGLNativeDisplayType(EGL_DEFAULT_DISPLAY);
}

EGLNativeWindowType QEgl::nativeWindow(QWidget* widget)
{
    return (EGLNativeWindowType)(widget->winId());
}

EGLNativePixmapType QEgl::nativePixmap(QPixmap* pixmap)
{
    Q_UNUSED(pixmap);
    return 0;
}

//EGLDisplay QEglContext::display()
//{
//    return eglGetDisplay(EGLNativeDisplayType(EGL_DEFAULT_DISPLAY));
//}

static QPlatformScreen *screenForDevice(QPaintDevice *device)
{
    QPlatformIntegration *pi = QApplicationPrivate::platformIntegration();

    QList<QPlatformScreen *> screens = pi->screens();

    int screenNumber;
    if (device && device->devType() == QInternal::Widget)
        screenNumber = qApp->desktop()->screenNumber(static_cast<QWidget *>(device));
    else
        screenNumber = 0;
    if (screenNumber < 0 || screenNumber >= screens.size())
        return 0;
    return screens[screenNumber];
}

// Set pixel format and other properties based on a paint device.
void QEglProperties::setPaintDeviceFormat(QPaintDevice *dev)
{
    if (!dev)
        return;

    // Find the QGLScreen for this paint device.
    QPlatformScreen *screen = screenForDevice(dev);
    if (!screen)
        return;
    int devType = dev->devType();
    if (devType == QInternal::Image)
        setPixelFormat(static_cast<QImage *>(dev)->format());
    else
        setPixelFormat(screen->format());
}

QT_END_NAMESPACE

#endif // !QT_NO_EGL
