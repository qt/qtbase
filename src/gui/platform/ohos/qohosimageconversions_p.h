// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSIMAGECONVERSIONS_P_H
#define QOHOSIMAGECONVERSIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtGui/qimage.h>
#include <memory>
#include <multimedia/image_framework/image/pixelmap_native.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT std::shared_ptr<::OH_PixelmapNative> makeOhosNativePixelMapFromQImage(QImage qImage);

Q_GUI_EXPORT std::shared_ptr<::OH_PixelmapNative> makeEmptyOhosNativePixelMap();

Q_GUI_EXPORT std::shared_ptr<::OH_PixelmapNative> wrapOhosNativePixelMapPtr(::OH_PixelmapNative *pixelMap);

Q_GUI_EXPORT QNapi::Object makeOhosNapiPixelMapFromQImage(QOhosJsState &jsState, const QImage &image);

QT_END_NAMESPACE

#endif
