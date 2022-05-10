// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdirect2dcontext.h"
#include "qwindowsdirect2dhelpers.h"
#include "qwindowsdirect2ddevicecontext.h"

#include <wrl.h>

using Microsoft::WRL::ComPtr;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DDeviceContextPrivate {
public:
    QWindowsDirect2DDeviceContextPrivate(ID2D1DeviceContext *dc)
        : deviceContext(dc)
    {
        if (!dc) {
            HRESULT hr = QWindowsDirect2DContext::instance()->d2dDevice()->CreateDeviceContext(
                        D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                        &deviceContext);
            if (Q_UNLIKELY(FAILED(hr)))
                qFatal("%s: Couldn't create Direct2D Device Context: %#lx", __FUNCTION__, hr);
        }

        Q_ASSERT(deviceContext);
        deviceContext->SetUnitMode(D2D1_UNIT_MODE_PIXELS);
    }

    void begin()
    {
        Q_ASSERT(deviceContext);
        Q_ASSERT(refCount >= 0);

        if (refCount == 0)
            deviceContext->BeginDraw();

        refCount++;
    }

    bool end()
    {
        Q_ASSERT(deviceContext);
        Q_ASSERT(refCount > 0);

        bool success = true;
        refCount--;

        if (refCount == 0) {
            D2D1_TAG tag1, tag2;
            HRESULT hr = deviceContext->EndDraw(&tag1, &tag2);

            if (FAILED(hr)) {
                success = false;
                qWarning("%s: EndDraw failed: %#lx, tag1: %lld, tag2: %lld",
                         __FUNCTION__, long(hr), tag1, tag2);
            }
        }

        return success;
    }

    ComPtr<ID2D1DeviceContext> deviceContext;
    int refCount = 0;
};

QWindowsDirect2DDeviceContext::QWindowsDirect2DDeviceContext(ID2D1DeviceContext *dc)
    : d_ptr(new QWindowsDirect2DDeviceContextPrivate(dc))
{
}

QWindowsDirect2DDeviceContext::~QWindowsDirect2DDeviceContext()
{

}

ID2D1DeviceContext *QWindowsDirect2DDeviceContext::get() const
{
    Q_D(const QWindowsDirect2DDeviceContext);
    Q_ASSERT(d->deviceContext);

    return d->deviceContext.Get();
}

void QWindowsDirect2DDeviceContext::begin()
{
    Q_D(QWindowsDirect2DDeviceContext);
    d->begin();
}

bool QWindowsDirect2DDeviceContext::end()
{
    Q_D(QWindowsDirect2DDeviceContext);
    return d->end();
}

void QWindowsDirect2DDeviceContext::suspend()
{
    Q_D(QWindowsDirect2DDeviceContext);
    if (d->refCount > 0)
        d->deviceContext->EndDraw();
}

void QWindowsDirect2DDeviceContext::resume()
{
    Q_D(QWindowsDirect2DDeviceContext);
    if (d->refCount > 0)
        d->deviceContext->BeginDraw();
}

QWindowsDirect2DDeviceContextSuspender::QWindowsDirect2DDeviceContextSuspender(QWindowsDirect2DDeviceContext *dc)
    : m_dc(dc)
{
    Q_ASSERT(m_dc);
    m_dc->suspend();
}

QWindowsDirect2DDeviceContextSuspender::~QWindowsDirect2DDeviceContextSuspender()
{
    resume();
}

void QWindowsDirect2DDeviceContextSuspender::resume()
{
    if (m_dc) {
        m_dc->resume();
        m_dc = nullptr;
    }
}

QT_END_NAMESPACE
