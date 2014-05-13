/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dwindow.h"
#include "qwindowsdirect2ddevicecontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2dplatformpixmap.h"

#include <d3d11.h>
#include <d2d1_1.h>
using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

QWindowsDirect2DWindow::QWindowsDirect2DWindow(QWindow *window, const QWindowsWindowData &data)
    : QWindowsWindow(window, data)
    , m_needsFullFlush(true)
{
    if (window->type() == Qt::Desktop)
        return; // No further handling for Qt::Desktop

    DXGI_SWAP_CHAIN_DESC1 desc = {};

    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 1;
    desc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

    HRESULT hr = QWindowsDirect2DContext::instance()->dxgiFactory()->CreateSwapChainForHwnd(
                QWindowsDirect2DContext::instance()->d3dDevice(), // [in]   IUnknown *pDevice
                handle(),                                         // [in]   HWND hWnd
                &desc,                                            // [in]   const DXGI_SWAP_CHAIN_DESC1 *pDesc
                NULL,                                             // [in]   const DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullscreenDesc
                NULL,                                             // [in]   IDXGIOutput *pRestrictToOutput
                m_swapChain.ReleaseAndGetAddressOf());            // [out]  IDXGISwapChain1 **ppSwapChain

    if (FAILED(hr))
        qWarning("%s: Could not create swap chain: %#x", __FUNCTION__, hr);

    hr = QWindowsDirect2DContext::instance()->d2dDevice()->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                m_deviceContext.GetAddressOf());
    if (FAILED(hr))
        qWarning("%s: Couldn't create Direct2D Device context: %#x", __FUNCTION__, hr);
}

QWindowsDirect2DWindow::~QWindowsDirect2DWindow()
{
}

QPixmap *QWindowsDirect2DWindow::pixmap()
{
    setupBitmap();

    return m_pixmap.data();
}

void QWindowsDirect2DWindow::flush(QWindowsDirect2DBitmap *bitmap, const QRegion &region, const QPoint &offset)
{
    DXGI_SWAP_CHAIN_DESC1 desc;
    HRESULT hr = m_swapChain->GetDesc1(&desc);
    QRect geom = geometry();

    if (FAILED(hr) || (desc.Width != geom.width()) || (desc.Height != geom.height())) {
        resizeSwapChain(geom.size());
        m_swapChain->GetDesc1(&desc);
    }

    setupBitmap();
    if (!m_bitmap)
        return;

    if (bitmap != m_bitmap.data()) {
        m_bitmap->deviceContext()->begin();

        ID2D1DeviceContext *dc = m_bitmap->deviceContext()->get();
        if (!m_needsFullFlush) {
            QRegion clipped = region;
            clipped &= QRect(0, 0, desc.Width, desc.Height);

            foreach (const QRect &rect, clipped.rects()) {
                QRectF rectF(rect);
                dc->DrawBitmap(bitmap->bitmap(),
                               to_d2d_rect_f(rectF),
                               1.0,
                               D2D1_INTERPOLATION_MODE_LINEAR,
                               to_d2d_rect_f(rectF.translated(offset.x(), offset.y())));
            }
        } else {
            QRectF rectF(0, 0, desc.Width, desc.Height);
            dc->DrawBitmap(bitmap->bitmap(),
                           to_d2d_rect_f(rectF),
                           1.0,
                           D2D1_INTERPOLATION_MODE_LINEAR,
                           to_d2d_rect_f(rectF.translated(offset.x(), offset.y())));
            m_needsFullFlush = false;
        }

        m_bitmap->deviceContext()->end();
    }
}

void QWindowsDirect2DWindow::present()
{
    m_swapChain->Present(0, 0);
}

void QWindowsDirect2DWindow::resizeSwapChain(const QSize &size)
{
    if (!m_swapChain)
        return;

    m_pixmap.reset();
    m_bitmap.reset();
    m_deviceContext->SetTarget(Q_NULLPTR);

    HRESULT hr = m_swapChain->ResizeBuffers(0,
                                            size.width(), size.height(),
                                            DXGI_FORMAT_UNKNOWN,
                                            0);
    if (FAILED(hr))
        qWarning("%s: Could not resize swap chain: %#x", __FUNCTION__, hr);

    m_needsFullFlush = true;
}

QSharedPointer<QWindowsDirect2DBitmap> QWindowsDirect2DWindow::copyBackBuffer() const
{
    const QSharedPointer<QWindowsDirect2DBitmap> null_result;

    if (!m_bitmap)
        return null_result;

    D2D1_PIXEL_FORMAT format = m_bitmap->bitmap()->GetPixelFormat();
    D2D1_SIZE_U size = m_bitmap->bitmap()->GetPixelSize();

    FLOAT dpiX, dpiY;
    m_bitmap->bitmap()->GetDpi(&dpiX, &dpiY);

    D2D1_BITMAP_PROPERTIES1 properties = {
        format,                     // D2D1_PIXEL_FORMAT pixelFormat;
        dpiX,                       // FLOAT dpiX;
        dpiY,                       // FLOAT dpiY;
        D2D1_BITMAP_OPTIONS_TARGET, // D2D1_BITMAP_OPTIONS bitmapOptions;
        Q_NULLPTR                   // _Field_size_opt_(1) ID2D1ColorContext *colorContext;
    };
    ComPtr<ID2D1Bitmap1> copy;
    HRESULT hr = m_deviceContext.Get()->CreateBitmap(size, NULL, 0, properties, &copy);

    if (FAILED(hr)) {
        qWarning("%s: Could not create staging bitmap: %#x", __FUNCTION__, hr);
        return null_result;
    }

    hr = copy.Get()->CopyFromBitmap(NULL, m_bitmap->bitmap(), NULL);
    if (FAILED(hr)) {
        qWarning("%s: Could not copy from bitmap! %#x", __FUNCTION__, hr);
        return null_result;
    }

    return QSharedPointer<QWindowsDirect2DBitmap>(new QWindowsDirect2DBitmap(copy.Get(), Q_NULLPTR));
}

void QWindowsDirect2DWindow::setupBitmap()
{
    if (m_bitmap)
        return;

    if (!m_deviceContext)
        return;

    if (!m_swapChain)
        return;

    ComPtr<IDXGISurface1> backBufferSurface;
    HRESULT hr = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferSurface));
    if (FAILED(hr)) {
        qWarning("%s: Could not query backbuffer for DXGI Surface: %#x", __FUNCTION__, hr);
        return;
    }

    ComPtr<ID2D1Bitmap1> backBufferBitmap;
    hr = m_deviceContext->CreateBitmapFromDxgiSurface(backBufferSurface.Get(), NULL, backBufferBitmap.GetAddressOf());
    if (FAILED(hr)) {
        qWarning("%s: Could not create Direct2D Bitmap from DXGI Surface: %#x", __FUNCTION__, hr);
        return;
    }

    m_bitmap.reset(new QWindowsDirect2DBitmap(backBufferBitmap.Get(), m_deviceContext.Get()));

    QWindowsDirect2DPlatformPixmap *pp = new QWindowsDirect2DPlatformPixmap(QPlatformPixmap::PixmapType,
                                                                            m_bitmap.data());
    m_pixmap.reset(new QPixmap(pp));
}

QT_END_NAMESPACE
