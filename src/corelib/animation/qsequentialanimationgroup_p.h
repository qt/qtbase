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

#ifndef QSEQUENTIALANIMATIONGROUP_P_H
#define QSEQUENTIALANIMATIONGROUP_P_H

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

#include "qsequentialanimationgroup.h"
#include "private/qanimationgroup_p.h"

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QSequentialAnimationGroupPrivate : public QAnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QSequentialAnimationGroup)
public:
    QSequentialAnimationGroupPrivate()
        : currentAnimation(nullptr), currentAnimationIndex(-1), lastLoop(0)
    { }


    struct AnimationIndex
    {
        AnimationIndex() : index(0), timeOffset(0) {}
        // index points to the animation at timeOffset, skipping 0 duration animations.
        // Note that the index semantic is slightly different depending on the direction.
        int index; // the index of the animation in timeOffset
        int timeOffset; // time offset when the animation at index starts.
    };

    int animationActualTotalDuration(int index) const;
    AnimationIndex indexForCurrentTime() const;

    void setCurrentAnimation(int index, bool intermediate = false);
    void activateCurrentAnimation(bool intermediate = false);

    void animationInsertedAt(int index) override;
    void animationRemoved(int index, QAbstractAnimation *anim) override;

    bool atEnd() const;

    QAbstractAnimation *currentAnimation;
    int currentAnimationIndex;

    // this is the actual duration of uncontrolled animations
    // it helps seeking and even going forward
    QVector<int> actualDuration;

    void restart();
    int lastLoop;

    // handle time changes
    void rewindForwards(const AnimationIndex &newAnimationIndex);
    void advanceForwards(const AnimationIndex &newAnimationIndex);

    // private slot
    void _q_uncontrolledAnimationFinished();
};

QT_END_NAMESPACE

#endif //QSEQUENTIALANIMATIONGROUP_P_H
