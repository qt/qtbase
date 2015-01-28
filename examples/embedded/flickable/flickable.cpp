/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "flickable.h"

#include <QtCore>
#include <QtWidgets>

class FlickableTicker: QObject
{
public:
    FlickableTicker(Flickable *scroller) {
        m_scroller = scroller;
    }

    void start(int interval) {
        if (!m_timer.isActive())
            m_timer.start(interval, this);
    }

    void stop() {
        m_timer.stop();
    }

protected:
    void timerEvent(QTimerEvent *event) {
        Q_UNUSED(event);
        m_scroller->tick();
    }

private:
    Flickable *m_scroller;
    QBasicTimer m_timer;
};

class FlickablePrivate
{
public:
    typedef enum {
        Steady,
        Pressed,
        ManualScroll,
        AutoScroll,
        Stop
    } State;

    State state;
    int threshold;
    QPoint pressPos;
    QPoint offset;
    QPoint delta;
    QPoint speed;
    FlickableTicker *ticker;
    QTime timeStamp;
    QWidget *target;
    QList<QEvent*> ignoreList;
};

Flickable::Flickable()
{
    d = new FlickablePrivate;
    d->state = FlickablePrivate::Steady;
    d->threshold = 10;
    d->ticker = new FlickableTicker(this);
    d->timeStamp = QTime::currentTime();
    d->target = 0;
}

Flickable::~Flickable()
{
    delete d;
}

void Flickable::setThreshold(int th)
{
    if (th >= 0)
        d->threshold = th;
}

int Flickable::threshold() const
{
    return d->threshold;
}

void Flickable::setAcceptMouseClick(QWidget *target)
{
    d->target = target;
}

static QPoint deaccelerate(const QPoint &speed, int a = 1, int max = 64)
{
    int x = qBound(-max, speed.x(), max);
    int y = qBound(-max, speed.y(), max);
    x = (x == 0) ? x : (x > 0) ? qMax(0, x - a) : qMin(0, x + a);
    y = (y == 0) ? y : (y > 0) ? qMax(0, y - a) : qMin(0, y + a);
    return QPoint(x, y);
}

void Flickable::handleMousePress(QMouseEvent *event)
{
    event->ignore();

    if (event->button() != Qt::LeftButton)
        return;

    if (d->ignoreList.removeAll(event))
        return;

    switch (d->state) {

    case FlickablePrivate::Steady:
        event->accept();
        d->state = FlickablePrivate::Pressed;
        d->pressPos = event->pos();
        break;

    case FlickablePrivate::AutoScroll:
        event->accept();
        d->state = FlickablePrivate::Stop;
        d->speed = QPoint(0, 0);
        d->pressPos = event->pos();
        d->offset = scrollOffset();
        d->ticker->stop();
        break;

    default:
        break;
    }
}

void Flickable::handleMouseRelease(QMouseEvent *event)
{
    event->ignore();

    if (event->button() != Qt::LeftButton)
        return;

    if (d->ignoreList.removeAll(event))
        return;

    QPoint delta;

    switch (d->state) {

    case FlickablePrivate::Pressed:
        event->accept();
        d->state = FlickablePrivate::Steady;
        if (d->target) {
            QMouseEvent *event1 = new QMouseEvent(QEvent::MouseButtonPress,
                                                  d->pressPos, Qt::LeftButton,
                                                  Qt::LeftButton, Qt::NoModifier);
            QMouseEvent *event2 = new QMouseEvent(*event);
            d->ignoreList << event1;
            d->ignoreList << event2;
            QApplication::postEvent(d->target, event1);
            QApplication::postEvent(d->target, event2);
        }
        break;

    case FlickablePrivate::ManualScroll:
        event->accept();
        delta = event->pos() - d->pressPos;
        if (d->timeStamp.elapsed() > 100) {
            d->timeStamp = QTime::currentTime();
            d->speed = delta - d->delta;
            d->delta = delta;
        }
        d->offset = scrollOffset();
        d->pressPos = event->pos();
        if (d->speed == QPoint(0, 0)) {
            d->state = FlickablePrivate::Steady;
        } else {
            d->speed /= 4;
            d->state = FlickablePrivate::AutoScroll;
            d->ticker->start(20);
        }
        break;

    case FlickablePrivate::Stop:
        event->accept();
        d->state = FlickablePrivate::Steady;
        d->offset = scrollOffset();
        break;

    default:
        break;
    }
}

void Flickable::handleMouseMove(QMouseEvent *event)
{
    event->ignore();

    if (!(event->buttons() & Qt::LeftButton))
        return;

    if (d->ignoreList.removeAll(event))
        return;

    QPoint delta;

    switch (d->state) {

    case FlickablePrivate::Pressed:
    case FlickablePrivate::Stop:
        delta = event->pos() - d->pressPos;
        if (delta.x() > d->threshold || delta.x() < -d->threshold ||
                delta.y() > d->threshold || delta.y() < -d->threshold) {
            d->timeStamp = QTime::currentTime();
            d->state = FlickablePrivate::ManualScroll;
            d->delta = QPoint(0, 0);
            d->pressPos = event->pos();
            event->accept();
        }
        break;

    case FlickablePrivate::ManualScroll:
        event->accept();
        delta = event->pos() - d->pressPos;
        setScrollOffset(d->offset - delta);
        if (d->timeStamp.elapsed() > 100) {
            d->timeStamp = QTime::currentTime();
            d->speed = delta - d->delta;
            d->delta = delta;
        }
        break;

    default:
        break;
    }
}

void Flickable::tick()
{
    if (d->state == FlickablePrivate:: AutoScroll) {
        d->speed = deaccelerate(d->speed);
        setScrollOffset(d->offset - d->speed);
        d->offset = scrollOffset();
        if (d->speed == QPoint(0, 0)) {
            d->state = FlickablePrivate::Steady;
            d->ticker->stop();
        }
    } else {
        d->ticker->stop();
    }
}
