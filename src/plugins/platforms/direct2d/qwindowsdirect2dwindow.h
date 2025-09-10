// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSDIRECT2DWINDOW_H
#define QWINDOWSDIRECT2DWINDOW_H

#include "qwindowswindow.h"
#include <QtCore/private/qcomptr_p.h>

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
    ComPtr<IDXGISwapChain1> m_swapChain;
    ComPtr<ID2D1DeviceContext> m_deviceContext;
    QScopedPointer<QWindowsDirect2DBitmap> m_bitmap;
    QScopedPointer<QPixmap> m_pixmap;
    bool m_needsFullFlush = true;
    bool m_directRendering = false;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DWINDOW_H
