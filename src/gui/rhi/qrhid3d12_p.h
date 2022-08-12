// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHID3D12_H
#define QRHID3D12_H

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

// no d3d includes here, to prevent precompiled header mess due to COM

QT_BEGIN_NAMESPACE

struct Q_GUI_EXPORT QRhiD3D12InitParams : public QRhiInitParams
{
    bool enableDebugLayer = false;
};

struct Q_GUI_EXPORT QRhiD3D12NativeHandles : public QRhiNativeHandles
{
    // to import a device
    void *dev = nullptr;
    int minimumFeatureLevel = 0;
    // to just specify the adapter to use, set these and leave dev set to null
    quint32 adapterLuidLow = 0;
    qint32 adapterLuidHigh = 0;
    // in addition, can specify the command queue to use
    void *commandQueue = nullptr;
};

struct Q_GUI_EXPORT QRhiD3D12CommandBufferNativeHandles : public QRhiNativeHandles
{
    void *commandList = nullptr; // ID3D12GraphicsCommandList
};

QT_END_NAMESPACE

#endif
