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

#ifndef QEGLFSKMSDEVICE_H
#define QEGLFSKMSDEVICE_H

#include "private/qeglfsglobal_p.h"
#include "qeglfskmseventreader.h"
#include <QtKmsSupport/private/qkmsdevice_p.h>

QT_BEGIN_NAMESPACE

class Q_EGLFS_EXPORT QEglFSKmsDevice : public QKmsDevice
{
public:
    QEglFSKmsDevice(QKmsScreenConfig *screenConfig, const QString &path);

    void registerScreen(QPlatformScreen *screen,
                        bool isPrimary,
                        const QPoint &virtualPos,
                        const QList<QPlatformScreen *> &virtualSiblings) override;

    QEglFSKmsEventReader *eventReader() { return &m_eventReader; }

protected:
    QEglFSKmsEventReader m_eventReader;
};

QT_END_NAMESPACE

#endif // QEGLFSKMSDEVICE_H
