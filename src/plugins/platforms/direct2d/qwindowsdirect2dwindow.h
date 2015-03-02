/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
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

    void setWindowFlags(Qt::WindowFlags flags) Q_DECL_OVERRIDE;

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
    bool m_needsFullFlush;
    bool m_directRendering;
};

QT_END_NAMESPACE

#endif // QWINDOWSDIRECT2DWINDOW_H
