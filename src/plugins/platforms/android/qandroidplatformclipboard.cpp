// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformclipboard.h"
#include "androidjniclipboard.h"
#ifndef QT_NO_CLIPBOARD

QT_BEGIN_NAMESPACE

QAndroidPlatformClipboard::QAndroidPlatformClipboard()
{
    QtAndroidClipboard::setClipboardManager(this);
}

QAndroidPlatformClipboard::~QAndroidPlatformClipboard()
{
    if (data)
        delete data;
}

QMimeData *QAndroidPlatformClipboard::mimeData(QClipboard::Mode mode)
{
    Q_UNUSED(mode);
    Q_ASSERT(supportsMode(mode));
    if (data)
        data->deleteLater();
    data = QtAndroidClipboard::getClipboardMimeData();
    return data;
}

void QAndroidPlatformClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (!data) {
        QtAndroidClipboard::clearClipboardData();
        return;
    }
    if (data && supportsMode(mode))
        QtAndroidClipboard::setClipboardMimeData(data);
    if (data != 0)
        data->deleteLater();
}

bool QAndroidPlatformClipboard::supportsMode(QClipboard::Mode mode) const
{
    return QClipboard::Clipboard == mode;
}

QT_END_NAMESPACE

#endif // QT_NO_CLIPBOARD
