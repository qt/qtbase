/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidplatformforeignwindow.h"
#include "androidjnimain.h"
#include <QtCore/qvariant.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/private/qjnihelpers_p.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformForeignWindow::QAndroidPlatformForeignWindow(QWindow *window, WId nativeHandle)
    : QAndroidPlatformWindow(window),
      m_surfaceId(-1)
{
    m_view = reinterpret_cast<jobject>(nativeHandle);
    if (m_view.isValid())
        QtAndroid::setViewVisibility(m_view.object(), false);
}

QAndroidPlatformForeignWindow::~QAndroidPlatformForeignWindow()
{
    if (m_view.isValid())
        QtAndroid::setViewVisibility(m_view.object(), false);
    if (m_surfaceId != -1)
        QtAndroid::destroySurface(m_surfaceId);
}

void QAndroidPlatformForeignWindow::lower()
{
    if (m_surfaceId == -1)
        return;

    QAndroidPlatformWindow::lower();
    QtAndroid::bringChildToBack(m_surfaceId);
}

void QAndroidPlatformForeignWindow::raise()
{
    if (m_surfaceId == -1)
        return;

    QAndroidPlatformWindow::raise();
    QtAndroid::bringChildToFront(m_surfaceId);
}

void QAndroidPlatformForeignWindow::setGeometry(const QRect &rect)
{
    QAndroidPlatformWindow::setGeometry(rect);

    if (m_surfaceId != -1)
        QtAndroid::setSurfaceGeometry(m_surfaceId, rect);
}

void QAndroidPlatformForeignWindow::setVisible(bool visible)
{
    if (!m_view.isValid())
        return;

    QtAndroid::setViewVisibility(m_view.object(), visible);

    QAndroidPlatformWindow::setVisible(visible);
    if (!visible && m_surfaceId != -1) {
        QtAndroid::destroySurface(m_surfaceId);
        m_surfaceId = -1;
    } else if (m_surfaceId == -1) {
        m_surfaceId = QtAndroid::insertNativeView(m_view.object(), geometry());
    }
}

void QAndroidPlatformForeignWindow::applicationStateChanged(Qt::ApplicationState state)
{
    if (state <= Qt::ApplicationHidden
            && m_surfaceId != -1) {
        QtAndroid::destroySurface(m_surfaceId);
        m_surfaceId = -1;
    } else if (m_view.isValid() && m_surfaceId == -1){
        m_surfaceId = QtAndroid::insertNativeView(m_view.object(), geometry());
    }

    QAndroidPlatformWindow::applicationStateChanged(state);
}

void QAndroidPlatformForeignWindow::setParent(const QPlatformWindow *window)
{
    Q_UNUSED(window);
}

QT_END_NAMESPACE
