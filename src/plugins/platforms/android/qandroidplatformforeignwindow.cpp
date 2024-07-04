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
    : QAndroidPlatformWindow(window)
    , m_view(reinterpret_cast<jobject>(nativeHandle))
    , m_nativeViewInserted(false)
{
}

void QAndroidPlatformForeignWindow::initialize()
{
    QAndroidPlatformWindow::initialize();

    if (isEmbeddingContainer()) {
        m_nativeViewId = m_view.callMethod<jint>("getId");
        return;
    }

    if (m_view.isValid())
        QtAndroid::setViewVisibility(m_view.object(), false);
}

QAndroidPlatformForeignWindow::~QAndroidPlatformForeignWindow()
{
    if (isEmbeddingContainer())
        return;

    if (m_view.isValid())
        QtAndroid::setViewVisibility(m_view.object(), false);

    m_nativeQtWindow.callMethod<void>("removeNativeView");

}

void QAndroidPlatformForeignWindow::setVisible(bool visible)
{
    if (isEmbeddingContainer()) {
        QAndroidPlatformWindow::setVisible(visible);
        return;
    }

    if (!m_view.isValid())
        return;

    QtAndroid::setViewVisibility(m_view.object(), visible);
    m_nativeQtWindow.callMethod<void>("setVisible", visible);

    if (!visible && m_nativeViewInserted) {
        m_nativeQtWindow.callMethod<void>("removeNativeView");
        m_nativeViewInserted = false;
    } else if (!m_nativeViewInserted) {
        addViewToWindow();
    }
}

void QAndroidPlatformForeignWindow::applicationStateChanged(Qt::ApplicationState state)
{
    if (!isEmbeddingContainer()) {
        if (state <= Qt::ApplicationHidden
                && m_nativeViewInserted) {
            m_nativeQtWindow.callMethod<void>("removeNativeView");
            m_nativeViewInserted = false;
        } else if (m_view.isValid() && !m_nativeViewInserted){
            addViewToWindow();
        }
    }

    QAndroidPlatformWindow::applicationStateChanged(state);
}

WId QAndroidPlatformForeignWindow::winId() const
{
    if (isEmbeddingContainer() && m_view.isValid())
        return reinterpret_cast<WId>(m_view.object());
    if (m_nativeQtWindow.isValid())
        return reinterpret_cast<WId>(m_nativeQtWindow.object());
    return 0L;
}

void QAndroidPlatformForeignWindow::addViewToWindow()
{
    if (isEmbeddingContainer())
        return;

    m_nativeQtWindow.callMethod<void>("setNativeView", m_view);
    m_nativeViewInserted = true;
}

QT_END_NAMESPACE
