// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
