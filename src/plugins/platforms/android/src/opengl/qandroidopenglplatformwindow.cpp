/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qandroidopenglplatformwindow.h"
#include "androidjnimain.h"
#include <qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

EGLSurface QAndroidOpenGLPlatformWindow::m_staticSurface = 0;
EGLNativeWindowType QAndroidOpenGLPlatformWindow::m_staticNativeWindow = 0;
QReadWriteLock QAndroidOpenGLPlatformWindow::m_staticSurfaceLock;
QBasicAtomicInt QAndroidOpenGLPlatformWindow::m_referenceCount = Q_BASIC_ATOMIC_INITIALIZER(0);

QAndroidOpenGLPlatformWindow::QAndroidOpenGLPlatformWindow(QWindow *window)
    : QEglFSWindow(window)
{
}

QAndroidOpenGLPlatformWindow::~QAndroidOpenGLPlatformWindow()
{
    destroy();
}

bool QAndroidOpenGLPlatformWindow::isExposed() const
{
    return QtAndroid::nativeWindow(false) != 0 && QEglFSWindow::isExposed();
}

void QAndroidOpenGLPlatformWindow::invalidateSurface()
{
    QWindowSystemInterface::handleExposeEvent(window(), QRegion()); // Obscure event
    QWindowSystemInterface::flushWindowSystemEvents();
    QEglFSWindow::invalidateSurface();

    m_window = 0;
    m_surface = 0;

    if (!m_referenceCount.deref()){
        QWriteLocker locker(&m_staticSurfaceLock);

        EGLDisplay display = (static_cast<QEglFSScreen *>(window()->screen()->handle()))->display();
        eglDestroySurface(display, m_staticSurface);

        m_staticSurface = 0;
        m_staticNativeWindow = 0;
    }
}

void QAndroidOpenGLPlatformWindow::updateStaticNativeWindow()
{
    QWriteLocker locker(&m_staticSurfaceLock);
    m_staticNativeWindow = QtAndroid::nativeWindow(false);
}

void QAndroidOpenGLPlatformWindow::resetSurface()
{
    // Only add a reference if we're not already holding one, otherwise we're just updating
    // the native window pointer
    if (m_window == 0)
        m_referenceCount.ref();

    if (m_staticSurface == 0) {
        QWriteLocker locker(&m_staticSurfaceLock);
        QEglFSWindow::resetSurface();
        m_staticSurface = m_surface;
        m_staticNativeWindow = m_window;
    } else {
        QReadLocker locker(&m_staticSurfaceLock);
        m_window = m_staticNativeWindow;
        m_surface = m_staticSurface;
    }

    {
        lock();
        scheduleResize(QtAndroid::nativeWindowSize());
        QWindowSystemInterface::handleExposeEvent(window(), QRegion(geometry())); // Expose event
        unlock();
    }

    QWindowSystemInterface::flushWindowSystemEvents();
}

void QAndroidOpenGLPlatformWindow::destroy()
{
    if (!m_referenceCount.deref()) {
        QEglFSWindow::destroy();
    } else {
        m_window = 0;
        m_surface = 0;
    }
}

void QAndroidOpenGLPlatformWindow::raise()
{
}

void QAndroidOpenGLPlatformWindow::setVisible(bool visible)
{
    QEglFSWindow::setVisible(visible);
    QWindowSystemInterface::handleExposeEvent(window(), QRegion(geometry())); // Expose event
    QWindowSystemInterface::flushWindowSystemEvents();
}

QT_END_NAMESPACE
