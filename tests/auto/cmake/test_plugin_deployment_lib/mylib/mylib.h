// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QList>

#ifdef BUILD_MYLIB
#  define MYLIB_EXPORT Q_DECL_EXPORT
#else
#  define MYLIB_EXPORT Q_DECL_IMPORT
#endif

MYLIB_EXPORT QList<QByteArray> getSupportedImageFormats();
