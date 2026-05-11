// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMBACKINGSTOREGL_H
#define QOHOSPLATFORMBACKINGSTOREGL_H

#include <QtCore/qglobal.h>
#include <memory>
#include <qpa/qplatformbackingstore.h>

QT_BEGIN_NAMESPACE

std::unique_ptr<QPlatformBackingStore> makeGlOhosPlatformBackingStore(QWindow *window);

QT_END_NAMESPACE

#endif
