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

#include "qmeegoextensions.h"
#include <private/qeglcontext_p.h>
#include <private/qpixmapdata_gl_p.h>

bool QMeeGoExtensions::initialized = false;
bool QMeeGoExtensions::hasImageShared = false;
bool QMeeGoExtensions::hasSurfaceScaling = false;
bool QMeeGoExtensions::hasLockSurface = false;
bool QMeeGoExtensions::hasFenceSync = false;

/* Extension funcs */

typedef EGLBoolean (EGLAPIENTRY *eglQueryImageNOKFunc)(EGLDisplay, EGLImageKHR, EGLint, EGLint*);
typedef EGLNativeSharedImageTypeNOK (EGLAPIENTRY *eglCreateSharedImageNOKFunc)(EGLDisplay, EGLImageKHR, EGLint*);
typedef EGLBoolean (EGLAPIENTRY *eglDestroySharedImageNOKFunc)(EGLDisplay, EGLNativeSharedImageTypeNOK);
typedef EGLBoolean (EGLAPIENTRY *eglSetSurfaceScalingNOKFunc)(EGLDisplay, EGLSurface, EGLint, EGLint, EGLint, EGLint);
typedef EGLBoolean (EGLAPIENTRY *eglLockSurfaceKHRFunc)(EGLDisplay, EGLSurface, const EGLint*);
typedef EGLBoolean (EGLAPIENTRY *eglUnlockSurfaceKHRFunc)(EGLDisplay, EGLSurface);
typedef EGLSyncKHR (EGLAPIENTRY *eglCreateSyncKHRFunc)(EGLDisplay, EGLenum, const EGLint*);
typedef EGLBoolean (EGLAPIENTRY *eglDestroySyncKHRFunc)(EGLDisplay, EGLSyncKHR);
typedef EGLint (EGLAPIENTRY *eglClientWaitSyncKHRFunc)(EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR);
typedef EGLBoolean (EGLAPIENTRY *eglGetSyncAttribKHRFunc)(EGLDisplay, EGLSyncKHR, EGLint, EGLint*);

static eglQueryImageNOKFunc _eglQueryImageNOK = 0;
static eglCreateSharedImageNOKFunc _eglCreateSharedImageNOK = 0;
static eglDestroySharedImageNOKFunc _eglDestroySharedImageNOK = 0;
static eglSetSurfaceScalingNOKFunc _eglSetSurfaceScalingNOK = 0;
static eglLockSurfaceKHRFunc _eglLockSurfaceKHR = 0;
static eglUnlockSurfaceKHRFunc _eglUnlockSurfaceKHR = 0;
static eglCreateSyncKHRFunc _eglCreateSyncKHR = 0;
static eglDestroySyncKHRFunc _eglDestroySyncKHR = 0;
static eglClientWaitSyncKHRFunc _eglClientWaitSyncKHR = 0;
static eglGetSyncAttribKHRFunc _eglGetSyncAttribKHR = 0;

/* Public */

void QMeeGoExtensions::ensureInitialized()
{
    if (!initialized)
        initialize();

    initialized = true;
}

EGLNativeSharedImageTypeNOK QMeeGoExtensions::eglCreateSharedImageNOK(EGLDisplay dpy, EGLImageKHR image, EGLint *props)
{
    if (!hasImageShared)
        qFatal("EGL_NOK_image_shared not found but trying to use capability!");

    return _eglCreateSharedImageNOK(dpy, image, props);
}

bool QMeeGoExtensions::eglQueryImageNOK(EGLDisplay dpy, EGLImageKHR image, EGLint prop, EGLint *v)
{
    if (!hasImageShared)
        qFatal("EGL_NOK_image_shared not found but trying to use capability!");

    return _eglQueryImageNOK(dpy, image, prop, v);
}

bool QMeeGoExtensions::eglDestroySharedImageNOK(EGLDisplay dpy, EGLNativeSharedImageTypeNOK img)
{
    if (!hasImageShared)
        qFatal("EGL_NOK_image_shared not found but trying to use capability!");

    return _eglDestroySharedImageNOK(dpy, img);
}

bool QMeeGoExtensions::eglSetSurfaceScalingNOK(EGLDisplay dpy, EGLSurface surface, int x, int y, int width, int height)
{
    if (!hasSurfaceScaling)
        qFatal("EGL_NOK_surface_scaling not found but trying to use capability!");

   return _eglSetSurfaceScalingNOK(dpy, surface, x, y, width, height);
}

bool QMeeGoExtensions::eglLockSurfaceKHR(EGLDisplay display, EGLSurface surface, const EGLint *attrib_list)
{
    if (!hasLockSurface)
        qFatal("EGL_KHR_lock_surface2 not found but trying to use capability!");

    return _eglLockSurfaceKHR(display, surface, attrib_list);
}

bool QMeeGoExtensions::eglUnlockSurfaceKHR(EGLDisplay display, EGLSurface surface)
{
    if (!hasLockSurface)
        qFatal("EGL_KHR_lock_surface2 not found but trying to use capability!");

    return _eglUnlockSurfaceKHR(display, surface);
}

EGLSyncKHR QMeeGoExtensions::eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    if (!hasFenceSync)
        qFatal("EGL_KHR_fence_sync not found but trying to use capability!");

    return _eglCreateSyncKHR(dpy, type, attrib_list);
}

bool QMeeGoExtensions::eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    if (!hasFenceSync)
        qFatal("EGL_KHR_fence_sync not found but trying to use capability!");

    return _eglDestroySyncKHR(dpy, sync);
}

EGLint QMeeGoExtensions::eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
    if (!hasFenceSync)
        qFatal("EGL_KHR_fence_sync not found but trying to use capability!");

    return _eglClientWaitSyncKHR(dpy, sync, flags, timeout);
}

EGLBoolean QMeeGoExtensions::eglGetSyncAttribKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint attribute, EGLint *value)
{
    if (!hasFenceSync)
        qFatal("EGL_KHR_fence_sync not found but trying to use capability!");

    return _eglGetSyncAttribKHR(dpy, sync, attribute, value);
}

/* Private */

void QMeeGoExtensions::initialize()
{
    QGLContext *ctx = (QGLContext *) QGLContext::currentContext();
    qt_resolve_eglimage_gl_extensions(ctx);

    if (QEgl::hasExtension("EGL_NOK_image_shared")) {
        qDebug("MeegoGraphics: found EGL_NOK_image_shared");
        _eglQueryImageNOK = (eglQueryImageNOKFunc) eglGetProcAddress("eglQueryImageNOK");
        _eglCreateSharedImageNOK = (eglCreateSharedImageNOKFunc) eglGetProcAddress("eglCreateSharedImageNOK");
        _eglDestroySharedImageNOK = (eglDestroySharedImageNOKFunc) eglGetProcAddress("eglDestroySharedImageNOK");
        _eglLockSurfaceKHR = (eglLockSurfaceKHRFunc) eglGetProcAddress("eglLockSurfaceKHR");
        _eglUnlockSurfaceKHR = (eglUnlockSurfaceKHRFunc) eglGetProcAddress("eglUnlockSurfaceKHR");

        Q_ASSERT(_eglQueryImageNOK && _eglCreateSharedImageNOK && _eglDestroySharedImageNOK);
        hasImageShared = true;
    }

    if (QEgl::hasExtension("EGL_NOK_surface_scaling")) {
        qDebug("MeegoGraphics: found EGL_NOK_surface_scaling");
        _eglSetSurfaceScalingNOK = (eglSetSurfaceScalingNOKFunc) eglGetProcAddress("eglSetSurfaceScalingNOK");

        Q_ASSERT(_eglSetSurfaceScalingNOK);
        hasSurfaceScaling = true;
    }

    if (QEgl::hasExtension("EGL_KHR_lock_surface2")) {
        qDebug("MeegoGraphics: found EGL_KHR_lock_surface2");
        _eglLockSurfaceKHR = (eglLockSurfaceKHRFunc) eglGetProcAddress("eglLockSurfaceKHR");
        _eglUnlockSurfaceKHR = (eglUnlockSurfaceKHRFunc) eglGetProcAddress("eglUnlockSurfaceKHR");

        Q_ASSERT(_eglLockSurfaceKHR && _eglUnlockSurfaceKHR);
        hasLockSurface = true;
    }

    if (QEgl::hasExtension("EGL_KHR_fence_sync")) {
        qDebug("MeegoGraphics: found EGL_KHR_fence_sync");
        _eglCreateSyncKHR = (eglCreateSyncKHRFunc) eglGetProcAddress("eglCreateSyncKHR");
        _eglDestroySyncKHR = (eglDestroySyncKHRFunc) eglGetProcAddress("eglDestroySyncKHR");
        _eglClientWaitSyncKHR = (eglClientWaitSyncKHRFunc) eglGetProcAddress("eglClientWaitSyncKHR");
        _eglGetSyncAttribKHR = (eglGetSyncAttribKHRFunc) eglGetProcAddress("eglGetSyncAttribKHR");

        Q_ASSERT(_eglCreateSyncKHR && _eglDestroySyncKHR && _eglClientWaitSyncKHR && _eglGetSyncAttribKHR);
        hasFenceSync = true;
    }
}

