// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHIMETAL_H
#define QRHIMETAL_H

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

#include <private/qrhi_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(MTLDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCommandQueue);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLCommandBuffer);
Q_FORWARD_DECLARE_OBJC_CLASS(MTLRenderCommandEncoder);

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QRhiMetalInitParams : public QRhiInitParams
{
};

struct Q_GUI_EXPORT QRhiMetalNativeHandles : public QRhiNativeHandles
{
    MTLDevice *dev = nullptr;
    MTLCommandQueue *cmdQueue = nullptr;
};

struct Q_GUI_EXPORT QRhiMetalCommandBufferNativeHandles : public QRhiNativeHandles
{
    MTLCommandBuffer *commandBuffer = nullptr;
    MTLRenderCommandEncoder *encoder = nullptr;
};

QT_END_NAMESPACE

#endif
