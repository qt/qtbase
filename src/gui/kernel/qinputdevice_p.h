/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QINPUTDEVICE_P_H
#define QINPUTDEVICE_P_H

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
#include <QtGui/qinputdevice.h>
#include "private/qobject_p.h"

QT_BEGIN_NAMESPACE

class Q_GUI_EXPORT QInputDevicePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QInputDevice)
public:
    QInputDevicePrivate(const QString &name, qint64 winSysId, QInputDevice::DeviceType type,
                        QInputDevice::Capabilities caps = QInputDevice::Capability::None,
                        const QString &seatName = QString())
      : name(name), seatName(seatName), systemId(winSysId), capabilities(caps),
        deviceType(type)
    {
        // if the platform doesn't provide device IDs, make one up,
        // but try to avoid clashing with OS-provided 32-bit IDs
        static qint64 nextId = qint64(1) << 33;
        if (!systemId)
            systemId = nextId++;
    }

    QString name;
    QString seatName;
    QString busId;
    QRect availableVirtualGeometry;
    void *qqExtra = nullptr;    // Qt Quick can store arbitrary device-specific data here
    qint64 systemId = 0;
    QInputDevice::Capabilities capabilities = QInputDevice::Capability::None;
    QInputDevice::DeviceType deviceType = QInputDevice::DeviceType::Unknown;
    bool pointingDeviceType = false;

    static void registerDevice(const QInputDevice *dev);
    static void unregisterDevice(const QInputDevice *dev);
    static bool isRegistered(const QInputDevice *dev);
    static const QInputDevice *fromId(qint64 systemId);

    void setAvailableVirtualGeometry(QRect a)
    {
        if (a == availableVirtualGeometry)
            return;

        availableVirtualGeometry = a;
        capabilities |= QInputDevice::Capability::NormalizedPosition;
        Q_Q(QInputDevice);
        Q_EMIT q->availableVirtualGeometryChanged(availableVirtualGeometry);
    }

    inline static QInputDevicePrivate *get(QInputDevice *q)
    {
        return static_cast<QInputDevicePrivate *>(QObjectPrivate::get(q));
    }

    inline static const QInputDevicePrivate *get(const QInputDevice *q)
    {
        return static_cast<const QInputDevicePrivate *>(QObjectPrivate::get(q));
    }
};

QT_END_NAMESPACE

#endif // QINPUTDEVICE_P_H
