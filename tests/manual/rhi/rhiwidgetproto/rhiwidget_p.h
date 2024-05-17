// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef RHIWIDGET_P_H
#define RHIWIDGET_P_H

#include "rhiwidget.h"

#include <private/qwidget_p.h>
#include <private/qbackingstorerhisupport_p.h>

class QRhiWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QRhiWidget)
public:
    TextureData texture() const override
    {
        TextureData td;
        if (!textureInvalid)
            td.textureLeft = t;
        return td;
    }
    QPlatformBackingStoreRhiConfig rhiConfig() const override;

    void ensureRhi();
    void ensureTexture();

    QRhi *rhi = nullptr;
    QRhiTexture *t = nullptr;
    bool noSize = false;
    QPlatformBackingStoreRhiConfig config;
    QRhiTexture::Format format = QRhiTexture::RGBA8;
    QSize explicitSize;
    QBackingStoreRhiSupport offscreenRenderer;
    bool textureInvalid = false;
};

#endif
