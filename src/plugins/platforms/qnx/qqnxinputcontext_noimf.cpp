/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqnxinputcontext_noimf.h"
#include "qqnxabstractvirtualkeyboard.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethodEvent>

#if defined(QQNXINPUTCONTEXT_DEBUG)
#define qInputContextDebug qDebug
#else
#define qInputContextDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxInputContext::QQnxInputContext(QQnxIntegration *integration, QQnxAbstractVirtualKeyboard &keyboard) :
    QPlatformInputContext(),
    m_inputPanelVisible(false),
    m_inputPanelLocale(QLocale::c()),
    m_integration(integration),
    m_virtualKeyboard(keyboard)
{
    connect(&keyboard, SIGNAL(heightChanged(int)), this, SLOT(keyboardHeightChanged()));
    connect(&keyboard, SIGNAL(visibilityChanged(bool)), this, SLOT(keyboardVisibilityChanged(bool)));
    connect(&keyboard, SIGNAL(localeChanged(QLocale)), this, SLOT(keyboardLocaleChanged(QLocale)));
    keyboardVisibilityChanged(keyboard.isVisible());
    keyboardLocaleChanged(keyboard.locale());
}

QQnxInputContext::~QQnxInputContext()
{
}

bool QQnxInputContext::isValid() const
{
    return true;
}

bool QQnxInputContext::hasPhysicalKeyboard()
{
    // TODO: This should query the system to check if a USB keyboard is connected.
    return false;
}

void QQnxInputContext::reset()
{
}

bool QQnxInputContext::filterEvent( const QEvent *event )
{
    if (hasPhysicalKeyboard())
        return false;

    if (event->type() == QEvent::CloseSoftwareInputPanel) {
        m_virtualKeyboard.hideKeyboard();
        qInputContextDebug("hiding virtual keyboard");
        return false;
    }

    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        m_virtualKeyboard.showKeyboard();
        qInputContextDebug("requesting virtual keyboard");
        return false;
    }

    return false;

}

QRectF QQnxInputContext::keyboardRect() const
{
    QRect screenGeometry = m_integration->primaryDisplay()->geometry();
    return QRectF(screenGeometry.x(), screenGeometry.height() - m_virtualKeyboard.height(),
                  screenGeometry.width(), m_virtualKeyboard.height());
}

bool QQnxInputContext::handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap)
{
    Q_UNUSED(flags);
    Q_UNUSED(sym);
    Q_UNUSED(mod);
    Q_UNUSED(scan);
    Q_UNUSED(cap);
    return false;
}

void QQnxInputContext::showInputPanel()
{
    qInputContextDebug();
    m_virtualKeyboard.showKeyboard();
}

void QQnxInputContext::hideInputPanel()
{
    qInputContextDebug();
    m_virtualKeyboard.hideKeyboard();
}

bool QQnxInputContext::isInputPanelVisible() const
{
    return m_inputPanelVisible;
}

QLocale QQnxInputContext::locale() const
{
    return m_inputPanelLocale;
}

void QQnxInputContext::keyboardHeightChanged()
{
    emitKeyboardRectChanged();
}

void QQnxInputContext::keyboardVisibilityChanged(bool visible)
{
    qInputContextDebug() << "visible=" << visible;
    if (m_inputPanelVisible != visible) {
        m_inputPanelVisible = visible;
        emitInputPanelVisibleChanged();
    }
}

void QQnxInputContext::keyboardLocaleChanged(const QLocale &locale)
{
    qInputContextDebug() << "locale=" << locale;
    if (m_inputPanelLocale != locale) {
        m_inputPanelLocale = locale;
        emitLocaleChanged();
    }
}

void QQnxInputContext::setFocusObject(QObject *object)
{
    qInputContextDebug() << "input item=" << object;

    if (!inputMethodAccepted()) {
        if (m_inputPanelVisible)
            hideInputPanel();
    } else {
        QInputMethodQueryEvent query(Qt::ImHints | Qt::ImEnterKeyType);
        QCoreApplication::sendEvent(object, &query);
        int inputHints = query.value(Qt::ImHints).toInt();
        Qt::EnterKeyType qtEnterKeyType = Qt::EnterKeyType(query.value(Qt::ImEnterKeyType).toInt());

        m_virtualKeyboard.setInputHints(inputHints);
        m_virtualKeyboard.setEnterKeyType(
            QQnxAbstractVirtualKeyboard::qtEnterKeyTypeToQnx(qtEnterKeyType)
        );

        if (!m_inputPanelVisible)
            showInputPanel();
    }
}

QT_END_NAMESPACE
