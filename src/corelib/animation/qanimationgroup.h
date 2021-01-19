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

#ifndef QANIMATIONGROUP_H
#define QANIMATIONGROUP_H

#include <QtCore/qabstractanimation.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QAnimationGroupPrivate;
class Q_CORE_EXPORT QAnimationGroup : public QAbstractAnimation
{
    Q_OBJECT

public:
    QAnimationGroup(QObject *parent = nullptr);
    ~QAnimationGroup();

    QAbstractAnimation *animationAt(int index) const;
    int animationCount() const;
    int indexOfAnimation(QAbstractAnimation *animation) const;
    void addAnimation(QAbstractAnimation *animation);
    void insertAnimation(int index, QAbstractAnimation *animation);
    void removeAnimation(QAbstractAnimation *animation);
    QAbstractAnimation *takeAnimation(int index);
    void clear();

protected:
    QAnimationGroup(QAnimationGroupPrivate &dd, QObject *parent);
    bool event(QEvent *event) override;

private:
    Q_DISABLE_COPY(QAnimationGroup)
    Q_DECLARE_PRIVATE(QAnimationGroup)
};

QT_END_NAMESPACE

#endif //QANIMATIONGROUP_H
