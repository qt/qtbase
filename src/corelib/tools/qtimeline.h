/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QTIMELINE_H
#define QTIMELINE_H

#include <QtCore/qglobal.h>

QT_REQUIRE_CONFIG(easingcurve);

#include <QtCore/qeasingcurve.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QTimeLinePrivate;
class Q_CORE_EXPORT QTimeLine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int duration READ duration WRITE setDuration BINDABLE bindableDuration)
    Q_PROPERTY(int updateInterval READ updateInterval WRITE setUpdateInterval
               BINDABLE bindableUpdateInterval)
    Q_PROPERTY(int currentTime READ currentTime WRITE setCurrentTime BINDABLE bindableCurrentTime)
    Q_PROPERTY(Direction direction READ direction WRITE setDirection BINDABLE bindableDirection)
    Q_PROPERTY(int loopCount READ loopCount WRITE setLoopCount BINDABLE bindableLoopCount)
    Q_PROPERTY(QEasingCurve easingCurve READ easingCurve WRITE setEasingCurve
               BINDABLE bindableEasingCurve)
public:
    enum State {
        NotRunning,
        Paused,
        Running
    };
    enum Direction {
        Forward,
        Backward
    };

    explicit QTimeLine(int duration = 1000, QObject *parent = nullptr);
    virtual ~QTimeLine();

    State state() const;

    int loopCount() const;
    void setLoopCount(int count);
    QBindable<int> bindableLoopCount();

    Direction direction() const;
    void setDirection(Direction direction);
    QBindable<Direction> bindableDirection();

    int duration() const;
    void setDuration(int duration);
    QBindable<int> bindableDuration();

    int startFrame() const;
    void setStartFrame(int frame);
    int endFrame() const;
    void setEndFrame(int frame);
    void setFrameRange(int startFrame, int endFrame);

    int updateInterval() const;
    void setUpdateInterval(int interval);
    QBindable<int> bindableUpdateInterval();

    QEasingCurve easingCurve() const;
    void setEasingCurve(const QEasingCurve &curve);
    QBindable<QEasingCurve> bindableEasingCurve();

    int currentTime() const;
    QBindable<int> bindableCurrentTime();
    int currentFrame() const;
    qreal currentValue() const;

    int frameForTime(int msec) const;
    virtual qreal valueForTime(int msec) const;

public Q_SLOTS:
    void start();
    void resume();
    void stop();
    void setPaused(bool paused);
    void setCurrentTime(int msec);
    void toggleDirection();

Q_SIGNALS:
    void valueChanged(qreal x, QPrivateSignal);
    void frameChanged(int, QPrivateSignal);
    void stateChanged(QTimeLine::State newState, QPrivateSignal);
    void finished(QPrivateSignal);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    Q_DISABLE_COPY(QTimeLine)
    Q_DECLARE_PRIVATE(QTimeLine)
};

QT_END_NAMESPACE

#endif

