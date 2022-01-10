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

#ifndef QEVENTPOINT_P_H
#define QEVENTPOINT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of other Qt classes. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qevent.h>
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcPointerVel);
Q_DECLARE_LOGGING_CATEGORY(lcEPDetach);

class QPointingDevice;

class QEventPointPrivate : public QSharedData
{
public:
    QEventPointPrivate(int id, const QPointingDevice *device)
      : device(device), pointId(id) { }

    QEventPointPrivate(int pointId, QEventPoint::State state, const QPointF &scenePosition, const QPointF &globalPosition)
        : scenePos(scenePosition), globalPos(globalPosition), pointId(pointId), state(state)
    {
        if (state == QEventPoint::State::Released)
            pressure = 0;
    }
    inline bool operator==(const QEventPointPrivate &other) const
    {
        return device == other.device
            && window == other.window
            && target == other.target
            && pos == other.pos
            && scenePos == other.scenePos
            && globalPos == other.globalPos
            && globalPressPos == other.globalPressPos
            && globalGrabPos == other.globalGrabPos
            && globalLastPos == other.globalLastPos
            && pressure == other.pressure
            && rotation == other.rotation
            && ellipseDiameters == other.ellipseDiameters
            && velocity == other.velocity
            && timestamp == other.timestamp
            && lastTimestamp == other.lastTimestamp
            && pressTimestamp == other.pressTimestamp
            && uniqueId == other.uniqueId
            && pointId == other.pointId
            && state == other.state;
    }

    const QPointingDevice *device = nullptr;
    QPointer<QWindow> window;
    QPointer<QObject> target;
    QPointF pos, scenePos, globalPos,
            globalPressPos, globalGrabPos, globalLastPos;
    qreal pressure = 1;
    qreal rotation = 0;
    QSizeF ellipseDiameters = QSizeF(0, 0);
    QVector2D velocity;
    ulong timestamp = 0;
    ulong lastTimestamp = 0;
    ulong pressTimestamp = 0;
    QPointingDeviceUniqueId uniqueId;
    int pointId = -1;
    QEventPoint::State state = QEventPoint::State::Unknown;
    bool accept = false;
};

// Private subclasses to allow accessing and modifying protected variables.
// These should NOT hold any extra state.

class QMutableEventPoint
{
public:
    static QEventPoint withTimeStamp(ulong timestamp, int pointId, QEventPoint::State state,
                                     QPointF position, QPointF scenePosition, QPointF globalPosition)
    {
        QEventPoint p(pointId, state, scenePosition, globalPosition);
        p.d->timestamp = timestamp;
        p.d->pos = position;
        return p;
    }

    static Q_GUI_EXPORT void update(const QEventPoint &from, QEventPoint &to);

    static Q_GUI_EXPORT void detach(QEventPoint &p);

#define TRIVIAL_SETTER(type, field, Field) \
    static void set##Field (QEventPoint &p, type arg) { p.d->field = std::move(arg); } \
    /* end */

    TRIVIAL_SETTER(int, pointId, Id)
    TRIVIAL_SETTER(const QPointingDevice *, device, Device)

    // not trivial:
    static Q_GUI_EXPORT void setTimestamp(QEventPoint &p, ulong t);

    TRIVIAL_SETTER(ulong, pressTimestamp, PressTimestamp)
    TRIVIAL_SETTER(QEventPoint::State, state, State)
    TRIVIAL_SETTER(QPointingDeviceUniqueId, uniqueId, UniqueId)
    TRIVIAL_SETTER(QPointF, pos, Position)
    TRIVIAL_SETTER(QPointF, scenePos, ScenePosition)
    TRIVIAL_SETTER(QPointF, globalPos, GlobalPosition)

    TRIVIAL_SETTER(QPointF, globalPressPos, GlobalPressPosition)
    TRIVIAL_SETTER(QPointF, globalGrabPos, GlobalGrabPosition)
    TRIVIAL_SETTER(QPointF, globalLastPos, GlobalLastPosition)
    TRIVIAL_SETTER(QSizeF, ellipseDiameters, EllipseDiameters)
    TRIVIAL_SETTER(qreal, pressure, Pressure)
    TRIVIAL_SETTER(qreal, rotation, Rotation)
    TRIVIAL_SETTER(QVector2D, velocity, Velocity)

    static QWindow *window(const QEventPoint &p) { return p.d->window.data(); }

    TRIVIAL_SETTER(QWindow *, window, Window)

    static QObject *target(const QEventPoint &p) { return p.d->target.data(); }

    TRIVIAL_SETTER(QObject *, target, Target)

#undef TRIVIAL_SETTER
};

QT_END_NAMESPACE

#endif // QEVENTPOINT_P_H
