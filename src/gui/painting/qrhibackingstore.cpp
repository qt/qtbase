// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhibackingstore_p.h"
#include "qpa/qplatformwindow.h"
#include <private/qimage_p.h>

QT_BEGIN_NAMESPACE

QRhiBackingStore::QRhiBackingStore(QWindow *window)
    : QRasterBackingStore(window)
{
}

QRhiBackingStore::~QRhiBackingStore()
{
}

void QRhiBackingStore::flush(QWindow *flushedWindow, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (!rhi(flushedWindow)) {
        QPlatformBackingStoreRhiConfig rhiConfig;
        switch (flushedWindow->surfaceType()) {
        case QSurface::OpenGLSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::OpenGL);
            break;
        case QSurface::MetalSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::Metal);
            break;
        case QSurface::Direct3DSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::D3D11);
            break;
        case QSurface::VulkanSurface:
            rhiConfig.setApi(QPlatformBackingStoreRhiConfig::Vulkan);
            break;
        default:
            Q_UNREACHABLE();
        }

        rhiConfig.setEnabled(true);
        createRhi(flushedWindow, rhiConfig);
    }

    // The backing store operates on behalf of its window(), even if we're
    // flushing a child window, so pull the source DPR from the window().
    const qreal sourceDevicePixelRatio = window()->devicePixelRatio();

    // QBackingStore::flush will convert the region and offset from device independent
    // pixels to native pixels before calling QPlatformBackingStore::flush, which means
    // we can't pass on the window's DPR as the sourceTransformFactor, as that will include
    // the Qt scale factor, which has already been applied. Instead we ask the platform
    // window, which only reflect the remaining scale factor from the OS.
    const qreal sourceTransformFactor = flushedWindow->handle()->devicePixelRatio();

    static QPlatformTextureList emptyTextureList;
    bool translucentBackground = m_image.hasAlphaChannel();
    rhiFlush(flushedWindow, sourceDevicePixelRatio,
        region, offset, &emptyTextureList, translucentBackground,
        sourceTransformFactor);
}

QImage::Format QRhiBackingStore::format() const
{
    QImage::Format fmt = QRasterBackingStore::format();

    // With render-to-texture widgets and QRhi-based flushing the backingstore
    // image must have an alpha channel. Hence upgrading the format. Matches
    // what other platforms (Windows, xcb) do.
    if (QImage::toPixelFormat(fmt).alphaUsage() != QPixelFormat::UsesAlpha)
        fmt = qt_maybeDataCompatibleAlphaVersion(fmt);

    return fmt;
}

QT_END_NAMESPACE
