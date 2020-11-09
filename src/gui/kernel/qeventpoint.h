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

#ifndef QEVENTPOINT_H
#define QEVENTPOINT_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qpointingdevice.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

class QEventPointPrivate;
QT_DECLARE_QESDP_SPECIALIZATION_DTOR_WITH_EXPORT(QEventPointPrivate, Q_GUI_EXPORT)

class Q_GUI_EXPORT QEventPoint
{
    Q_GADGET
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    QDOC_PROPERTY(QPointingDevice *device READ device CONSTANT) // qdoc doesn't know const
    Q_PROPERTY(const QPointingDevice *device READ device CONSTANT)
    Q_PROPERTY(int id READ id CONSTANT)
    Q_PROPERTY(QPointingDeviceUniqueId uniqueId READ uniqueId CONSTANT)
    Q_PROPERTY(State state READ state CONSTANT)
    Q_PROPERTY(ulong timestamp READ timestamp CONSTANT)
    Q_PROPERTY(ulong pressTimestamp READ pressTimestamp CONSTANT)
    Q_PROPERTY(ulong lastTimestamp READ lastTimestamp CONSTANT)
    Q_PROPERTY(qreal timeHeld READ timeHeld CONSTANT)
    Q_PROPERTY(qreal pressure READ pressure CONSTANT)
    Q_PROPERTY(qreal rotation READ rotation CONSTANT)
    Q_PROPERTY(QSizeF ellipseDiameters READ ellipseDiameters CONSTANT)
    Q_PROPERTY(QVector2D velocity READ velocity CONSTANT)
    Q_PROPERTY(QPointF position READ position CONSTANT)
    Q_PROPERTY(QPointF pressPosition READ pressPosition CONSTANT)
    Q_PROPERTY(QPointF grabPosition READ grabPosition CONSTANT)
    Q_PROPERTY(QPointF lastPosition READ lastPosition CONSTANT)
    Q_PROPERTY(QPointF scenePosition READ scenePosition CONSTANT)
    Q_PROPERTY(QPointF scenePressPosition READ scenePressPosition CONSTANT)
    Q_PROPERTY(QPointF sceneGrabPosition READ sceneGrabPosition CONSTANT)
    Q_PROPERTY(QPointF sceneLastPosition READ sceneLastPosition CONSTANT)
    Q_PROPERTY(QPointF globalPosition READ globalPosition CONSTANT)
    Q_PROPERTY(QPointF globalPressPosition READ globalPressPosition CONSTANT)
    Q_PROPERTY(QPointF globalGrabPosition READ globalGrabPosition CONSTANT)
    Q_PROPERTY(QPointF globalLastPosition READ globalLastPosition CONSTANT)
public:
    enum State : quint8 {
        Unknown     = Qt::TouchPointUnknownState,
        Stationary  = Qt::TouchPointStationary,
        Pressed     = Qt::TouchPointPressed,
        Updated     = Qt::TouchPointMoved,
        Released    = Qt::TouchPointReleased
    };
    Q_DECLARE_FLAGS(States, State)
    Q_FLAG(States)

    explicit QEventPoint(int id = -1, const QPointingDevice *device = nullptr);
    QEventPoint(int pointId, State state, const QPointF &scenePosition, const QPointF &globalPosition);
    QEventPoint(const QEventPoint &other) noexcept;
    QEventPoint &operator=(const QEventPoint &other) noexcept;
    QEventPoint(QEventPoint && other) noexcept = default;
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QEventPoint)
    bool operator==(const QEventPoint &other) const noexcept;
    bool operator!=(const QEventPoint &other) const noexcept { return !operator==(other); }
    ~QEventPoint();
    inline void swap(QEventPoint &other) noexcept
    { qSwap(d, other.d); }

    QPointF position() const;
    QPointF pressPosition() const;
    QPointF grabPosition() const;
    QPointF lastPosition() const;
    QPointF scenePosition() const;
    QPointF scenePressPosition() const;
    QPointF sceneGrabPosition() const;
    QPointF sceneLastPosition() const;
    QPointF globalPosition() const;
    QPointF globalPressPosition() const;
    QPointF globalGrabPosition() const;
    QPointF globalLastPosition() const;
    QPointF normalizedPosition() const;

#if QT_DEPRECATED_SINCE(6, 0)
    // QEventPoint replaces QTouchEvent::TouchPoint, so we need all its old accessors, for now
    QT_DEPRECATED_VERSION_X_6_0("Use position()")
    QPointF pos() const { return position(); }
    QT_DEPRECATED_VERSION_X_6_0("Use pressPosition()")
    QPointF startPos() const { return pressPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use scenePosition()")
    QPointF scenePos() const { return scenePosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use scenePressPosition()")
    QPointF startScenePos() const { return scenePressPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use globalPosition()")
    QPointF screenPos() const { return globalPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use globalPressPosition()")
    QPointF startScreenPos() const { return globalPressPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use globalPressPosition()")
    QPointF startNormalizedPos() const;
    QT_DEPRECATED_VERSION_X_6_0("Use normalizedPosition()")
    QPointF normalizedPos() const { return normalizedPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use lastPosition()")
    QPointF lastPos() const { return lastPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use sceneLastPosition()")
    QPointF lastScenePos() const { return sceneLastPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use globalLastPosition()")
    QPointF lastScreenPos() const { return globalLastPosition(); }
    QT_DEPRECATED_VERSION_X_6_0("Use globalLastPosition()")
    QPointF lastNormalizedPos() const;
#endif // QT_DEPRECATED_SINCE(6, 0)
    QVector2D velocity() const;
    State state() const;
    const QPointingDevice *device() const;
    int id() const;
    QPointingDeviceUniqueId uniqueId() const;
    ulong timestamp() const;
    ulong lastTimestamp() const;
    ulong pressTimestamp() const;
    qreal timeHeld() const;
    qreal pressure() const;
    qreal rotation() const;
    QSizeF ellipseDiameters() const;

    bool isAccepted() const;
    void setAccepted(bool accepted = true);

private:
    QExplicitlySharedDataPointer<QEventPointPrivate> d;
    friend class QMutableEventPoint;
    friend class QPointerEvent;
};

#ifndef QT_NO_DEBUG_STREAM
Q_GUI_EXPORT QDebug operator<<(QDebug, const QEventPoint *);
Q_GUI_EXPORT QDebug operator<<(QDebug, const QEventPoint &);
#endif

Q_DECLARE_SHARED(QEventPoint)

QT_END_NAMESPACE

#endif // QEVENTPOINT_H
