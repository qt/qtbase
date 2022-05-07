/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Gui module
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
