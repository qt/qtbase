// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSDIRECT2DCONTEXT_H
#define QWINDOWSDIRECT2DCONTEXT_H

#include <QtCore/qscopedpointer.h>

struct ID3D11Device;
struct ID2D1Device;
struct ID2D1Factory1;
struct IDXGIFactory2;
struct ID3D11DeviceContext;
struct IDWriteFactory;
struct IDWriteGdiInterop;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DContextPrivate;
class QWindowsDirect2DContext
{
    Q_DECLARE_PRIVATE( QWindowsDirect2DContext )

public:
    QWindowsDirect2DContext();
    ~QWindowsDirect2DContext();

    bool init();

    static QWindowsDirect2DContext *instance();

    ID3D11Device *d3dDevice() const;
    ID2D1Device *d2dDevice() const;
    ID2D1Factory1 *d2dFactory() const;
    IDXGIFactory2 *dxgiFactory() const;
    ID3D11DeviceContext *d3dDeviceContext() const;
    IDWriteFactory *dwriteFactory() const;
    IDWriteGdiInterop *dwriteGdiInterop() const;

private:
    QScopedPointer<QWindowsDirect2DContextPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DCONTEXT_H
