/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    \code
        QParallelAnimationGroup *group = new QParallelAnimationGroup;
        group->addAnimation(anim1);
        group->addAnimation(anim2);

        group->start();
    \endcode

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

    for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it) {
        const int currentDuration = (*it)->totalDuration();
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
            for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it) {
                QAbstractAnimation *animation = (*it);
                if (animation->state() != QAbstractAnimation::Stopped)
                    animation->setCurrentTime(dura);   // will stop
            }
        }
    } else if (d->currentLoop < d->lastLoop) {
        // simulate completion of the loop seeking backwards
        for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it) {
            QAbstractAnimation *animation = *it;
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
    for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it) {
        QAbstractAnimation *animation = *it;
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
        for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it)
            (*it)->stop();
        d->disconnectUncontrolledAnimations();
        break;
    case Paused:
        for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it) {
            if ((*it)->state() == Running)
                (*it)->pause();
        }
        break;
    case Running:
        d->connectUncontrolledAnimations();
        for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it) {
            QAbstractAnimation *animation = *it;
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
    for (AnimationListConstIt it = animations.constBegin(), cend = animations.constEnd(); it != cend; ++it)
        maxDuration = qMax(maxDuration, (*it)->totalDuration());

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
    for (AnimationListConstIt it = animations.constBegin(), cend = animations.constEnd(); it != cend; ++it) {
        QAbstractAnimation *animation = *it;
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

void QParallelAnimationGroupPrivate::animationRemoved(int index, QAbstractAnimation *anim)
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
        for (AnimationListConstIt it = d->animations.constBegin(), cend = d->animations.constEnd(); it != cend; ++it)
            (*it)->setDirection(direction);
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
