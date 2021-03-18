/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#include "qqnxabstractvirtualkeyboard.h"

QT_BEGIN_NAMESPACE

QQnxAbstractVirtualKeyboard::QQnxAbstractVirtualKeyboard(QObject *parent)
    : QObject(parent)
    , m_height(0)
    , m_visible(false)
    , m_locale(QLocale::system())
    , m_keyboardMode(Default)
    , m_enterKeyType(DefaultReturn)
{
}

void QQnxAbstractVirtualKeyboard::setKeyboardMode(KeyboardMode mode)
{
    if (mode == m_keyboardMode)
        return;

    m_keyboardMode = mode;

    if (m_visible)
        applyKeyboardOptions();
}

void QQnxAbstractVirtualKeyboard::setEnterKeyType(EnterKeyType type)
{
    if (type == m_enterKeyType)
        return;

    m_enterKeyType = type;

    if (m_visible)
        applyKeyboardOptions();
}

void QQnxAbstractVirtualKeyboard::setInputHints(int inputHints)
{
    if (inputHints & Qt::ImhEmailCharactersOnly) {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::Email);
    } else if (inputHints & Qt::ImhDialableCharactersOnly) {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::Phone);
    } else if (inputHints & Qt::ImhUrlCharactersOnly) {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::Url);
    } else if (inputHints & Qt::ImhFormattedNumbersOnly || inputHints & Qt::ImhDigitsOnly) {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::Number);
    } else if (inputHints & Qt::ImhDate || inputHints & Qt::ImhTime) {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::NumPunc); // Use NumPunc so that : is available.
    } else if (inputHints & Qt::ImhHiddenText) {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::Password);
    } else {
        setKeyboardMode(QQnxAbstractVirtualKeyboard::Default);
    }
}

void QQnxAbstractVirtualKeyboard::setHeight(int height)
{
    if (height == m_height)
        return;

    const int effectiveHeight = this->height();

    m_height = height;

    if (effectiveHeight != this->height())
        emit heightChanged(this->height());
}

void QQnxAbstractVirtualKeyboard::setVisible(bool visible)
{
    if (visible == m_visible)
        return;

    const int effectiveHeight = height();

    m_visible = visible;

    emit visibilityChanged(visible);

    if (effectiveHeight != height())
        emit heightChanged(height());
}

void QQnxAbstractVirtualKeyboard::setLocale(const QLocale &locale)
{
    if (locale == m_locale)
        return;

    m_locale = locale;

    emit localeChanged(locale);
}

QQnxAbstractVirtualKeyboard::EnterKeyType
    QQnxAbstractVirtualKeyboard::qtEnterKeyTypeToQnx(Qt::EnterKeyType type)
{
    switch (type) {
    case Qt::EnterKeyDone:
        return Done;
    case Qt::EnterKeyGo:
        return Go;
    case Qt::EnterKeyNext:
        return Next;
    case Qt::EnterKeySearch:
        return Search;
    case Qt::EnterKeySend:
        return Send;
    case Qt::EnterKeyDefault:
    case Qt::EnterKeyReturn:
    case Qt::EnterKeyPrevious: // unsupported
        return DefaultReturn;
    }
}

QT_END_NAMESPACE
