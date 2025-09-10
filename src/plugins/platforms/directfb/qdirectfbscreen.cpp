// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdirectfbscreen.h"
#include "qdirectfbcursor.h"

QT_BEGIN_NAMESPACE

QDirectFbScreen::QDirectFbScreen(int display)
    : QPlatformScreen()
    , m_layer(QDirectFbConvenience::dfbDisplayLayer(display))
{
    m_layer->SetCooperativeLevel(m_layer.data(), DLSCL_SHARED);

    DFBDisplayLayerConfig config;
    m_layer->GetConfiguration(m_layer.data(), &config);

    m_format = QDirectFbConvenience::imageFormatFromSurfaceFormat(config.pixelformat, config.surface_caps);
    m_geometry = QRect(0, 0, config.width, config.height);
    const int dpi = 72;
    const qreal inch = 25.4;
    m_depth = QDirectFbConvenience::colorDepthForSurface(config.pixelformat);
    m_physicalSize = QSizeF(config.width, config.height) * inch / dpi;

    m_cursor.reset(new QDirectFBCursor(this));
}

IDirectFBDisplayLayer *QDirectFbScreen::dfbLayer() const
{
    return m_layer.data();
}


QT_END_NAMESPACE
