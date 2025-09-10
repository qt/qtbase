// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxinputcontext_noimf.h"
#include "qqnxabstractvirtualkeyboard.h"
#include "qqnxintegration.h"
#include "qqnxscreen.h"

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtGui/QInputMethodEvent>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaInputMethods, "qt.qpa.input.methods");

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
        qCDebug(lcQpaInputMethods) << "hiding virtual keyboard";
        return false;
    }

    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        m_virtualKeyboard.showKeyboard();
        qCDebug(lcQpaInputMethods) << "requesting virtual keyboard";
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
    qCDebug(lcQpaInputMethods) << Q_FUNC_INFO;
    m_virtualKeyboard.showKeyboard();
}

void QQnxInputContext::hideInputPanel()
{
    qCDebug(lcQpaInputMethods) << Q_FUNC_INFO;
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
    qCDebug(lcQpaInputMethods) << "visible=" << visible;
    if (m_inputPanelVisible != visible) {
        m_inputPanelVisible = visible;
        emitInputPanelVisibleChanged();
    }
}

void QQnxInputContext::keyboardLocaleChanged(const QLocale &locale)
{
    qCDebug(lcQpaInputMethods) << "locale=" << locale;
    if (m_inputPanelLocale != locale) {
        m_inputPanelLocale = locale;
        emitLocaleChanged();
    }
}

void QQnxInputContext::setFocusObject(QObject *object)
{
    qCDebug(lcQpaInputMethods) << "input item=" << object;;

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
