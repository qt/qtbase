/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qiosservices.h"

#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>
#include <QtCore/private/qcore_mac_p.h>

#include <QtGui/qdesktopservices.h>

#import <UIKit/UIApplication.h>

QT_BEGIN_NAMESPACE

bool QIOSServices::openUrl(const QUrl &url)
{
    if (qt_apple_isApplicationExtension()) {
        qWarning() << "openUrl not implement for application extensions yet";
        return false;
    }

    if (url == m_handlingUrl)
        return false;

    if (url.scheme().isEmpty())
        return openDocument(url);

    NSURL *nsUrl = url.toNSURL();
    UIApplication *application = qt_apple_sharedApplication();

    if (![application canOpenURL:nsUrl])
        return false;

    static SEL openUrlSelector = @selector(openURL:options:completionHandler:);
    NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:
        [UIApplication instanceMethodSignatureForSelector:openUrlSelector]];
    invocation.target = application;
    invocation.selector = openUrlSelector;

    static auto kEmptyDictionary = @{};
    // Indices 0 and 1 are self and _cmd
    [invocation setArgument:&nsUrl atIndex:2];
    [invocation setArgument:&kEmptyDictionary atIndex:3];
    // Fourth argument is nil, so left unset

    [invocation invoke];

    return true;
}

bool QIOSServices::openDocument(const QUrl &url)
{
    // FIXME: Implement using UIDocumentInteractionController
    return QPlatformServices::openDocument(url);
}

/* Callback from iOS that the application should handle a URL */
bool QIOSServices::handleUrl(const QUrl &url)
{
    QUrl previouslyHandling = m_handlingUrl;
    m_handlingUrl = url;

    // FIXME: Add platform services callback from QDesktopServices::setUrlHandler
    // so that we can warn the user if calling setUrlHandler without also setting
    // up the matching keys in the Info.plist file (CFBundleURLTypes and friends).
    bool couldHandle = QDesktopServices::openUrl(url);

    m_handlingUrl = previouslyHandling;
    return couldHandle;
}

QT_END_NAMESPACE
