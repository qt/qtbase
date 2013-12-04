/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsdirect2dbackingstore.h"
#include "qwindowsdirect2dintegration.h"
#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dpaintdevice.h"
#include "qwindowsdirect2dbitmap.h"
#include "qwindowsdirect2ddevicecontext.h"

#include "qwindowswindow.h"
#include "qwindowscontext.h"

#include <QtGui/QWindow>
#include <QtCore/QDebug>

#include <dxgi1_2.h>
#include <d3d11.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DBackingStorePrivate
{
public:
    QWindowsDirect2DBackingStorePrivate() {}

    ComPtr<IDXGISwapChain1> swapChain;
    QSharedPointer<QWindowsDirect2DBitmap> backingStore;
    QScopedPointer<QWindowsDirect2DPaintDevice> nativePaintDevice;

    bool init(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC1 desc = {};

        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = 1;
        desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

        HRESULT hr = QWindowsDirect2DContext::instance()->dxgiFactory()->CreateSwapChainForHwnd(
                    QWindowsDirect2DContext::instance()->d3dDevice(), // [in]   IUnknown *pDevice
                    hwnd,                                             // [in]   HWND hWnd
                    &desc,                                            // [in]   const DXGI_SWAP_CHAIN_DESC1 *pDesc
                    NULL,                                             // [in]   const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc
                    NULL,                                             // [in]   IDXGIOutput *pRestrictToOutput
                    swapChain.ReleaseAndGetAddressOf());              // [out]  IDXGISwapChain1 **ppSwapChain

        if (FAILED(hr))
            qWarning("%s: Could not create swap chain: %#x", __FUNCTION__, hr);

        return SUCCEEDED(hr);
    }

    bool setupPaintDevice()
    {
        if (!backingStore) {
            ComPtr<ID2D1DeviceContext> deviceContext;
            ComPtr<IDXGISurface1> backBufferSurface;
            ComPtr<ID2D1Bitmap1> backBufferBitmap;

            HRESULT hr = QWindowsDirect2DContext::instance()->d2dDevice()->CreateDeviceContext(
                        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                        deviceContext.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                qWarning("%s: Couldn't create Direct2D Device context: %#x", __FUNCTION__, hr);
                return false;
            }

            hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferSurface));
            if (FAILED(hr)) {
                qWarning("%s: Could not query backbuffer for DXGI Surface: %#x", __FUNCTION__, hr);
                return false;
            }

            hr = deviceContext->CreateBitmapFromDxgiSurface(backBufferSurface.Get(), NULL, backBufferBitmap.ReleaseAndGetAddressOf());
            if (FAILED(hr)) {
                qWarning("%s: Could not create Direct2D Bitmap from DXGI Surface: %#x", __FUNCTION__, hr);
                return false;
            }

            backingStore.reset(new QWindowsDirect2DBitmap(backBufferBitmap.Get(), deviceContext.Get()));
        }

        if (!nativePaintDevice)
            nativePaintDevice.reset(new QWindowsDirect2DPaintDevice(backingStore.data(), QInternal::Widget));

        return true;
    }

    void releaseBackingStore()
    {
        nativePaintDevice.reset();
        backingStore.reset();
    }

    QPaintDevice *paintDevice()
    {
        setupPaintDevice();
        return nativePaintDevice.data();
    }

    void flush()
    {
        swapChain->Present(1, 0);
    }
};

/*!
    \class QWindowsDirect2DBackingStore
    \brief Backing store for windows.
    \internal
    \ingroup qt-lighthouse-win
*/

QWindowsDirect2DBackingStore::QWindowsDirect2DBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , d_ptr(new QWindowsDirect2DBackingStorePrivate)
{
}

bool QWindowsDirect2DBackingStore::init()
{
    if (window()->surfaceType() == QSurface::RasterSurface) {
        Q_D(QWindowsDirect2DBackingStore);
        return d->init(windowsWindow()->handle());
    }

    return true;
}

QWindowsDirect2DBackingStore *QWindowsDirect2DBackingStore::create(QWindow *window)
{
    QWindowsDirect2DBackingStore *result = new QWindowsDirect2DBackingStore(window);

    if (!result->init()) {
        delete result;
        result = 0;
    }

    return result;
}

QWindowsDirect2DBackingStore::~QWindowsDirect2DBackingStore()
{
    Q_D(QWindowsDirect2DBackingStore);
    d->releaseBackingStore();
    d->swapChain.Reset();
}

QPaintDevice *QWindowsDirect2DBackingStore::paintDevice()
{
    QPaintDevice *result = 0;

    if (window()->surfaceType() == QSurface::RasterSurface) {
        Q_D(QWindowsDirect2DBackingStore);
        result = d->paintDevice();
    }

    return result;
}

void QWindowsDirect2DBackingStore::flush(QWindow *, const QRegion &, const QPoint &)
{
    if (window()->surfaceType() == QSurface::RasterSurface) {
        Q_D(QWindowsDirect2DBackingStore);
        d->flush();
    }
}

void QWindowsDirect2DBackingStore::resize(const QSize &size, const QRegion &region)
{
    Q_UNUSED(region);

    if (window()->surfaceType() != QSurface::RasterSurface)
        return;

    Q_D(QWindowsDirect2DBackingStore);
    d->releaseBackingStore();
    QWindowsDirect2DContext::instance()->d3dDeviceContext()->ClearState();

    HRESULT hr = d->swapChain->ResizeBuffers(0,
                                             size.width(), size.height(),
                                             DXGI_FORMAT_UNKNOWN,
                                             0);
    if (FAILED(hr))
        qWarning("%s: Could not resize buffers: %#x", __FUNCTION__, hr);
}

QWindowsWindow *QWindowsDirect2DBackingStore::windowsWindow() const
{
    if (const QWindow *w = window())
        if (QPlatformWindow *pw = w->handle())
            return static_cast<QWindowsWindow *>(pw);
    return 0;
}

QT_END_NAMESPACE
