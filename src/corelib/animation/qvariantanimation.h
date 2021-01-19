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

#ifndef QVARIANTANIMATION_H
#define QVARIANTANIMATION_H

#include <QtCore/qeasingcurve.h>
#include <QtCore/qabstractanimation.h>
#include <QtCore/qvector.h>
#include <QtCore/qvariant.h>
#include <QtCore/qpair.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QVariantAnimationPrivate;
class Q_CORE_EXPORT QVariantAnimation : public QAbstractAnimation
{
    Q_OBJECT
    Q_PROPERTY(QVariant startValue READ startValue WRITE setStartValue)
    Q_PROPERTY(QVariant endValue READ endValue WRITE setEndValue)
    Q_PROPERTY(QVariant currentValue READ currentValue NOTIFY valueChanged)
    Q_PROPERTY(int duration READ duration WRITE setDuration)
    Q_PROPERTY(QEasingCurve easingCurve READ easingCurve WRITE setEasingCurve)

public:
    typedef QPair<qreal, QVariant> KeyValue;
    typedef QVector<KeyValue> KeyValues;

    QVariantAnimation(QObject *parent = nullptr);
    ~QVariantAnimation();

    QVariant startValue() const;
    void setStartValue(const QVariant &value);

    QVariant endValue() const;
    void setEndValue(const QVariant &value);

    QVariant keyValueAt(qreal step) const;
    void setKeyValueAt(qreal step, const QVariant &value);

    KeyValues keyValues() const;
    void setKeyValues(const KeyValues &values);

    QVariant currentValue() const;

    int duration() const override;
    void setDuration(int msecs);

    QEasingCurve easingCurve() const;
    void setEasingCurve(const QEasingCurve &easing);

    typedef QVariant (*Interpolator)(const void *from, const void *to, qreal progress);

Q_SIGNALS:
    void valueChanged(const QVariant &value);

protected:
    QVariantAnimation(QVariantAnimationPrivate &dd, QObject *parent = nullptr);
    bool event(QEvent *event) override;

    void updateCurrentTime(int) override;
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState) override;

    virtual void updateCurrentValue(const QVariant &value);
    virtual QVariant interpolated(const QVariant &from, const QVariant &to, qreal progress) const;

private:
    template <typename T> friend void qRegisterAnimationInterpolator(QVariant (*func)(const T &, const T &, qreal));
    static void registerInterpolator(Interpolator func, int interpolationType);

    Q_DISABLE_COPY(QVariantAnimation)
    Q_DECLARE_PRIVATE(QVariantAnimation)
};

template <typename T>
void qRegisterAnimationInterpolator(QVariant (*func)(const T &from, const T &to, qreal progress)) {
    QVariantAnimation::registerInterpolator(reinterpret_cast<QVariantAnimation::Interpolator>(reinterpret_cast<void(*)()>(func)), qMetaTypeId<T>());
}

QT_END_NAMESPACE

#endif //QVARIANTANIMATION_H
