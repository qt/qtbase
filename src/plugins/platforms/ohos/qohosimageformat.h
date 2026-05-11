// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSIMAGEFORMAT_H
#define QOHOSIMAGEFORMAT_H

#include <QtCore/private/qohoscommon_p.h>
#include <QtGui/qimage.h>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

QOhosOptional<::PIXEL_FORMAT> tryMapQtPixelFormatToOhosPixelFormat(QImage::Format format);

QT_END_NAMESPACE

#endif
