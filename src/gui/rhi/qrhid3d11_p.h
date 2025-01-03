// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRHID3D11_H
#define QRHID3D11_H

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

struct Q_GUI_EXPORT QRhiD3D11InitParams : public QRhiInitParams
{
    bool enableDebugLayer = false;

    int framesUntilKillingDeviceViaTdr = -1;
    bool repeatDeviceKill = false;
};

struct Q_GUI_EXPORT QRhiD3D11NativeHandles : public QRhiNativeHandles
{
    // to import a device and a context
    void *dev = nullptr;
    void *context = nullptr;
    // alternatively, to specify the device feature level and/or the adapter to use
    int featureLevel = 0;
    quint32 adapterLuidLow = 0;
    qint32 adapterLuidHigh = 0;
};

QT_END_NAMESPACE

#endif
