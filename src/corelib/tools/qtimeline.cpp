// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimeline.h"

#include <private/qproperty_p.h>
#include <private/qobject_p.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qmath.h>
#include <QtCore/qelapsedtimer.h>

QT_BEGIN_NAMESPACE

class QTimeLinePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QTimeLine)
public:
    QElapsedTimer timer;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimeLinePrivate, QEasingCurve, easingCurve,
                                         QEasingCurve::InOutSine)

    int startTime = 0;
    void setDuration(int duration) { q_func()->setDuration(duration); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QTimeLinePrivate, int, duration,
                                       &QTimeLinePrivate::setDuration, 1000)
    int startFrame = 0;
    int endFrame = 0;
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimeLinePrivate, int, updateInterval, 1000 / 25)
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(QTimeLinePrivate, int, loopCount, 1)
    int currentLoopCount = 0;

    void setCurrentTimeForwardToQ(int time) { q_func()->setCurrentTime(time); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QTimeLinePrivate, int, currentTime,
                                       &QTimeLinePrivate::setCurrentTimeForwardToQ, 0)
    int timerId = 0;

    void setDirection(QTimeLine::Direction direction) { q_func()->setDirection(direction); }
    Q_OBJECT_COMPAT_PROPERTY_WITH_ARGS(QTimeLinePrivate, QTimeLine::Direction, direction,
                                       &QTimeLinePrivate::setDirection, QTimeLine::Forward)
    QTimeLine::State state = QTimeLine::NotRunning;
    inline void setState(QTimeLine::State newState)
    {
        Q_Q(QTimeLine);
        if (newState != state)
            emit q->stateChanged(state = newState, QTimeLine::QPrivateSignal());
    }

    void setCurrentTime(int msecs);
};

/*!
    \internal
*/
void QTimeLinePrivate::setCurrentTime(int msecs)
{
    Q_Q(QTimeLine);
    currentTime.removeBindingUnlessInWrapper();
    const auto previousCurrentTime = currentTime.valueBypassingBindings();

    const qreal lastValue = q->valueForTime(previousCurrentTime);
    const int lastFrame = q->frameForTime(previousCurrentTime);

    // Determine if we are looping.
    const int elapsed = (direction == QTimeLine::Backward) ? (-msecs + duration) : msecs;
    const int loopCountNow = elapsed / duration;

    const bool looping = (loopCountNow != currentLoopCount);
#ifdef QTIMELINE_DEBUG
    qDebug() << "QTimeLinePrivate::setCurrentTime:" << msecs << duration << "with loopCountNow"
             << loopCountNow << "currentLoopCount" << currentLoopCount << "looping" << looping;
#endif
    if (looping)
        currentLoopCount = loopCountNow;

    // Normalize msecs to be between 0 and duration, inclusive.
    currentTime.setValueBypassingBindings(elapsed % duration);
    if (direction.value() == QTimeLine::Backward)
        currentTime.setValueBypassingBindings(duration - currentTime.valueBypassingBindings());

    // Check if we have reached the end of loopcount.
    bool finished = false;
    if (loopCount && currentLoopCount >= loopCount) {
        finished = true;
        currentTime.setValueBypassingBindings((direction == QTimeLine::Backward) ? 0 : duration);
        currentLoopCount = loopCount - 1;
    }

    const int currentFrame = q->frameForTime(currentTime.valueBypassingBindings());
#ifdef QTIMELINE_DEBUG
    qDebug() << "QTimeLinePrivate::setCurrentTime: frameForTime"
             << currentTime.valueBypassingBindings() << currentFrame;
#endif
    const qreal currentValue = q->valueForTime(currentTime.valueBypassingBindings());
    if (!qFuzzyCompare(lastValue, currentValue))
        emit q->valueChanged(currentValue, QTimeLine::QPrivateSignal());
    if (lastFrame != currentFrame) {
        const int transitionframe = (direction == QTimeLine::Forward ? endFrame : startFrame);
        if (looping && !finished && transitionframe != currentFrame) {
#ifdef QTIMELINE_DEBUG
            qDebug("QTimeLinePrivate::setCurrentTime: transitionframe");
#endif
            emit q->frameChanged(transitionframe, QTimeLine::QPrivateSignal());
        }
#ifdef QTIMELINE_DEBUG
        else {
            QByteArray reason;
            if (!looping)
                reason += " not looping";
            if (finished) {
                if (!reason.isEmpty())
                    reason += " and";
                reason += " finished";
            }
            if (transitionframe == currentFrame) {
                if (!reason.isEmpty())
                    reason += " and";
                reason += " transitionframe is equal to currentFrame: " + QByteArray::number(currentFrame);
            }
            qDebug("QTimeLinePrivate::setCurrentTime: not transitionframe because %s",  reason.constData());
        }
#endif
        emit q->frameChanged(currentFrame, QTimeLine::QPrivateSignal());
    }
    if (finished && state == QTimeLine::Running) {
        q->stop();
        emit q->finished(QTimeLine::QPrivateSignal());
    }
    if (currentTime.valueBypassingBindings() != previousCurrentTime)
        currentTime.notify();
}
QBindable<int> QTimeLine::bindableCurrentTime()
{
    Q_D(QTimeLine);
    return &d->currentTime;
}

/*!
    \class QTimeLine
    \inmodule QtCore
    \brief The QTimeLine class provides a timeline for controlling animations.
    \since 4.2
    \ingroup animation

    It's most commonly used to animate a GUI control by calling a slot
    periodically. You can construct a timeline by passing its duration in
    milliseconds to QTimeLine's constructor. The timeline's duration describes
    for how long the animation will run. Then you set a suitable frame range
    by calling setFrameRange(). Finally connect the frameChanged() signal to a
    suitable slot in the widget you wish to animate (for example, \l {QProgressBar::}{setValue()}
    in QProgressBar). When you proceed to calling start(), QTimeLine will enter
    Running state, and start emitting frameChanged() at regular intervals,
    causing your widget's connected property's value to grow from the lower
    end to the upper and of your frame range, at a steady rate. You can
    specify the update interval by calling setUpdateInterval(). When done,
    QTimeLine enters NotRunning state, and emits finished().

    Example:

    \snippet code/src_corelib_tools_qtimeline.cpp 0

    By default the timeline runs once, from its beginning to its end,
    upon which you must call start() again to restart from the beginning. To
    make the timeline loop, you can call setLoopCount(), passing the number of
    times the timeline should run before finishing. The direction can also be
    changed, causing the timeline to run backward, by calling
    setDirection(). You can also pause and unpause the timeline while it's
    running by calling setPaused(). For interactive control, the
    setCurrentTime() function is provided, which sets the time position of the
    time line directly. Although most useful in NotRunning state (e.g.,
    connected to a valueChanged() signal in a QSlider), this function can be
    called at any time.

    The frame interface is useful for standard widgets, but QTimeLine can be
    used to control any type of animation. The heart of QTimeLine lies in the
    valueForTime() function, which generates a \e value between 0 and 1 for a
    given time. This value is typically used to describe the steps of an
    animation, where 0 is the first step of an animation, and 1 is the last
    step. When running, QTimeLine generates values between 0 and 1 by calling
    valueForTime() and emitting valueChanged(). By default, valueForTime()
    applies an interpolation algorithm to generate these value. You can choose
    from a set of predefined timeline algorithms by calling setEasingCurve().

    Note that, by default, QTimeLine uses QEasingCurve::InOutSine, which
    provides a value that grows slowly, then grows steadily, and finally grows
    slowly. For a custom timeline, you can reimplement valueForTime(), in which
    case QTimeLine's easingCurve property is ignored.

    \sa QProgressBar, QProgressDialog
*/

/*!
    \enum QTimeLine::State

    This enum describes the state of the timeline.

    \value NotRunning The timeline is not running. This is the initial state
    of QTimeLine, and the state QTimeLine reenters when finished. The current
    time, frame and value remain unchanged until either setCurrentTime() is
    called, or the timeline is started by calling start().

    \value Paused The timeline is paused (i.e., temporarily
    suspended). Calling setPaused(false) will resume timeline activity.

    \value Running The timeline is running. While control is in the event
    loop, QTimeLine will update its current time at regular intervals,
    emitting valueChanged() and frameChanged() when appropriate.

    \sa state(), stateChanged()
*/

/*!
    \enum QTimeLine::Direction

    This enum describes the direction of the timeline when in \l Running state.

    \value Forward The current time of the timeline increases with time (i.e.,
    moves from 0 and towards the end / duration).

    \value Backward The current time of the timeline decreases with time (i.e.,
    moves from the end / duration and towards 0).

    \sa setDirection()
*/

/*!
    \fn void QTimeLine::valueChanged(qreal value)

    QTimeLine emits this signal at regular intervals when in \l Running state,
    but only if the current value changes. \a value is the current value. \a value is
    a number between 0.0 and 1.0

    \sa QTimeLine::setDuration(), QTimeLine::valueForTime(), QTimeLine::updateInterval
*/

/*!
    \fn void QTimeLine::frameChanged(int frame)

    QTimeLine emits this signal at regular intervals when in \l Running state,
    but only if the current frame changes. \a frame is the current frame number.

    \sa QTimeLine::setFrameRange(), QTimeLine::updateInterval
*/

/*!
    \fn void QTimeLine::stateChanged(QTimeLine::State newState)

    This signal is emitted whenever QTimeLine's state changes. The new state
    is \a newState.
*/

/*!
    \fn void QTimeLine::finished()

    This signal is emitted when QTimeLine finishes (i.e., reaches the end of
    its time line), and does not loop.
*/

/*!
    Constructs a timeline with a duration of \a duration milliseconds. \a
    parent is passed to QObject's constructor. The default duration is 1000
    milliseconds.
 */
QTimeLine::QTimeLine(int duration, QObject *parent)
    : QObject(*new QTimeLinePrivate, parent)
{
    setDuration(duration);
}

/*!
    Destroys the timeline.
 */
QTimeLine::~QTimeLine()
{
    Q_D(QTimeLine);

    if (d->state == Running)
        stop();
}

/*!
    Returns the state of the timeline.

    \sa start(), setPaused(), stop()
*/
QTimeLine::State QTimeLine::state() const
{
    Q_D(const QTimeLine);
    return d->state;
}

/*!
    \property QTimeLine::loopCount
    \brief the number of times the timeline should loop before it's finished.

    A loop count of 0 means that the timeline will loop forever.

    By default, this property contains a value of 1.
*/
int QTimeLine::loopCount() const
{
    Q_D(const QTimeLine);
    return d->loopCount;
}

void QTimeLine::setLoopCount(int count)
{
    Q_D(QTimeLine);
    d->loopCount = count;
}

QBindable<int> QTimeLine::bindableLoopCount()
{
    Q_D(QTimeLine);
    return &d->loopCount;
}

/*!
    \property QTimeLine::direction
    \brief the direction of the timeline when QTimeLine is in \l Running
    state.

    This direction indicates whether the time moves from 0 towards the
    timeline duration, or from the value of the duration and towards 0 after
    start() has been called.

    Any binding of direction will be removed not only by setDirection(),
    but also by toggleDirection().

    By default, this property is set to \l Forward.
*/
QTimeLine::Direction QTimeLine::direction() const
{
    Q_D(const QTimeLine);
    return d->direction;
}
void QTimeLine::setDirection(Direction direction)
{
    Q_D(QTimeLine);
    d->direction.removeBindingUnlessInWrapper();
    const auto previousDirection = d->direction.valueBypassingBindings();
    d->direction.setValueBypassingBindings(direction);
    d->startTime = d->currentTime;
    d->timer.start();
    if (previousDirection != d->direction.valueBypassingBindings())
        d->direction.notify();
}

QBindable<QTimeLine::Direction> QTimeLine::bindableDirection()
{
    Q_D(QTimeLine);
    return &d->direction;
}

/*!
    \property QTimeLine::duration
    \brief the total duration of the timeline in milliseconds.

    By default, this value is 1000 (i.e., 1 second), but you can change this
    by either passing a duration to QTimeLine's constructor, or by calling
    setDuration(). The duration must be larger than 0.

    \note Changing the duration does not cause the current time to be reset
    to zero or the new duration. You also need to call setCurrentTime() with
    the desired value.
*/
int QTimeLine::duration() const
{
    Q_D(const QTimeLine);
    return d->duration;
}
void QTimeLine::setDuration(int duration)
{
    Q_D(QTimeLine);
    if (duration <= 0) {
        qWarning("QTimeLine::setDuration: cannot set duration <= 0");
        return;
    }
    d->duration.removeBindingUnlessInWrapper();
    if (duration != d->duration.valueBypassingBindings()) {
        d->duration.setValueBypassingBindings(duration);
        d->duration.notify();
    }
}

QBindable<int> QTimeLine::bindableDuration()
{
    Q_D(QTimeLine);
    return &d->duration;
}

/*!
    Returns the start frame, which is the frame corresponding to the start of
    the timeline (i.e., the frame for which the current value is 0).

    \sa setStartFrame(), setFrameRange()
*/
int QTimeLine::startFrame() const
{
    Q_D(const QTimeLine);
    return d->startFrame;
}

/*!
    Sets the start frame, which is the frame corresponding to the start of the
    timeline (i.e., the frame for which the current value is 0), to \a frame.

    \sa startFrame(), endFrame(), setFrameRange()
*/
void QTimeLine::setStartFrame(int frame)
{
    Q_D(QTimeLine);
    d->startFrame = frame;
}

/*!
    Returns the end frame, which is the frame corresponding to the end of the
    timeline (i.e., the frame for which the current value is 1).

    \sa setEndFrame(), setFrameRange()
*/
int QTimeLine::endFrame() const
{
    Q_D(const QTimeLine);
    return d->endFrame;
}

/*!
    Sets the end frame, which is the frame corresponding to the end of the
    timeline (i.e., the frame for which the current value is 1), to \a frame.

    \sa endFrame(), startFrame(), setFrameRange()
*/
void QTimeLine::setEndFrame(int frame)
{
    Q_D(QTimeLine);
    d->endFrame = frame;
}

/*!
    Sets the timeline's frame counter to start at \a startFrame, and end and
    \a endFrame. For each time value, QTimeLine will find the corresponding
    frame when you call currentFrame() or frameForTime() by interpolating,
    using the return value of valueForTime().

    When in Running state, QTimeLine also emits the frameChanged() signal when
    the frame changes.

    \sa startFrame(), endFrame(), start(), currentFrame()
*/
void QTimeLine::setFrameRange(int startFrame, int endFrame)
{
    Q_D(QTimeLine);
    d->startFrame = startFrame;
    d->endFrame = endFrame;
}

/*!
    \property QTimeLine::updateInterval
    \brief the time in milliseconds between each time QTimeLine updates its
    current time.

    When updating the current time, QTimeLine will emit valueChanged() if the
    current value changed, and frameChanged() if the frame changed.

    By default, the interval is 40 ms, which corresponds to a rate of 25
    updates per second.
*/
int QTimeLine::updateInterval() const
{
    Q_D(const QTimeLine);
    return d->updateInterval;
}
void QTimeLine::setUpdateInterval(int interval)
{
    Q_D(QTimeLine);
    d->updateInterval = interval;
}
QBindable<int> QTimeLine::bindableUpdateInterval()
{
    Q_D(QTimeLine);
    return &d->updateInterval;
}

/*!
    \property QTimeLine::easingCurve

    \since 4.6

    Specifies the easing curve that the timeline will use.
    If valueForTime() is reimplemented, this value is ignored.

    \sa valueForTime()
*/

QEasingCurve QTimeLine::easingCurve() const
{
    Q_D(const QTimeLine);
    return d->easingCurve;
}

void QTimeLine::setEasingCurve(const QEasingCurve &curve)
{
    Q_D(QTimeLine);
    d->easingCurve = curve;
}

QBindable<QEasingCurve> QTimeLine::bindableEasingCurve()
{
    Q_D(QTimeLine);
    return &d->easingCurve;
}

/*!
    \property QTimeLine::currentTime
    \brief the current time of the time line.

    When QTimeLine is in Running state, this value is updated continuously as
    a function of the duration and direction of the timeline. Otherwise, it is
    value that was current when stop() was called last, or the value set by
    setCurrentTime().

    \note You can bind other properties to currentTime, but it is not
    recommended setting bindings to it. As animation progresses, the currentTime
    is updated automatically, which cancels its bindings.

    By default, this property contains a value of 0.
*/
int QTimeLine::currentTime() const
{
    Q_D(const QTimeLine);
    return d->currentTime;
}
void QTimeLine::setCurrentTime(int msec)
{
    Q_D(QTimeLine);
    d->startTime = 0;
    d->currentLoopCount = 0;
    d->timer.restart();
    d->setCurrentTime(msec);
}

/*!
    Returns the frame corresponding to the current time.

    \sa currentTime(), frameForTime(), setFrameRange()
*/
int QTimeLine::currentFrame() const
{
    Q_D(const QTimeLine);
    return frameForTime(d->currentTime);
}

/*!
    Returns the value corresponding to the current time.

    \sa valueForTime(), currentFrame()
*/
qreal QTimeLine::currentValue() const
{
    Q_D(const QTimeLine);
    return valueForTime(d->currentTime);
}

/*!
    Returns the frame corresponding to the time \a msec. This value is
    calculated using a linear interpolation of the start and end frame, based
    on the value returned by valueForTime().

    \sa valueForTime(), setFrameRange()
*/
int QTimeLine::frameForTime(int msec) const
{
    Q_D(const QTimeLine);
    if (d->direction == Forward)
        return d->startFrame + int((d->endFrame - d->startFrame) * valueForTime(msec));
    return d->startFrame + qCeil((d->endFrame - d->startFrame) * valueForTime(msec));
}

/*!
    Returns the timeline value for the time \a msec. The returned value, which
    varies depending on the curve shape, is always between 0 and 1. If \a msec
    is 0, the default implementation always returns 0.

    Reimplement this function to provide a custom curve shape for your
    timeline.

    \sa easingCurve, frameForTime()
*/
qreal QTimeLine::valueForTime(int msec) const
{
    Q_D(const QTimeLine);
    msec = qBound(0, msec, d->duration.value());

    qreal value = msec / qreal(d->duration.value());
    return d->easingCurve.value().valueForProgress(value);
}

/*!
    Starts the timeline. QTimeLine will enter Running state, and once it
    enters the event loop, it will update its current time, frame and value at
    regular intervals. The default interval is 40 ms (i.e., 25 times per
    second). You can change the update interval by calling
    setUpdateInterval().

    The timeline will start from position 0, or the end if going backward.
    If you want to resume a stopped timeline without restarting, you can call
    resume() instead.

    \sa resume(), updateInterval(), frameChanged(), valueChanged()
*/
void QTimeLine::start()
{
    Q_D(QTimeLine);
    if (d->timerId) {
        qWarning("QTimeLine::start: already running");
        return;
    }
    int curTime = 0;
    if (d->direction == Backward)
        curTime = d->duration;
    d->timerId = startTimer(d->updateInterval);
    d->startTime = curTime;
    d->currentLoopCount = 0;
    d->timer.start();
    d->setState(Running);
    d->setCurrentTime(curTime);
}

/*!
    Resumes the timeline from the current time. QTimeLine will reenter Running
    state, and once it enters the event loop, it will update its current time,
    frame and value at regular intervals.

    In contrast to start(), this function does not restart the timeline before
    it resumes.

    \sa start(), updateInterval(), frameChanged(), valueChanged()
*/
void QTimeLine::resume()
{
    Q_D(QTimeLine);
    if (d->timerId) {
        qWarning("QTimeLine::resume: already running");
        return;
    }
    d->timerId = startTimer(d->updateInterval);
    d->startTime = d->currentTime;
    d->timer.start();
    d->setState(Running);
}

/*!
    Stops the timeline, causing QTimeLine to enter NotRunning state.

    \sa start()
*/
void QTimeLine::stop()
{
    Q_D(QTimeLine);
    if (d->timerId)
        killTimer(d->timerId);
    d->setState(NotRunning);
    d->timerId = 0;
}

/*!
    If \a paused is true, the timeline is paused, causing QTimeLine to enter
    Paused state. No updates will be signaled until either start() or
    setPaused(false) is called. If \a paused is false, the timeline is resumed
    and continues where it left.

    \sa state(), start()
*/
void QTimeLine::setPaused(bool paused)
{
    Q_D(QTimeLine);
    if (d->state == NotRunning) {
        qWarning("QTimeLine::setPaused: Not running");
        return;
    }
    if (paused && d->state != Paused) {
        d->startTime = d->currentTime;
        killTimer(d->timerId);
        d->timerId = 0;
        d->setState(Paused);
    } else if (!paused && d->state == Paused) {
        // Same as resume()
        d->timerId = startTimer(d->updateInterval);
        d->startTime = d->currentTime;
        d->timer.start();
        d->setState(Running);
    }
}

/*!
    Toggles the direction of the timeline. If the direction was Forward, it
    becomes Backward, and vice verca.

    Existing bindings of \l direction are removed.

    \sa setDirection()
*/
void QTimeLine::toggleDirection()
{
    Q_D(QTimeLine);
    setDirection(d->direction == Forward ? Backward : Forward);
}

/*!
    \reimp
*/
void QTimeLine::timerEvent(QTimerEvent *event)
{
    Q_D(QTimeLine);
    if (event->timerId() != d->timerId) {
        event->ignore();
        return;
    }
    event->accept();

    if (d->direction == Forward) {
        d->setCurrentTime(d->startTime + d->timer.elapsed());
    } else {
        d->setCurrentTime(d->startTime - d->timer.elapsed());
    }
}

QT_END_NAMESPACE

#include "moc_qtimeline.cpp"
