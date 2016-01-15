/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qiosservices.h"

#include <QtCore/qurl.h>
#include <QtGui/qdesktopservices.h>

#import <UIKit/UIApplication.h>

QT_BEGIN_NAMESPACE

bool QIOSServices::openUrl(const QUrl &url)
{
    if (url == m_handlingUrl)
        return false;

    if (url.scheme().isEmpty())
        return openDocument(url);

    NSURL *nsUrl = url.toNSURL();

    if (![[UIApplication sharedApplication] canOpenURL:nsUrl])
        return false;

    return [[UIApplication sharedApplication] openURL:nsUrl];
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
