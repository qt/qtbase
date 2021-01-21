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

#ifndef QEVENT_P_H
#define QEVENT_P_H

#include <QtGui/private/qtguiglobal_p.h>
#include <QtCore/qurl.h>
#include <QtGui/qevent.h>


QT_BEGIN_NAMESPACE

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

class QTouchEventTouchPointPrivate
{
public:
    inline QTouchEventTouchPointPrivate(int id)
        : ref(1),
          id(id),
          state(Qt::TouchPointReleased),
          pressure(-1),
          rotation(0),
          ellipseDiameters(0, 0),
          stationaryWithModifiedProperty(false)
    { }

    inline QTouchEventTouchPointPrivate *detach()
    {
        QTouchEventTouchPointPrivate *d = new QTouchEventTouchPointPrivate(*this);
        d->ref.storeRelaxed(1);
        if (!this->ref.deref())
            delete this;
        return d;
    }

    QAtomicInt ref;
    int id;
    QPointingDeviceUniqueId uniqueId;
    Qt::TouchPointStates state;
    QPointF pos, scenePos, screenPos, normalizedPos,
            startPos, startScenePos, startScreenPos, startNormalizedPos,
            lastPos, lastScenePos, lastScreenPos, lastNormalizedPos;
    qreal pressure;
    qreal rotation;
    QSizeF ellipseDiameters;
    QVector2D velocity;
    QTouchEvent::TouchPoint::InfoFlags flags;
    bool stationaryWithModifiedProperty : 1;
    QVector<QPointF> rawScreenPositions;
};

#if QT_CONFIG(tabletevent)
class QTabletEventPrivate
{
public:
    inline QTabletEventPrivate(Qt::MouseButton button, Qt::MouseButtons buttons)
        : b(button),
          buttonState(buttons)
    { }

    Qt::MouseButton b;
    Qt::MouseButtons buttonState;
};
#endif // QT_CONFIG(tabletevent)

QT_END_NAMESPACE

#endif // QEVENT_P_H
