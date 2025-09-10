// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdirect2dnativeinterface.h"

#include <QtGui/qbackingstore.h>

QT_BEGIN_NAMESPACE

void *QWindowsDirect2DNativeInterface::nativeResourceForBackingStore(const QByteArray &resource, QBackingStore *bs)
{
    if (!bs || !bs->handle()) {
        qWarning("%s: '%s' requested for null backingstore or backingstore without handle.", __FUNCTION__, resource.constData());
        return nullptr;
    }

    // getDC is so common we don't want to print an "invalid key" line for it
    if (resource == "getDC")
        return nullptr;

    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return nullptr;

}

QT_END_NAMESPACE
