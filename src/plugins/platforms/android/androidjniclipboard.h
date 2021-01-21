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

#ifndef ANDROIDJNICLIPBOARD_H
#define ANDROIDJNICLIPBOARD_H

#include <QString>
#include "qandroidplatformclipboard.h"
#include "androidjnimain.h"

QT_BEGIN_NAMESPACE

class QAndroidPlatformClipboard;
namespace QtAndroidClipboard
{
    // Clipboard support
    void setClipboardManager(QAndroidPlatformClipboard *manager);
    void setClipboardMimeData(QMimeData *data);
    QMimeData *getClipboardMimeData();
    void clearClipboardData();
    void onClipboardDataChanged(JNIEnv */*env*/, jobject /*thiz*/);
    // Clipboard support
}

QT_END_NAMESPACE

#endif // ANDROIDJNICLIPBOARD_H
