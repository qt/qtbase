// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

/*!
    \class QParallelAnimationGroup
    \inmodule QtCore
    \brief The QParallelAnimationGroup class provides a parallel group of animations.
    \since 4.6
    \ingroup animation

    QParallelAnimationGroup--a \l{QAnimationGroup}{container for
    animations}--starts all its animations when it is
    \l{QAbstractAnimation::start()}{started} itself, i.e., runs all
    animations in parallel. The animation group finishes when the
    longest lasting animation has finished.

    You can treat QParallelAnimationGroup as any other QAbstractAnimation,
    e.g., pause, resume, or add it to other animation groups.

    \snippet code/src_corelib_animation_qparallelanimationgroup.cpp 0

    In this example, \c anim1 and \c anim2 are two
    \l{QPropertyAnimation}s that have already been set up.

    \sa QAnimationGroup, QPropertyAnimation, {The Animation Framework}
*/


#include "qparallelanimationgroup.h"
#include "qparallelanimationgroup_p.h"
//#define QANIMATION_DEBUG

QT_BEGIN_NAMESPACE

typedef QList<QAbstractAnimation *>::ConstIterator AnimationListConstIt;
typedef QHash<QAbstractAnimation*, int>::Iterator AnimationTimeHashIt;
typedef QHash<QAbstractAnimation*, int>::ConstIterator AnimationTimeHashConstIt;

/*!
    Constructs a QParallelAnimationGroup.
    \a parent is passed to QObject's constructor.
*/
QParallelAnimationGroup::QParallelAnimationGroup(QObject *parent)
    : QAnimationGroup(*new QParallelAnimationGroupPrivate, parent)
{
}

/*!
    \internal
*/
QParallelAnimationGroup::QParallelAnimationGroup(QParallelAnimationGroupPrivate &dd,
                                                 QObject *parent)
    : QAnimationGroup(dd, parent)
{
}

/*!
    Destroys the animation group. It will also destroy all its animations.
*/
QParallelAnimationGroup::~QParallelAnimationGroup()
{
}

/*!
    \reimp
*/
int QParallelAnimationGroup::duration() const
{
    Q_D(const QParallelAnimationGroup);
    int ret = 0;

    for (auto ani : std::as_const(d->animations)) {
        const int currentDuration = ani->totalDuration();
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret = qMax(ret, currentDuration);
    }

    return ret;
}

/*!
    \reimp
*/
void QParallelAnimationGroup::updateCurrentTime(int currentTime)
{
    Q_D(QParallelAnimationGroup);
    if (d->animations.isEmpty())
        return;

    if (d->currentLoop > d->lastLoop) {
        // simulate completion of the loop
        int dura = duration();
        if (dura > 0) {
            for (auto animation : std::as_const(d->animations)) {
                if (animation->state() != QAbstractAnimation::Stopped)
                    animation->setCurrentTime(dura);   // will stop
            }
        }
    } else if (d->currentLoop < d->lastLoop) {
        // simulate completion of the loop seeking backwards
        for (auto animation : std::as_const(d->animations)) {
            //we need to make sure the animation is in the right state
            //and then rewind it
            d->applyGroupState(animation);
            animation->setCurrentTime(0);
            animation->stop();
        }
    }

#ifdef QANIMATION_DEBUG
    qDebug("QParallellAnimationGroup %5d: setCurrentTime(%d), loop:%d, last:%d, timeFwd:%d, lastcurrent:%d, %d",
        __LINE__, d->currentTime, d->currentLoop, d->lastLoop, timeFwd, d->lastCurrentTime, state());
#endif
    // finally move into the actual time of the current loop
    for (auto animation : std::as_const(d->animations)) {
        const int dura = animation->totalDuration();
        //if the loopcount is bigger we should always start all animations
        if (d->currentLoop > d->lastLoop
            //if we're at the end of the animation, we need to start it if it wasn't already started in this loop
            //this happens in Backward direction where not all animations are started at the same time
            || d->shouldAnimationStart(animation, d->lastCurrentTime > dura /*startIfAtEnd*/)) {
            d->applyGroupState(animation);
        }

        if (animation->state() == state()) {
            animation->setCurrentTime(currentTime);
            if (dura > 0 && currentTime > dura)
                animation->stop();
        }
    }
    d->lastLoop = d->currentLoop;
    d->lastCurrentTime = currentTime;
}

/*!
    \reimp
*/
void QParallelAnimationGroup::updateState(QAbstractAnimation::State newState,
                                          QAbstractAnimation::State oldState)
{
    Q_D(QParallelAnimationGroup);
    QAnimationGroup::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (auto ani : std::as_const(d->animations))
            ani->stop();
        d->disconnectUncontrolledAnimations();
        break;
    case Paused:
        for (auto ani : std::as_const(d->animations)) {
            if (ani->state() == Running)
                ani->pause();
        }
        break;
    case Running:
        d->connectUncontrolledAnimations();
        for (auto animation : std::as_const(d->animations)) {
            if (oldState == Stopped)
                animation->stop();
            animation->setDirection(d->direction);
            if (d->shouldAnimationStart(animation, oldState == Stopped))
                animation->start();
        }
        break;
    }
}

void QParallelAnimationGroupPrivate::_q_uncontrolledAnimationFinished()
{
    Q_Q(QParallelAnimationGroup);

    QAbstractAnimation *animation = qobject_cast<QAbstractAnimation *>(q->sender());
    Q_ASSERT(animation);

    int uncontrolledRunningCount = 0;
    if (animation->duration() == -1 || animation->loopCount() < 0) {
        for (AnimationTimeHashIt it = uncontrolledFinishTime.begin(), cend = uncontrolledFinishTime.end(); it != cend; ++it) {
            if (it.key() == animation) {
                *it = animation->currentTime();
            }
            if (it.value() == -1)
                ++uncontrolledRunningCount;
        }
    }

    if (uncontrolledRunningCount > 0)
        return;

    int maxDuration = 0;
    for (auto ani : std::as_const(animations))
        maxDuration = qMax(maxDuration, ani->totalDuration());

    if (currentTime >= maxDuration)
        q->stop();
}

void QParallelAnimationGroupPrivate::disconnectUncontrolledAnimations()
{
    for (AnimationTimeHashConstIt it = uncontrolledFinishTime.constBegin(), cend = uncontrolledFinishTime.constEnd(); it != cend; ++it)
        disconnectUncontrolledAnimation(it.key());

    uncontrolledFinishTime.clear();
}

void QParallelAnimationGroupPrivate::connectUncontrolledAnimations()
{
    for (auto animation : std::as_const(animations)) {
        if (animation->duration() == -1 || animation->loopCount() < 0) {
            uncontrolledFinishTime[animation] = -1;
            connectUncontrolledAnimation(animation);
        }
    }
}

bool QParallelAnimationGroupPrivate::shouldAnimationStart(QAbstractAnimation *animation, bool startIfAtEnd) const
{
    const int dura = animation->totalDuration();
    if (dura == -1)
        return !isUncontrolledAnimationFinished(animation);
    if (startIfAtEnd)
        return currentTime <= dura;
    if (direction == QAbstractAnimation::Forward)
        return currentTime < dura;
    else //direction == QAbstractAnimation::Backward
        return currentTime && currentTime <= dura;
}

void QParallelAnimationGroupPrivate::applyGroupState(QAbstractAnimation *animation)
{
    switch (state)
    {
    case QAbstractAnimation::Running:
        animation->start();
        break;
    case QAbstractAnimation::Paused:
        animation->pause();
        break;
    case QAbstractAnimation::Stopped:
    default:
        break;
    }
}


bool QParallelAnimationGroupPrivate::isUncontrolledAnimationFinished(QAbstractAnimation *anim) const
{
    return uncontrolledFinishTime.value(anim, -1) >= 0;
}

void QParallelAnimationGroupPrivate::animationRemoved(qsizetype index, QAbstractAnimation *anim)
{
    QAnimationGroupPrivate::animationRemoved(index, anim);
    disconnectUncontrolledAnimation(anim);
    uncontrolledFinishTime.remove(anim);
}

/*!
    \reimp
*/
void QParallelAnimationGroup::updateDirection(QAbstractAnimation::Direction direction)
{
    Q_D(QParallelAnimationGroup);
    //we need to update the direction of the current animation
    if (state() != Stopped) {
        for (auto ani : std::as_const(d->animations))
            ani->setDirection(direction);
    } else {
        if (direction == Forward) {
            d->lastLoop = 0;
            d->lastCurrentTime = 0;
        } else {
            // Looping backwards with loopCount == -1 does not really work well...
            d->lastLoop = (d->loopCount == -1 ? 0 : d->loopCount - 1);
            d->lastCurrentTime = duration();
        }
    }
}

/*!
    \reimp
*/
bool QParallelAnimationGroup::event(QEvent *event)
{
    return QAnimationGroup::event(event);
}

QT_END_NAMESPACE

#include "moc_qparallelanimationgroup.cpp"
