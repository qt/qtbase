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

#include "qwindowsgdiintegration.h"
#include "qwindowscontext.h"
#include "qwindowsbackingstore.h"
#include "qwindowsgdinativeinterface.h"

#include <QtCore/qdebug.h>
#include <QtGui/private/qpixmap_raster_p.h>

QT_BEGIN_NAMESPACE

class QWindowsGdiIntegrationPrivate
{
public:
    QWindowsGdiNativeInterface m_nativeInterface;
};

QWindowsGdiIntegration::QWindowsGdiIntegration(const QStringList &paramList)
    : QWindowsIntegration(paramList)
    , d(new QWindowsGdiIntegrationPrivate)
{}

QWindowsGdiIntegration::~QWindowsGdiIntegration()
{}

QPlatformNativeInterface *QWindowsGdiIntegration::nativeInterface() const
{
    return &d->m_nativeInterface;
}

QPlatformPixmap *QWindowsGdiIntegration::createPlatformPixmap(QPlatformPixmap::PixelType type) const
{
    return new QRasterPlatformPixmap(type);
}

QPlatformBackingStore *QWindowsGdiIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QWindowsBackingStore(window);
}

QT_END_NAMESPACE
