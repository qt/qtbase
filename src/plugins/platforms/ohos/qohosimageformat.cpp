// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosimageformat.h"

QT_BEGIN_NAMESPACE

QOhosOptional<::PIXEL_FORMAT> tryMapQtPixelFormatToOhosPixelFormat(QImage::Format format)
{
    static_assert(
        Q_BYTE_ORDER == Q_LITTLE_ENDIAN,
        "Pixel format mapping is currently supported for little-endian targets only");

    switch (format) {
        case QImage::Format_RGBA8888:
            return makeQOhosOptional(::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_8888);
        case QImage::Format_RGB888:
            return makeQOhosOptional(::PIXEL_FORMAT::PIXEL_FORMAT_RGB_888);
        case QImage::Format_Alpha8:
            return makeQOhosOptional(::PIXEL_FORMAT::PIXEL_FORMAT_ALPHA_8);
        case QImage::Format_ARGB32:
            return makeQOhosOptional(::PIXEL_FORMAT::PIXEL_FORMAT_BGRA_8888);
        case QImage::Format_RGBA16FPx4:
            return makeQOhosOptional(::PIXEL_FORMAT::PIXEL_FORMAT_RGBA_F16);
        case QImage::Format_RGB16:
            return makeQOhosOptional(::PIXEL_FORMAT::PIXEL_FORMAT_RGB_565);
        default:
            return {};
    }
}

QT_END_NAMESPACE
