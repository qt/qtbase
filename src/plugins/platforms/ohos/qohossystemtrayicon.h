// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSSYSTEMTRAYICON_H
#define QOHOSSYSTEMTRAYICON_H

#include <QtGui/qpa/qplatformsystemtrayicon.h>

QT_BEGIN_NAMESPACE

std::unique_ptr<QPlatformSystemTrayIcon> makeQOhosSystemTrayIcon();

QT_END_NAMESPACE

#endif // QOHOSSYSTEMTRAYICON_H
