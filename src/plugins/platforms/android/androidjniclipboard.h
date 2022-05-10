// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
