// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "nativewindowdump.h"

#include <QtGui/QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <QtCore/QDebug>

namespace QtDiag {

void dumpNativeWindows(WId wid)
{
    QPlatformNativeInterface *ni = QGuiApplication::platformNativeInterface();
    QString result;
    QMetaObject::invokeMethod(ni, "dumpNativeWindows", Qt::DirectConnection,
                              Q_RETURN_ARG(QString, result),
                              Q_ARG(WId, wid));
    qDebug().noquote() << result;
}

void dumpNativeQtTopLevels()
{
}

} // namespace QtDiag
