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

#ifndef QPARALLELANIMATIONGROUP_H
#define QPARALLELANIMATIONGROUP_H

#include <QtCore/qanimationgroup.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QParallelAnimationGroupPrivate;
class Q_CORE_EXPORT QParallelAnimationGroup : public QAnimationGroup
{
    Q_OBJECT

public:
    QParallelAnimationGroup(QObject *parent = nullptr);
    ~QParallelAnimationGroup();

    int duration() const override;

protected:
    QParallelAnimationGroup(QParallelAnimationGroupPrivate &dd, QObject *parent);
    bool event(QEvent *event) override;

    void updateCurrentTime(int currentTime) override;
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState) override;
    void updateDirection(QAbstractAnimation::Direction direction) override;

private:
    Q_DISABLE_COPY(QParallelAnimationGroup)
    Q_DECLARE_PRIVATE(QParallelAnimationGroup)
    Q_PRIVATE_SLOT(d_func(), void _q_uncontrolledAnimationFinished())
};

QT_END_NAMESPACE

#endif // QPARALLELANIMATIONGROUP
