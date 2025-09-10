// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrhibackingstore_p.h"
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

    static QPlatformTextureList emptyTextureList;
    bool translucentBackground = m_image.hasAlphaChannel();
    rhiFlush(flushedWindow, flushedWindow->devicePixelRatio(),
        region, offset, &emptyTextureList, translucentBackground);
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
