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

#ifndef QSEQUENTIALANIMATIONGROUP_H
#define QSEQUENTIALANIMATIONGROUP_H

#include <QtCore/qanimationgroup.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QPauseAnimation;
class QSequentialAnimationGroupPrivate;

class Q_CORE_EXPORT QSequentialAnimationGroup : public QAnimationGroup
{
    Q_OBJECT
    Q_PROPERTY(QAbstractAnimation* currentAnimation READ currentAnimation NOTIFY currentAnimationChanged)

public:
    QSequentialAnimationGroup(QObject *parent = nullptr);
    ~QSequentialAnimationGroup();

    QPauseAnimation *addPause(int msecs);
    QPauseAnimation *insertPause(int index, int msecs);

    QAbstractAnimation *currentAnimation() const;
    int duration() const override;

Q_SIGNALS:
    void currentAnimationChanged(QAbstractAnimation *current);

protected:
    QSequentialAnimationGroup(QSequentialAnimationGroupPrivate &dd, QObject *parent);
    bool event(QEvent *event) override;

    void updateCurrentTime(int) override;
    void updateState(QAbstractAnimation::State newState, QAbstractAnimation::State oldState) override;
    void updateDirection(QAbstractAnimation::Direction direction) override;

private:
    Q_DISABLE_COPY(QSequentialAnimationGroup)
    Q_DECLARE_PRIVATE(QSequentialAnimationGroup)
    Q_PRIVATE_SLOT(d_func(), void _q_uncontrolledAnimationFinished())
};

QT_END_NAMESPACE

#endif //QSEQUENTIALANIMATIONGROUP_H
