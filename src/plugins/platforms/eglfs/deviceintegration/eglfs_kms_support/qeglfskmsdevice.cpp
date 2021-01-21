/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Copyright (C) 2016 Pelagicore AG
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

#include "qeglfskmsdevice.h"
#include "qeglfskmsscreen.h"
#include "private/qeglfsintegration_p.h"
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

QEglFSKmsDevice::QEglFSKmsDevice(QKmsScreenConfig *screenConfig, const QString &path)
    : QKmsDevice(screenConfig, path)
{
}

void QEglFSKmsDevice::registerScreen(QPlatformScreen *screen,
                                     bool isPrimary,
                                     const QPoint &virtualPos,
                                     const QList<QPlatformScreen *> &virtualSiblings)
{
    QEglFSKmsScreen *s = static_cast<QEglFSKmsScreen *>(screen);
    s->setVirtualPosition(virtualPos);
    s->setVirtualSiblings(virtualSiblings);
    QWindowSystemInterface::handleScreenAdded(s, isPrimary);
}

QT_END_NAMESPACE
