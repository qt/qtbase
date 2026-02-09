// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qeglfskmsdevice_p.h"
#include "qeglfskmsscreen_p.h"
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

void QEglFSKmsDevice::unregisterScreen(QPlatformScreen *screen)
{
    QEglFSKmsScreen *s = static_cast<QEglFSKmsScreen *>(screen);
    for (QPlatformScreen *sibling : s->virtualSiblings())
        static_cast<QEglFSKmsScreen *>(sibling)->removeSibling(s);

    QWindowSystemInterface::handleScreenRemoved(screen);
}

void QEglFSKmsDevice::updateScreen(QPlatformScreen *screen, const QPoint &virtualPos,
                                   const QList<QPlatformScreen *> &virtualSiblings)
{
    QEglFSKmsScreen *s = static_cast<QEglFSKmsScreen *>(screen);
    QRect before = s->geometry();
    s->setVirtualPosition(virtualPos);
    s->setVirtualSiblings(virtualSiblings);
    QRect after = s->geometry();

    if (before != after)
        QWindowSystemInterface::handleScreenGeometryChange(s->screen(), after,
                                                           s->availableGeometry());
}

void QEglFSKmsDevice::updateScreenOutput(QPlatformScreen *screen, const QKmsOutput &output)
{
    QEglFSKmsScreen *s = static_cast<QEglFSKmsScreen *>(screen);
    QRect before = s->geometry();
    s->updateOutput(output);
    QRect after = s->geometry();

    if (before != after)
        QWindowSystemInterface::handleScreenGeometryChange(s->screen(), after,
                                                           s->availableGeometry());
}

QT_END_NAMESPACE
