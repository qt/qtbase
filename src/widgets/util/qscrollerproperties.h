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

#ifndef QSCROLLERPROPERTIES_H
#define QSCROLLERPROPERTIES_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

QT_REQUIRE_CONFIG(scroller);

QT_BEGIN_NAMESPACE


class QScroller;
class QScrollerPrivate;
class QScrollerPropertiesPrivate;

class Q_WIDGETS_EXPORT QScrollerProperties
{
public:
    QScrollerProperties();
    QScrollerProperties(const QScrollerProperties &sp);
    QScrollerProperties &operator=(const QScrollerProperties &sp);
    virtual ~QScrollerProperties();

    bool operator==(const QScrollerProperties &sp) const;
    bool operator!=(const QScrollerProperties &sp) const;

    static void setDefaultScrollerProperties(const QScrollerProperties &sp);
    static void unsetDefaultScrollerProperties();

    enum OvershootPolicy
    {
        OvershootWhenScrollable,
        OvershootAlwaysOff,
        OvershootAlwaysOn
    };

    enum FrameRates {
        Standard,
        Fps60,
        Fps30,
        Fps20
    };

    enum ScrollMetric
    {
        MousePressEventDelay,                    // qreal [s]
        DragStartDistance,                       // qreal [m]
        DragVelocitySmoothingFactor,             // qreal [0..1/s]  (complex calculation involving time) v = v_new* DASF + v_old * (1-DASF)
        AxisLockThreshold,                       // qreal [0..1] atan(|min(dx,dy)|/|max(dx,dy)|)

        ScrollingCurve,                          // QEasingCurve
        DecelerationFactor,                      // slope of the curve

        MinimumVelocity,                         // qreal [m/s]
        MaximumVelocity,                         // qreal [m/s]
        MaximumClickThroughVelocity,             // qreal [m/s]

        AcceleratingFlickMaximumTime,            // qreal [s]
        AcceleratingFlickSpeedupFactor,          // qreal [1..]

        SnapPositionRatio,                       // qreal [0..1]
        SnapTime,                                // qreal [s]

        OvershootDragResistanceFactor,           // qreal [0..1]
        OvershootDragDistanceFactor,             // qreal [0..1]
        OvershootScrollDistanceFactor,           // qreal [0..1]
        OvershootScrollTime,                     // qreal [s]

        HorizontalOvershootPolicy,               // enum OvershootPolicy
        VerticalOvershootPolicy,                 // enum OvershootPolicy
        FrameRate,                               // enum FrameRates

        ScrollMetricCount
    };

    QVariant scrollMetric(ScrollMetric metric) const;
    void setScrollMetric(ScrollMetric metric, const QVariant &value);

protected:
    QScopedPointer<QScrollerPropertiesPrivate> d;

private:
    QScrollerProperties(QScrollerPropertiesPrivate &dd);

    friend class QScrollerPropertiesPrivate;
    friend class QScroller;
    friend class QScrollerPrivate;
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QScrollerProperties::OvershootPolicy)
Q_DECLARE_METATYPE(QScrollerProperties::FrameRates)

#endif // QSCROLLERPROPERTIES_H
