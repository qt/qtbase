/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QSCROLLERPROPERTIES_P_H
#define QSCROLLERPROPERTIES_P_H

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

#include <QtWidgets/private/qtwidgetsglobal_p.h>
#include <QPointF>
#include <QEasingCurve>
#include <qscrollerproperties.h>

QT_BEGIN_NAMESPACE

class QScrollerPropertiesPrivate
{
public:
    static QScrollerPropertiesPrivate *defaults();

    bool operator==(const QScrollerPropertiesPrivate &) const;

    qreal mousePressEventDelay;
    qreal dragStartDistance;
    qreal dragVelocitySmoothingFactor;
    qreal axisLockThreshold;
    QEasingCurve scrollingCurve;
    qreal decelerationFactor;
    qreal minimumVelocity;
    qreal maximumVelocity;
    qreal maximumClickThroughVelocity;
    qreal acceleratingFlickMaximumTime;
    qreal acceleratingFlickSpeedupFactor;
    qreal snapPositionRatio;
    qreal snapTime;
    qreal overshootDragResistanceFactor;
    qreal overshootDragDistanceFactor;
    qreal overshootScrollDistanceFactor;
    qreal overshootScrollTime;
    QScrollerProperties::OvershootPolicy hOvershootPolicy;
    QScrollerProperties::OvershootPolicy vOvershootPolicy;
    QScrollerProperties::FrameRates frameRate;
};

QT_END_NAMESPACE

#endif // QSCROLLERPROPERTIES_P_H

