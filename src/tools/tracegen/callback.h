// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CALLBACK_H
#define CALLBACK_H

#include "provider.h"

#include <qfile.h>

void writeCallback(QFile &file, const Provider &provider);

#endif // CALLBACK_H
