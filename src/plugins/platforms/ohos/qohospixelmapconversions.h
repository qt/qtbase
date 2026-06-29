// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPIXELMAPCONVERSIONS_H
#define QOHOSPIXELMAPCONVERSIONS_H

#include <QtCore/qglobal.h>
#include <QtGui/qimage.h>
#include <optional>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

QImage createQImageFromNativePixelMap(::OH_PixelmapNative *pixelMap);

QNapi::Object makeDisplayDensityScaledJsPixelMapFromQImage(QtOhos::JsState &jsState, const QImage &image);

std::optional<QNapi::Object> createDisplayDensityScaledJsMonochromePixelMapFromIconImage(
    QtOhos::JsState &jsState, const QImage &iconImage, bool isWhiteIcon);

QT_END_NAMESPACE

#endif
