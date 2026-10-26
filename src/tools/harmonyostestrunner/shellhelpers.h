// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SHELLHELPERS_H
#define SHELLHELPERS_H

#include "hdc.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/private/qexpected_p.h>

QT_BEGIN_NAMESPACE

enum class DeviceFileState { Present, Absent, Unknown };
enum class ProcessState { Running, NotRunning, Unknown };

void forceStopBundle(const Hdc &hdc, const QString &bundleName);
ProcessState queryProcessState(const Hdc &hdc, const QString &bundleName);
QString readDeviceFile(const Hdc &hdc, const QString &devicePath);
q23::expected<QByteArray, QString> copyDeviceFile(
        const Hdc &hdc, const QString &devicePath, const QString &localPath);
DeviceFileState queryDeviceFileState(const Hdc &hdc, const QString &devicePath);

QT_END_NAMESPACE

#endif // SHELLHELPERS_H
