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

#ifndef QEVENT_P_H
#define QEVENT_P_H

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
#include <QtCore/qurl.h>
#include <QtGui/qevent.h>
#include <QtGui/private/qpointingdevice_p.h>

QT_BEGIN_NAMESPACE

// Private subclasses to allow accessing and modifying protected variables.
// These should NOT hold any extra state.

class Q_GUI_EXPORT QMutableEventPoint : public QEventPoint
{
public:
    QMutableEventPoint(int pointId = -1, State state = QEventPoint::State::Stationary,
                       const QPointF &scenePosition = QPointF(), const QPointF &globalPosition = QPointF()) :
        QEventPoint(pointId, state, scenePosition, globalPosition) {}

    QMutableEventPoint(ulong timestamp, int pointId, State state,
                       const QPointF &position, const QPointF &scenePosition, const QPointF &globalPosition) :
        QEventPoint(pointId, state, scenePosition, globalPosition)
    {
        m_timestamp = timestamp;
        m_pos = position;
    }

    static QMutableEventPoint *from(QEventPoint *me) { return static_cast<QMutableEventPoint *>(me); }

    static QMutableEventPoint &from(QEventPoint &me) { return static_cast<QMutableEventPoint &>(me); }

    bool stationaryWithModifiedProperty() const { return m_stationaryWithModifiedProperty; }

    void setId(int pointId) { m_pointId = pointId; }

    void setDevice(const QPointingDevice *device) { m_device = device; }

    void setTimestamp(const ulong t) { m_timestamp = t; }

    void setPressTimestamp(const ulong t) { m_pressTimestamp = t; }

    void setState(QEventPoint::State state) { m_state = state; }

    void setUniqueId(const QPointingDeviceUniqueId &uid) { m_uniqueId = uid; }

    void setPosition(const QPointF &pos) { m_pos = pos; }

    void setScenePosition(const QPointF &pos) { m_scenePos = pos; }

    void setGlobalPosition(const QPointF &pos) { m_globalPos = pos; }

#if QT_DEPRECATED_SINCE(6, 0)
    // temporary replacements for QTouchEvent::TouchPoint setters, mainly to make porting easier
    QT_DEPRECATED_VERSION_X_6_0("Use setPosition()")
    void setPos(const QPointF &pos) { m_pos = pos; }
    QT_DEPRECATED_VERSION_X_6_0("Use setScenePosition()")
    void setScenePos(const QPointF &pos) { m_scenePos = pos; }
    QT_DEPRECATED_VERSION_X_6_0("Use setGlobalPosition()")
    void setScreenPos(const QPointF &pos) { m_globalPos = pos; }
#endif

    void setGlobalPressPosition(const QPointF &pos) { m_globalPressPos = pos; }

    void setGlobalGrabPosition(const QPointF &pos) { m_globalGrabPos = pos; }

    void setGlobalLastPosition(const QPointF &pos) { m_globalLastPos = pos; }

    void setEllipseDiameters(const QSizeF &d) { m_ellipseDiameters = d; }

    void setPressure(qreal v) { m_pressure = v; }

    void setRotation(qreal v) { m_rotation = v; }

    void setVelocity(const QVector2D &v) { m_velocity = v; }

    void setStationaryWithModifiedProperty(bool s = true) { m_stationaryWithModifiedProperty = s; }
};

static_assert(sizeof(QMutableEventPoint) == sizeof(QEventPoint));

class Q_GUI_EXPORT QMutableTouchEvent : public QTouchEvent
{
public:
    QMutableTouchEvent(QEvent::Type eventType,
                       const QPointingDevice *device = nullptr,
                       Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                       const QList<QEventPoint> &touchPoints = QList<QEventPoint>()) :
        QTouchEvent(eventType, device, modifiers, touchPoints) { }

    static QMutableTouchEvent *from(QTouchEvent *e) { return static_cast<QMutableTouchEvent *>(e); }

    static QMutableTouchEvent &from(QTouchEvent &e) { return static_cast<QMutableTouchEvent &>(e); }

    void setTarget(QObject *target) { m_target = target; }

    QList<QEventPoint> &touchPoints() { return m_touchPoints; }
};

static_assert(sizeof(QMutableTouchEvent) == sizeof(QTouchEvent));

class Q_GUI_EXPORT QMutableSinglePointEvent : public QSinglePointEvent
{
public:
    static QMutableSinglePointEvent *from(QSinglePointEvent *e) { return static_cast<QMutableSinglePointEvent *>(e); }

    static QMutableSinglePointEvent &from(QSinglePointEvent &e) { return static_cast<QMutableSinglePointEvent &>(e); }

    QMutableEventPoint &mutablePoint() { return QMutableEventPoint::from(m_point); }

    void setSource(Qt::MouseEventSource s) { m_source = s; }

    bool isDoubleClick() { return m_doubleClick; }

    void setDoubleClick(bool d = true) { m_doubleClick = d; }
};

static_assert(sizeof(QMutableSinglePointEvent) == sizeof(QSinglePointEvent));

QT_END_NAMESPACE

#endif // QEVENT_P_H
