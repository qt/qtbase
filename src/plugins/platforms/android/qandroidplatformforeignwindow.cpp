// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformforeignwindow.h"
#include "androidjnimain.h"
#include <QtCore/qvariant.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtCore/private/qjnihelpers_p.h>
#include <QtCore/qjnitypes.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformForeignWindow::QAndroidPlatformForeignWindow(QWindow *window, WId nativeHandle)
    : QAndroidPlatformWindow(window), m_view(nullptr), m_nativeViewInserted(false)
{
    m_view = reinterpret_cast<jobject>(nativeHandle);
    if (m_view.isValid())
        QtAndroid::setViewVisibility(m_view.object(), false);
}

QAndroidPlatformForeignWindow::~QAndroidPlatformForeignWindow()
{
    if (m_view.isValid())
        QtAndroid::setViewVisibility(m_view.object(), false);

    m_nativeQtWindow.callMethod<void>("removeNativeView");

}

void QAndroidPlatformForeignWindow::setGeometry(const QRect &rect)
{
    QAndroidPlatformWindow::setGeometry(rect);

    if (m_nativeViewInserted)
        setSurfaceGeometry(rect);
}

void QAndroidPlatformForeignWindow::setVisible(bool visible)
{
    if (!m_view.isValid())
        return;

    QtAndroid::setViewVisibility(m_view.object(), visible);

    QAndroidPlatformWindow::setVisible(visible);
    if (!visible && m_nativeViewInserted) {
        m_nativeQtWindow.callMethod<void>("removeNativeView");
        m_nativeViewInserted = false;
    } else if (!m_nativeViewInserted) {
        addViewToWindow();
    }
}

void QAndroidPlatformForeignWindow::applicationStateChanged(Qt::ApplicationState state)
{
    if (state <= Qt::ApplicationHidden && m_nativeViewInserted) {
        m_nativeQtWindow.callMethod<void>("removeNativeView");
        m_nativeViewInserted = false;
    } else if (m_view.isValid() && !m_nativeViewInserted){
        addViewToWindow();
    }

    QAndroidPlatformWindow::applicationStateChanged(state);
}

void QAndroidPlatformForeignWindow::setParent(const QPlatformWindow *window)
{
    Q_UNUSED(window);
}

void QAndroidPlatformForeignWindow::addViewToWindow()
{
    jint x = 0, y = 0, w = -1, h = -1;
    if (!geometry().isNull())
        geometry().getRect(&x, &y, &w, &h);

    m_nativeQtWindow.callMethod<void>("setNativeView", m_view, x, y, qMax(w, 1), qMax(h, 1));
    m_nativeViewInserted = true;
}

QT_END_NAMESPACE
