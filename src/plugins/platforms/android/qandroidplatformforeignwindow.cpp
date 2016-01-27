/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

QAndroidPlatformForeignWindow::QAndroidPlatformForeignWindow(QWindow *window)
    : QAndroidPlatformWindow(window),
      m_surfaceId(-1)
{
    const WId wId = window->property("_q_foreignWinId").value<WId>();
    m_view = reinterpret_cast<jobject>(wId);
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
