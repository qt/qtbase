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

#ifndef QQNXABSTRACTVIRTUALKEYBOARD_H
#define QQNXABSTRACTVIRTUALKEYBOARD_H

#include <QLocale>
#include <QObject>

QT_BEGIN_NAMESPACE

class QQnxAbstractVirtualKeyboard : public QObject
{
    Q_OBJECT
public:
    // Keyboard Types currently supported.
    // Default - Regular Keyboard
    // Url/Email - Enhanced keys for each types.
    // Web - Regular keyboard with two blank keys, currently unused.
    // NumPunc - Numbers & Punctionation, alternate to Symbol
    // Number - Number pad
    // Symbol - All symbols, alternate to NumPunc, currently unused.
    // Phone - Phone enhanced keyboard
    // Pin - Keyboard for entering Pins (Hex values).
    // Password - Keyboard with lots of extra characters for password input.
    // Alphanumeric - Similar to password without any of the security implications.
    //
    enum KeyboardMode { Default, Url, Email, Web, NumPunc, Number, Symbol, Phone, Pin, Password, Alphanumeric };
    enum EnterKeyType { DefaultReturn, Connect, Done, Go, Join, Next, Search, Send, Submit };

    explicit QQnxAbstractVirtualKeyboard(QObject *parent = 0);

    virtual bool showKeyboard() = 0;
    virtual bool hideKeyboard() = 0;

    int  height() { return m_visible ? m_height : 0; }
    bool isVisible() const { return m_visible; }
    QLocale locale() const { return m_locale; }

    void setKeyboardMode(KeyboardMode mode);
    void setEnterKeyType(EnterKeyType type);

    void setInputHints(int inputHints);
    KeyboardMode keyboardMode() const { return m_keyboardMode; }
    EnterKeyType enterKeyType() const { return m_enterKeyType; }

    static EnterKeyType qtEnterKeyTypeToQnx(Qt::EnterKeyType type);

Q_SIGNALS:
    void heightChanged(int height);
    void visibilityChanged(bool visible);
    void localeChanged(const QLocale &locale);

protected:
    virtual void applyKeyboardOptions() = 0;

    void setHeight(int height);
    void setVisible(bool visible);
    void setLocale(const QLocale &locale);

private:
    int m_height;
    bool m_visible;
    QLocale m_locale;
    KeyboardMode m_keyboardMode;
    EnterKeyType m_enterKeyType;
};

QT_END_NAMESPACE

#endif // QQNXABSTRACTVIRTUALKEYBOARD_H
