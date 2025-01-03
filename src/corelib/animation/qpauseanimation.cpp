// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

/*!
    \class QPauseAnimation
    \inmodule QtCore
    \brief The QPauseAnimation class provides a pause for QSequentialAnimationGroup.
    \since 4.6
    \ingroup animation

    If you wish to introduce a delay between animations in a
    QSequentialAnimationGroup, you can insert a QPauseAnimation. This
    class does not animate anything, but does not
    \l{QAbstractAnimation::finished()}{finish} before a specified
    number of milliseconds have elapsed from when it was started. You
    specify the duration of the pause in the constructor. It can also
    be set directly with setDuration().

    It is not necessary to construct a QPauseAnimation yourself.
    QSequentialAnimationGroup provides the convenience functions
    \l{QSequentialAnimationGroup::}{addPause()} and
    \l{QSequentialAnimationGroup::}{insertPause()}. These functions
    simply take the number of milliseconds the pause should last.

    \sa QSequentialAnimationGroup
*/

#include "qpauseanimation.h"
#include "qabstractanimation_p.h"
#include "private/qproperty_p.h"

QT_BEGIN_NAMESPACE

class QPauseAnimationPrivate : public QAbstractAnimationPrivate
{
    Q_DECLARE_PUBLIC(QPauseAnimation)
public:
    QPauseAnimationPrivate() : QAbstractAnimationPrivate(), duration(250)
    {
        isPause = true;
    }

    void setDuration(int msecs) { q_func()->setDuration(msecs); }
    Q_OBJECT_COMPAT_PROPERTY(QPauseAnimationPrivate, int, duration,
                             &QPauseAnimationPrivate::setDuration)
};

/*!
    Constructs a QPauseAnimation.
    \a parent is passed to QObject's constructor.
    The default duration is 0.
*/

QPauseAnimation::QPauseAnimation(QObject *parent) : QAbstractAnimation(*new QPauseAnimationPrivate, parent)
{
}

/*!
    Constructs a QPauseAnimation.
    \a msecs is the duration of the pause.
    \a parent is passed to QObject's constructor.
*/

QPauseAnimation::QPauseAnimation(int msecs, QObject *parent) : QAbstractAnimation(*new QPauseAnimationPrivate, parent)
{
    setDuration(msecs);
}

/*!
    Destroys the pause animation.
*/
QPauseAnimation::~QPauseAnimation()
{
}

/*!
    \property QPauseAnimation::duration
    \brief the duration of the pause.

    The duration of the pause. The duration should not be negative.
    The default duration is 250 milliseconds.
*/
int QPauseAnimation::duration() const
{
    Q_D(const QPauseAnimation);
    return d->duration;
}

void QPauseAnimation::setDuration(int msecs)
{
    if (msecs < 0) {
        qWarning("QPauseAnimation::setDuration: cannot set a negative duration");
        return;
    }
    Q_D(QPauseAnimation);

    d->duration.removeBindingUnlessInWrapper();
    if (msecs != d->duration.valueBypassingBindings()) {
        d->duration.setValueBypassingBindings(msecs);
        d->duration.notify();
    }
}

QBindable<int> QPauseAnimation::bindableDuration()
{
    Q_D(QPauseAnimation);
    return &d->duration;
}

/*!
    \reimp
 */
bool QPauseAnimation::event(QEvent *e)
{
    return QAbstractAnimation::event(e);
}

/*!
    \reimp
 */
void QPauseAnimation::updateCurrentTime(int)
{
}


QT_END_NAMESPACE

#include "moc_qpauseanimation.cpp"
