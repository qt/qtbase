/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCROLLERPROPERTIES_H
#define QSCROLLERPROPERTIES_H

#include <QtCore/QScopedPointer>
#include <QtCore/QMetaType>
#include <QtCore/QVariant>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Gui)

class QScroller;
class QScrollerPrivate;
class QScrollerPropertiesPrivate;

class Q_GUI_EXPORT QScrollerProperties
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

QT_END_HEADER

#endif // QSCROLLERPROPERTIES_H
