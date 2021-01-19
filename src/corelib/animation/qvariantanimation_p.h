/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QVARIANTANIMATION_P_H
#define QVARIANTANIMATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qvariantanimation.h"
#include <QtCore/qeasingcurve.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qvector.h>

#include "private/qabstractanimation_p.h"

#include <type_traits>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QVariantAnimationPrivate : public QAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QVariantAnimation)
public:

    QVariantAnimationPrivate();

    static QVariantAnimationPrivate *get(QVariantAnimation *q)
    {
        return q->d_func();
    }

    void setDefaultStartEndValue(const QVariant &value);


    QVariant currentValue;
    QVariant defaultStartEndValue;

    //this is used to keep track of the KeyValue interval in which we currently are
    struct
    {
        QVariantAnimation::KeyValue start, end;
    } currentInterval;

    QEasingCurve easing;
    int duration;
    QVariantAnimation::KeyValues keyValues;
    QVariantAnimation::Interpolator interpolator;

    void setCurrentValueForProgress(const qreal progress);
    void recalculateCurrentInterval(bool force=false);
    void setValueAt(qreal, const QVariant &);
    QVariant valueAt(qreal step) const;
    void convertValues(int t);

    void updateInterpolator();

    //XXX this is needed by dui
    static Q_CORE_EXPORT QVariantAnimation::Interpolator getInterpolator(int interpolationType);
};

//this should make the interpolation faster
template<typename T>
typename std::enable_if<std::is_unsigned<T>::value, T>::type
_q_interpolate(const T &f, const T &t, qreal progress)
{
    return T(f + t * progress - f * progress);
}

// the below will apply also to all non-arithmetic types
template<typename T>
typename std::enable_if<!std::is_unsigned<T>::value, T>::type
_q_interpolate(const T &f, const T &t, qreal progress)
{
    return T(f + (t - f) * progress);
}

template<typename T > inline QVariant _q_interpolateVariant(const T &from, const T &to, qreal progress)
{
    return _q_interpolate(from, to, progress);
}


QT_END_NAMESPACE

#endif //QVARIANTANIMATION_P_H
