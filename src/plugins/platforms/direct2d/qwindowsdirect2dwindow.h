/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
****************************************************************************/

#ifndef QWINDOWSDIRECT2DWINDOW_H
#define QWINDOWSDIRECT2DWINDOW_H

#include "qwindowswindow.h"
#include <wrl.h>

struct IDXGISwapChain1;
struct ID2D1DeviceContext;

QT_BEGIN_NAMESPACE

class QWindowsDirect2DBitmap;

class QWindowsDirect2DWindow : public QWindowsWindow
{
public:
    QWindowsDirect2DWindow(QWindow *window, const QWindowsWindowData &data);
    ~QWindowsDirect2DWindow();

    void setWindowFlags(Qt::WindowFlags flags) override;

    QPixmap *pixmap();
    void flush(QWindowsDirect2DBitmap *bitmap, const QRegion &region, const QPoint &offset);
    void present(const QRegion &region);
    void setupSwapChain();
    void resizeSwapChain(const QSize &size);

    QSharedPointer<QWindowsDirect2DBitmap> copyBackBuffer() const;

private:
    void setupBitmap();

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_deviceContext;
    QScopedPointer<QWindowsDirect2DBitmap> m_bitmap;
    QScopedPointer<QPixmap> m_pixmap;
    bool m_needsFullFlush = true;
    bool m_directRendering = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DWINDOW_H
