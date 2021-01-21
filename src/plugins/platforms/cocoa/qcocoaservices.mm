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

#include "qcocoaservices.h"

#include <AppKit/NSWorkspace.h>
#include <Foundation/NSURL.h>

#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

bool QCocoaServices::openUrl(const QUrl &url)
{
    const QString scheme = url.scheme();
    if (scheme.isEmpty())
        return openDocument(url);
    return [[NSWorkspace sharedWorkspace] openURL:url.toNSURL()];
}

bool QCocoaServices::openDocument(const QUrl &url)
{
    if (!url.isValid())
        return false;

    return [[NSWorkspace sharedWorkspace] openFile:url.toLocalFile().toNSString()];
}

QT_END_NAMESPACE
