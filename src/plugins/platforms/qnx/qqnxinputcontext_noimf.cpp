/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqnxinputcontext_noimf.h"
#include "qqnxabstractvirtualkeyboard.h"

#include <QtCore/QDebug>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QAbstractSpinBox>

QT_BEGIN_NAMESPACE

QQnxInputContext::QQnxInputContext(QQnxAbstractVirtualKeyboard &keyboard) :
    QPlatformInputContext(),
    m_inputPanelVisible(false),
    m_inputPanelLocale(QLocale::c()),
    m_virtualKeyboard(keyboard)
{
    connect(&keyboard, SIGNAL(visibilityChanged(bool)), this, SLOT(keyboardVisibilityChanged(bool)));
    connect(&keyboard, SIGNAL(localeChanged(QLocale)), this, SLOT(keyboardLocaleChanged(QLocale)));
    keyboardVisibilityChanged(keyboard.isVisible());
    keyboardLocaleChanged(keyboard.locale());

    QInputMethod *inputMethod = qApp->inputMethod();
    connect(inputMethod, SIGNAL(inputItemChanged()), this, SLOT(inputItemChanged()));
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
#if defined(QQNXINPUTCONTEXT_DEBUG)
        qDebug() << "QQNX: hiding virtual keyboard";
#endif
        return false;
    }

    if (event->type() == QEvent::RequestSoftwareInputPanel) {
        m_virtualKeyboard.showKeyboard();
#if defined(QQNXINPUTCONTEXT_DEBUG)
        qDebug() << "QQNX: requesting virtual keyboard";
#endif
        return false;
    }

    return false;

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
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
    m_virtualKeyboard.showKeyboard();
}

void QQnxInputContext::hideInputPanel()
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO;
#endif
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

void QQnxInputContext::keyboardVisibilityChanged(bool visible)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "visible=" << visible;
#endif
    if (m_inputPanelVisible != visible) {
        m_inputPanelVisible = visible;
        emitInputPanelVisibleChanged();
    }
}

void QQnxInputContext::keyboardLocaleChanged(const QLocale &locale)
{
#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "locale=" << locale;
#endif
    if (m_inputPanelLocale != locale) {
        m_inputPanelLocale = locale;
        emitLocaleChanged();
    }
}

void QQnxInputContext::inputItemChanged()
{
    QInputMethod *inputMethod = qApp->inputMethod();
    QObject *inputItem = inputMethod->inputItem();

#if defined(QQNXINPUTCONTEXT_DEBUG)
    qDebug() << Q_FUNC_INFO << "input item=" << inputItem;
#endif

    if (!inputItem) {
        if (m_inputPanelVisible)
            hideInputPanel();
    } else {
        if (qobject_cast<QAbstractSpinBox*>(inputItem))
            m_virtualKeyboard.setKeyboardMode(QQnxAbstractVirtualKeyboard::Phone);
        else
            m_virtualKeyboard.setKeyboardMode(QQnxAbstractVirtualKeyboard::Default);

        if (!m_inputPanelVisible)
            showInputPanel();
    }
}

QT_END_NAMESPACE
