/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpeppertheme.h"

#include "qpepperinstance_p.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

QT_BEGIN_NAMESPACE

QPepperTheme::QPepperTheme()
    : m_keyboardScheme(QPlatformTheme::X11KeyboardScheme)
{
    // Look at navigator.appVersion to get the host OS.
    // ## These calls to javascript are async, which means m_keyboardScheme
    // may not be set correctly during startup before handleGetAppVersionMessage
    // is called.

    QPepperInstancePrivate *instance = QPepperInstancePrivate::get();
    instance->registerMessageHandler("qtGetAppVersion", this, "handleGetAppVersionMessage");
    const char *getAppVersionsScript
        = "this.qtMessageHandlers[\"qtGetAppVersion\"] = function(url) { "
          "    embed.postMessage(\"qtGetAppVersion: \"  + navigator.appVersion);"
          "}";
    instance->runJavascript(getAppVersionsScript);
    instance->postMessage("qtGetAppVersion: ");
}

QPepperTheme::~QPepperTheme() {}

QVariant QPepperTheme::themeHint(ThemeHint hint) const
{
    switch (hint) {
    case QPlatformTheme::StyleNames:
        return QStringList(QStringLiteral("fusion"));
    case QPlatformTheme::KeyboardScheme:
        return m_keyboardScheme;
    default:
        break;
    }
    return QPlatformTheme::themeHint(hint);
}

void QPepperTheme::handleGetAppVersionMessage(const QByteArray &message)
{
    if (message.contains("OS X"))
        m_keyboardScheme = QPlatformTheme::MacKeyboardScheme;
    else if (message.contains("Win"))
        m_keyboardScheme = QPlatformTheme::WindowsKeyboardScheme;
    else
        m_keyboardScheme = QPlatformTheme::X11KeyboardScheme;
}

QT_END_NAMESPACE
