// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDXGIHDRINFO_P_H
#define QDXGIHDRINFO_P_H

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

#include <QtGui/qwindow.h>
#include <rhi/qrhi.h>

#include <dxgi1_6.h>

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QDxgiHdrInfo
{
public:
    // When already having an adapter, i.e. in a D3D11/12 backend of QRhi; this
    // is very cheap to construct.
    QDxgiHdrInfo(IDXGIAdapter1 *adapter);

    // When having the adapter LUID from somewhere (e.g.
    // VkPhysicalDeviceIDProperties::deviceLUID); This has to create a DXGI
    // factory and enumarate to find the matching IDXGIAdapter, so less cheap.
    QDxgiHdrInfo(LUID luid);

    // All functions below will enumerate all adapters and all their outputs
    // every time they are called. Probably not nice. Use when there is no
    // knowledge which GPUs will be used with QRhi.
    QDxgiHdrInfo();

    ~QDxgiHdrInfo();

    // True if the window belongs to an output where HDR is enabled. Moving a
    // window to another screen may make this function return a different value.
    //
    // If/how DXGI outputs map to screens, is out of our hands, but they will
    // typically match. However, this will not be functional when WARP is used
    // since the WARP adapter never has any outputs associated (even though
    // there are certainly screens still).
    //
    bool isHdrCapable(QWindow *w);

    // HDR info and white level. Or default, dummy values when the window is not
    // on a HDR-enabled output.
    QRhiSwapChainHdrInfo queryHdrInfo(QWindow *w);

private:
    static bool output6ForWindow(QWindow *w, IDXGIAdapter1 *adapter, IDXGIOutput6 **result);
    static bool outputDesc1ForWindow(QWindow *w, IDXGIAdapter1 *adapter, DXGI_OUTPUT_DESC1 *result);
    static float sdrWhiteLevelInNits(const DXGI_OUTPUT_DESC1 &outputDesc);

    IDXGIFactory2 *m_factory = nullptr;
    IDXGIAdapter1 *m_adapter = nullptr;
};

QT_END_NAMESPACE

#endif
