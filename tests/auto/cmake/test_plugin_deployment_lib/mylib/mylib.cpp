// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include "mylib.h"
#include <QtGui/QImageReader>

QList<QByteArray> getSupportedImageFormats()
{
    return QImageReader::supportedImageFormats();
}
