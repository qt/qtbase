// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SHELLHELPERS_H
#define SHELLHELPERS_H

#include "hdc.h"

#include <QtCore/qprocess.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

bool isProcessAlive(const Hdc &hdc, const QString &bundleName);
bool setupStdoutLogger(QProcess &stdoutLogger, const Hdc &hdc, const QString &shellStdoutPath);

QT_END_NAMESPACE

#endif // SHELLHELPERS_H
