/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QScrollBar>
#include <QEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QDebug>

#include "scroller.h"
#include "scroller_p.h"
#include "abstractscrollarea.h"
#include "scrollbar.h"

const int ScrollStep = 1;
const int UpdateScrollingInterval = 55;
const int UpdateScrollingSmoothInterval = 0;
static const qreal MaxScrollingSpeed = 48.0;

ScrollerPrivate::ScrollerPrivate(Scroller *scroller)
    : m_scrollArea(0)
    , m_scrollFactor(1.0)
    , m_state(Stopped)
    , q_ptr(scroller)
    , m_eventViewport(0)
{
}

ScrollerPrivate::~ScrollerPrivate()
{
}

void ScrollerPrivate::stopScrolling()
{
    m_state = ScrollerPrivate::Started;
    m_cursorPos = QCursor::pos();
    m_speed = QPoint(0, 0);

    if (m_scrollTimer.isActive())
        m_scrollTimer.stop();
}

//Maps screen coordinates to scrollArea coordinates though current m_eventViewport widget
QPointF ScrollerPrivate::mapToScrollArea(const QPoint &point)
{
    if (!m_scrollArea || !m_eventViewport)
        return point;

    QObject *vparent = m_eventViewport->parent();
    if (!vparent)
        return point;

    QGraphicsView *view = qobject_cast<QGraphicsView*>(vparent);
    if (!view)
        return point;

    QPoint pt = view->mapFromGlobal(point);
    return m_scrollArea->mapFromScene(view->mapToScene(pt));
}

bool ScrollerPrivate::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_scrollArea
        || (event->type() != QEvent::GraphicsSceneMouseMove
            && event->type() != QEvent::GraphicsSceneMousePress
            && event->type() != QEvent::GraphicsSceneMouseRelease
            /*&& event->type() != QEvent::GraphicsSceneKeyPressed
            && event->type() != QEvent::GraphicsSceneKeyReleased*/))
        return false;

    QGraphicsSceneMouseEvent* mouseEvent =
        static_cast<QGraphicsSceneMouseEvent*>(event);

    m_eventViewport = mouseEvent->widget();

    bool eventConsumed = false;

    switch (m_state) {
    case ScrollerPrivate::Stopped:
        if (mouseEvent->type() == QEvent::GraphicsSceneMousePress &&
            mouseEvent->buttons() == Qt::LeftButton) {
            m_cursorPos = QCursor::pos();
            m_speed = QPointF(0.0, 0.0);
            m_state = Started;
        }

        eventConsumed = true;
        break;

    case ScrollerPrivate::Started:
        if (mouseEvent->type() == QEvent::GraphicsSceneMouseMove) {
            m_cursorPos = QCursor::pos();
            m_state = ManualScrolling;

            if (!m_scrollTimer.isActive())
                m_scrollTimer.start(UpdateScrollingInterval);
            else {
                m_scrollTimer.stop();
                m_scrollTimer.start(UpdateScrollingInterval);
            }

        } else if (mouseEvent->type() == QEvent::MouseButtonRelease) {
            m_speed = QPoint(0, 0);
            m_state = Stopped;

            if (m_scrollTimer.isActive())
                m_scrollTimer.stop();
        }
        eventConsumed = true;
        break;

    case ScrollerPrivate::ManualScrolling:
        if (mouseEvent->type() == QEvent::GraphicsSceneMouseMove &&
            m_scrollArea->viewport()->boundingRect().contains(mouseEvent->pos()) ) {

            ScrollBar *hscroll = m_scrollArea->horizontalScrollBar();
            ScrollBar *vscroll = m_scrollArea->verticalScrollBar();

            QPointF d = m_scrollFactor * (mapToScrollArea(QCursor::pos()) - mapToScrollArea(m_cursorPos));

            hscroll->setSliderPosition(hscroll->sliderPosition() - d.x());
            vscroll->setSliderPosition(vscroll->sliderPosition() - d.y());

            if (m_lastCursorTime.elapsed() > UpdateScrollingInterval) {
                m_speed = mapToScrollArea(QCursor::pos()) - mapToScrollArea(m_cursorPos);
                m_lastCursorTime.restart();
            }

            m_lastFrameTime.restart();

            m_cursorPos = QCursor::pos();
        } else if (mouseEvent->type() == QEvent::GraphicsSceneMouseRelease) {
            m_state = AutoScrolling;
            m_scrollSlowAccum = 0;
            if (m_scrollTimer.isActive()) {
                m_scrollTimer.stop();
                m_scrollTimer.start(UpdateScrollingSmoothInterval);
            }
        }
        eventConsumed = true;
        break;

    case ScrollerPrivate::AutoScrolling:
        if (mouseEvent->type() == QEvent::GraphicsSceneMousePress) {
            stopScrolling();
        } else if (mouseEvent->type() == QEvent::MouseButtonRelease) {
            m_state = Stopped;
        }
        eventConsumed  = true;
        break;

    default:
        break;
    }

    return eventConsumed;
}

void ScrollerPrivate::updateScrolling()
{
    bool scrollOngoing = false;

    if (!m_scrollArea) {
        m_scrollTimer.stop();
        return;
    }

    if (m_state == ManualScrolling) {
        scrollOngoing = true;
        m_speed = mapToScrollArea(QCursor::pos()) - mapToScrollArea(m_cursorPos);
        m_cursorPos = QCursor::pos();
    } else if (m_state == AutoScrolling) {
        scrollOngoing = true;


        qreal x = qMax(-MaxScrollingSpeed, qMin(m_speed.x(), MaxScrollingSpeed));
        qreal y = qMax(-MaxScrollingSpeed, qMin(m_speed.y(), MaxScrollingSpeed));

        int sinceLast = m_lastFrameTime.elapsed();
        int slowdown = (ScrollStep * sinceLast) + m_scrollSlowAccum;
        m_scrollSlowAccum = slowdown & 0x3F;
        slowdown >>= 6;

        if (x > 0)
            x= qMax(qreal(0.0), x - slowdown);
        else
            x = qMin(qreal(0.0), x + slowdown);

        if (y > 0)
            y = qMax(qreal(0.0), y - slowdown);
        else
            y = qMin(qreal(0.0), y + slowdown);

        m_speed = QPoint(x,y);

        if (m_speed != QPoint(0,0)) {
            QPointF d;

            int xstep = (int(m_speed.x()) * sinceLast)>>6; // >>6 ~= *60 /1000 (==*64 /1024)
            int ystep = (int(m_speed.y()) * sinceLast)>>6;
            //qDebug() << sinceLast << "speedy" << speed.y()<<"ystep" << ystep;
            QPoint step = QPoint(xstep,ystep);

            if (ystep > 0)
                d = (m_scrollArea->pos() + step);
            else
                d = -(m_scrollArea->pos() - step);

            ScrollBar *hscroll = m_scrollArea->horizontalScrollBar();
            ScrollBar *vscroll = m_scrollArea->verticalScrollBar();

            hscroll->setSliderPosition(hscroll->sliderPosition() - m_scrollFactor * d.x());
            vscroll->setSliderPosition(vscroll->sliderPosition() - m_scrollFactor * d.y());
        } else {
            m_state = Stopped;
            scrollOngoing = false;
        }
    }

    m_lastFrameTime.restart();

    if (!scrollOngoing)
        m_scrollTimer.stop();
}


Scroller::Scroller(QObject *parent)
    : QObject(parent), d_ptr(new ScrollerPrivate(this))
{
    Q_D(Scroller);
    connect(&d->m_scrollTimer, SIGNAL(timeout()), this, SLOT(updateScrolling()));
}

Scroller::~Scroller()
{
    delete d_ptr;
}

void Scroller::setScrollable(AbstractScrollArea *area)
{
    Q_D(Scroller);

    if (!area)
        return;

    d->m_scrollArea = area;
}

void Scroller::setScrollFactor(qreal scrollFactor)
{
    Q_D(Scroller);

    d->m_scrollFactor = scrollFactor;
}

bool Scroller::eventFilter(QObject *obj, QEvent *event)
{
    Q_D(Scroller);
    return d->eventFilter(obj, event);
}

void Scroller::stopScrolling()
{
    Q_D(Scroller);
    d->stopScrolling();
}
#include "moc_scroller.cpp"
