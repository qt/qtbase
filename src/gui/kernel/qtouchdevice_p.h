/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QTOUCHDEVICE_P_H
#define QTOUCHDEVICE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qtouchdevice.h>

QT_BEGIN_NAMESPACE


class QTouchDevicePrivate
{
public:
    QTouchDevicePrivate()
        : type(QTouchDevice::TouchScreen),
          caps(QTouchDevice::Position),
          maxTouchPoints(1)
    {
        static quint8 nextId = 2;   // device 0 is not used, device 1 is for mouse device
        id = nextId++;
    }

    QTouchDevice::DeviceType type;
    QTouchDevice::Capabilities caps;
    QString name;
    int maxTouchPoints;
    quint8 id;

    static void registerDevice(const QTouchDevice *dev);
    static void unregisterDevice(const QTouchDevice *dev);
    static bool isRegistered(const QTouchDevice *dev);
    static const QTouchDevice *deviceById(quint8 id);
    static QTouchDevicePrivate *get(QTouchDevice *q) { return q->d; }
};

QT_END_NAMESPACE

#endif // QTOUCHDEVICE_P_H
