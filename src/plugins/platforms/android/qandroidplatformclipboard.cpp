/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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
