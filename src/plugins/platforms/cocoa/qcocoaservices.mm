// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcocoaservices.h"

#include <AppKit/NSWorkspace.h>
#include <Foundation/NSURL.h>

#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE

bool QCocoaServices::openUrl(const QUrl &url)
{
    return [[NSWorkspace sharedWorkspace] openURL:url.toNSURL()];
}

bool QCocoaServices::openDocument(const QUrl &url)
{
    return openUrl(url);
}

QT_END_NAMESPACE
