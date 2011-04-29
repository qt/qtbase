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

#include "qwaylandeglwindow.h"

#include "qwaylandscreen.h"
#include "qwaylandglcontext.h"

QWaylandEglWindow::QWaylandEglWindow(QWindow *window)
    : QWaylandWindow(window)
    , mGLContext(0)
    , mWaylandEglWindow(0)
{
    mEglIntegration = static_cast<QWaylandEglIntegration *>(mDisplay->eglIntegration());
    //super creates a new surface
    newSurfaceCreated();
}

QWaylandEglWindow::~QWaylandEglWindow()
{
    delete mGLContext;
}

QWaylandWindow::WindowType QWaylandEglWindow::windowType() const
{
    return QWaylandWindow::Egl;
}

void QWaylandEglWindow::setGeometry(const QRect &rect)
{
    QWaylandWindow::setGeometry(rect);
    if (mWaylandEglWindow) {
        wl_egl_window_resize(mWaylandEglWindow,rect.width(),rect.height(),0,0);
    }
}

void QWaylandEglWindow::setParent(const QPlatformWindow *parent)
{
    const QWaylandWindow *wParent = static_cast<const QWaylandWindow *>(parent);

    mParentWindow = wParent;
}

QPlatformGLContext * QWaylandEglWindow::glContext() const
{
    if (!mGLContext) {
        QWaylandEglWindow *that = const_cast<QWaylandEglWindow *>(this);
        that->mGLContext = new QWaylandGLContext(mEglIntegration->eglDisplay(),widget()->platformWindowFormat());

        EGLNativeWindowType window(reinterpret_cast<EGLNativeWindowType>(mWaylandEglWindow));
        EGLSurface surface = eglCreateWindowSurface(mEglIntegration->eglDisplay(),mGLContext->eglConfig(),window,NULL);
        that->mGLContext->setEglSurface(surface);
    }

    return mGLContext;
}

void QWaylandEglWindow::newSurfaceCreated()
{
    if (mWaylandEglWindow) {
        wl_egl_window_destroy(mWaylandEglWindow);
    }
    wl_visual *visual = QWaylandScreen::waylandScreenFromWidget(widget())->visual();
    QSize size = geometry().size();
    if (!size.isValid())
        size = QSize(0,0);

    mWaylandEglWindow = wl_egl_window_create(mEglIntegration->nativeDisplay(),mSurface,size.width(),size.height(),visual);
    if (mGLContext) {
        EGLNativeWindowType window(reinterpret_cast<EGLNativeWindowType>(mWaylandEglWindow));
        EGLSurface surface = eglCreateWindowSurface(mEglIntegration->eglDisplay(),mGLContext->eglConfig(),window,NULL);
        mGLContext->setEglSurface(surface);
    }
}
