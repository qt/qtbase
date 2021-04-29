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

#ifndef QPOINTINGDEVICE_P_H
#define QPOINTINGDEVICE_P_H

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

#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qevent_p.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/private/qinputdevice_p.h>
#include <QtCore/private/qflatmap_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPointerGrab);

class Q_GUI_EXPORT QPointingDevicePrivate : public QInputDevicePrivate
{
    Q_DECLARE_PUBLIC(QPointingDevice)
public:
    QPointingDevicePrivate(const QString &name, qint64 id, QInputDevice::DeviceType type,
                           QPointingDevice::PointerType pType, QPointingDevice::Capabilities caps,
                           int maxPoints, int buttonCount,
                           const QString &seatName = QString(),
                           QPointingDeviceUniqueId uniqueId = QPointingDeviceUniqueId())
      : QInputDevicePrivate(name, id, type, caps, seatName),
        uniqueId(uniqueId),
        maximumTouchPoints(qint8(maxPoints)), buttonCount(qint8(buttonCount)),
        pointerType(pType)
    {
        pointingDeviceType = true;
        activePoints.reserve(maxPoints);
    }

    void sendTouchCancelEvent(QTouchEvent *cancelEvent);

    /*! \internal
        This struct (stored in activePoints) holds persistent state between event deliveries.
    */
    struct EventPointData {
        QEventPoint eventPoint;
        QPointer<QObject> exclusiveGrabber;
        QList<QPointer <QObject> > passiveGrabbers;
    };
    EventPointData *queryPointById(int id) const;
    EventPointData *pointById(int id) const;
    void removePointById(int id);
    QObject *firstActiveTarget() const;
    QWindow *firstActiveWindow() const;

    QObject *firstPointExclusiveGrabber() const;
    void setExclusiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *exclusiveGrabber);
    bool removeExclusiveGrabber(const QPointerEvent *event, const QObject *grabber);
    bool addPassiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *grabber);
    bool removePassiveGrabber(const QPointerEvent *event, const QEventPoint &point, QObject *grabber);
    void clearPassiveGrabbers(const QPointerEvent *event, const QEventPoint &point);
    void removeGrabber(QObject *grabber, bool cancel = false);

    using EventPointMap = QFlatMap<int, EventPointData>;
    mutable EventPointMap activePoints;

    QPointingDeviceUniqueId uniqueId;
    quint32 toolId = 0;         // only for Wacom tablets
    qint8 maximumTouchPoints = 0;
    qint8 buttonCount = 0;
    QPointingDevice::PointerType pointerType = QPointingDevice::PointerType::Unknown;
    bool toolProximity = false;  // only for Wacom tablets

    inline static QPointingDevicePrivate *get(QPointingDevice *q)
    {
        return static_cast<QPointingDevicePrivate *>(QObjectPrivate::get(q));
    }

    inline static const QPointingDevicePrivate *get(const QPointingDevice *q)
    {
        return static_cast<const QPointingDevicePrivate *>(QObjectPrivate::get(q));
    }

    static const QPointingDevice *tabletDevice(QInputDevice::DeviceType deviceType,
                                               QPointingDevice::PointerType pointerType,
                                               QPointingDeviceUniqueId uniqueId);

    static const QPointingDevice *queryTabletDevice(QInputDevice::DeviceType deviceType,
                                                    QPointingDevice::PointerType pointerType,
                                                    QPointingDeviceUniqueId uniqueId,
                                                    qint64 systemId = 0);

    static const QPointingDevice *pointingDeviceById(qint64 systemId);
};

QT_END_NAMESPACE

#endif // QPOINTINGDEVICE_P_H
