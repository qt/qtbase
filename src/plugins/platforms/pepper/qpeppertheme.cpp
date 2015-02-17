/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Native Client platform plugin.
**
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
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
        return QStringList(QStringLiteral("cleanlooks"));
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
