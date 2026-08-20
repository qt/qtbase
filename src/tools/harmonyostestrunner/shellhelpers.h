// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SHELLHELPERS_H
#define SHELLHELPERS_H

#include "hdc.h"

#include <QtCore/qprocess.h>
#include <QtCore/qstring.h>

#include <memory>

QT_BEGIN_NAMESPACE

void forceStopBundle(const Hdc &hdc, const QString &bundleName);
bool isProcessAlive(const Hdc &hdc, const QString &bundleName);
QString readDeviceFile(const Hdc &hdc, const QString &devicePath);
std::unique_ptr<QProcess> streamDeviceFileWhileAppRuns(
    const Hdc &hdc, const QString &devicePath, const QString &bundleName);

QT_END_NAMESPACE

#endif // SHELLHELPERS_H
