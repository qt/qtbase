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
