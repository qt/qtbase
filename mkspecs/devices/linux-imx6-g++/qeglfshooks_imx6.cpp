/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qeglfshooks.h"
#include <EGL/eglvivante.h>

QT_BEGIN_NAMESPACE

class QEglFSImx6Hooks : public QEglFSHooks
{
public:
    QEglFSImx6Hooks();
    virtual QSize screenSize() const;
    virtual EGLNativeWindowType createNativeWindow(const QSize &size, const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);

private:
    QSize mScreenSize;
    EGLNativeDisplayType mNativeDisplay;
};


QEglFSImx6Hooks::QEglFSImx6Hooks()
{
    int width, height;
    mNativeDisplay = fbGetDisplayByIndex(0);
    fbGetDisplayGeometry(mNativeDisplay, &width, &height);
    mScreenSize.setHeight(height);
    mScreenSize.setWidth(width);
}

QSize QEglFSImx6Hooks::screenSize() const
{
    return mScreenSize;
}

EGLNativeWindowType QEglFSImx6Hooks::createNativeWindow(const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(format);

    EGLNativeWindowType eglWindow = fbCreateWindow(mNativeDisplay, 0, 0, size.width(), size.height());
    return eglWindow;
}


void QEglFSImx6Hooks::destroyNativeWindow(EGLNativeWindowType window)
{
    fbDestroyWindow(window);
}

QEglFSImx6Hooks eglFSImx6Hooks;
QEglFSHooks *platformHooks = &eglFSImx6Hooks;

QT_END_NAMESPACE
