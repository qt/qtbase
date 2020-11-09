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
        d->timestamp = timestamp;
        d->pos = position;
    }

    void updateFrom(const QEventPoint &other);

    static QMutableEventPoint *from(QEventPoint *me) { return static_cast<QMutableEventPoint *>(me); }

    static QMutableEventPoint &from(QEventPoint &me) { return static_cast<QMutableEventPoint &>(me); }

    static const QMutableEventPoint &constFrom(const QEventPoint &me) { return static_cast<const QMutableEventPoint &>(me); }

    void detach();

    void setId(int pointId) { d->pointId = pointId; }

    void setDevice(const QPointingDevice *device) { d->device = device; }

    void setTimestamp(const ulong t);

    void setPressTimestamp(const ulong t) { d->pressTimestamp = t; }

    void setState(QEventPoint::State state) { d->state = state; }

    void setUniqueId(const QPointingDeviceUniqueId &uid) { d->uniqueId = uid; }

    void setPosition(const QPointF &pos) { d->pos = pos; }

    void setScenePosition(const QPointF &pos) { d->scenePos = pos; }

    void setGlobalPosition(const QPointF &pos) { d->globalPos = pos; }

#if QT_DEPRECATED_SINCE(6, 0)
    // temporary replacements for QTouchEvent::TouchPoint setters, mainly to make porting easier
    QT_DEPRECATED_VERSION_X_6_0("Use setPosition()")
    void setPos(const QPointF &pos) { d->pos = pos; }
    QT_DEPRECATED_VERSION_X_6_0("Use setScenePosition()")
    void setScenePos(const QPointF &pos) { d->scenePos = pos; }
    QT_DEPRECATED_VERSION_X_6_0("Use setGlobalPosition()")
    void setScreenPos(const QPointF &pos) { d->globalPos = pos; }
#endif

    void setGlobalPressPosition(const QPointF &pos) { d->globalPressPos = pos; }

    void setGlobalGrabPosition(const QPointF &pos) { d->globalGrabPos = pos; }

    void setGlobalLastPosition(const QPointF &pos) { d->globalLastPos = pos; }

    void setEllipseDiameters(const QSizeF &diams) { d->ellipseDiameters = diams; }

    void setPressure(qreal v) { d->pressure = v; }

    void setRotation(qreal v) { d->rotation = v; }

    void setVelocity(const QVector2D &v) { d->velocity = v; }

    QWindow *window() const { return d->window.data(); }

    void setWindow(const QPointer<QWindow> &w) { d->window = w; }

    QObject *target() const { return d->target.data(); }

    void setTarget(const QPointer<QObject> &t) { d->target = t; }
};

static_assert(sizeof(QMutableEventPoint) == sizeof(QEventPoint));

QT_END_NAMESPACE

#endif // QEVENTPOINT_P_H
