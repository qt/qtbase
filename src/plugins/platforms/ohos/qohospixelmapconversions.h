// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPIXELMAPCONVERSIONS_H
#define QOHOSPIXELMAPCONVERSIONS_H

#include <QtCore/private/qnapi_p.h>
#include <QtCore/qglobal.h>
#include <QtGui/qimage.h>
#include <memory>
#include <multimedia/image_framework/image/pixelmap_native.h>
#include <qohosimageformat.h>
#include <qohosplugincore.h>

QT_BEGIN_NAMESPACE

std::shared_ptr<::OH_PixelmapNative> createNativePixelMapFromQImage(QImage qImage);

QImage createQImageFromNativePixelMap(::OH_PixelmapNative *pixelMap);

std::shared_ptr<::OH_PixelmapNative> makeEmptyNativePixelMap();

std::shared_ptr<::OH_PixelmapNative> wrapNativePixelMapPtr(::OH_PixelmapNative *pixelMap);

QNapi::Object createNapiPixelMapFromQImage(QOhosJsState &jsState, const QImage &image);

QT_END_NAMESPACE

#endif
