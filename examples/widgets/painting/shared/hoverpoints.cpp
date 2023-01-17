// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "arthurwidgets.h"
#include "hoverpoints.h"

#include <algorithm>

#if QT_CONFIG(opengl)
#include <QtOpenGL/QOpenGLWindow>
#endif

HoverPoints::HoverPoints(QWidget *widget, PointShape shape)
    : QObject(widget),
      m_widget(widget),
      m_shape(shape)
{
    widget->installEventFilter(this);
    widget->setAttribute(Qt::WA_AcceptTouchEvents);

    connect(this, &HoverPoints::pointsChanged,
            m_widget, QOverload<>::of(&QWidget::update));
}

void HoverPoints::setEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        m_widget->update();
    }
}

bool HoverPoints::eventFilter(QObject *object, QEvent *event)
{
    if (object != m_widget || !m_enabled)
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    {
        if (!m_fingerPointMapping.isEmpty())
            return true;
        auto *me = static_cast<const QMouseEvent *>(event);
        QPointF clickPos = me->position().toPoint();
        qsizetype index = -1;
        for (qsizetype i = 0; i < m_points.size(); ++i) {
            QPainterPath path;
            const QRectF rect = pointBoundingRect(m_points.at(i));
            if (m_shape == CircleShape)
                path.addEllipse(rect);
            else
                path.addRect(rect);

            if (path.contains(clickPos)) {
                index = i;
                break;
            }
        }

        if (me->button() == Qt::LeftButton) {
            if (index == -1) {
                if (!m_editable)
                    return false;
                qsizetype pos = 0;
                // Insert sort for x or y
                switch (m_sortType) {
                case XSort:
                    for (qsizetype i = 0; i < m_points.size(); ++i) {
                        if (m_points.at(i).x() > clickPos.x()) {
                            pos = i;
                            break;
                        }
                    }
                    break;
                case YSort:
                    for (qsizetype i = 0; i < m_points.size(); ++i) {
                        if (m_points.at(i).y() > clickPos.y()) {
                            pos = i;
                            break;
                        }
                    }
                    break;
                default:
                    break;
                }

                m_points.insert(pos, clickPos);
                m_locks.insert(pos, 0);
                m_currentIndex = pos;
                firePointChange();
            } else {
                m_currentIndex = index;
            }
            return true;

        } else if (me->button() == Qt::RightButton) {
            if (index >= 0 && m_editable) {
                if (m_locks[index] == 0) {
                    m_locks.remove(index);
                    m_points.remove(index);
                }
                firePointChange();
                return true;
            }
        }
    }
    break;

    case QEvent::MouseButtonRelease:
        if (!m_fingerPointMapping.isEmpty())
            return true;
        m_currentIndex = -1;
        break;

    case QEvent::MouseMove:
        if (!m_fingerPointMapping.isEmpty())
            return true;
        if (m_currentIndex >= 0) {
            auto *me = static_cast<const QMouseEvent *>(event);
            movePoint(m_currentIndex, me->position().toPoint());
        }
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    {
        auto *touchEvent = static_cast<const QTouchEvent*>(event);
        const auto points = touchEvent->points();
        const qreal pointSize = qMax(m_pointSize.width(), m_pointSize.height());
        for (const auto &point : points) {
            const int id = point.id();
            switch (point.state()) {
            case QEventPoint::Pressed:
            {
                // find the point, move it
                const auto mappedPoints = m_fingerPointMapping.values();
                QSet<qsizetype> activePoints(mappedPoints.begin(), mappedPoints.end());
                qsizetype activePoint = -1;
                qreal distance = -1;
                const qsizetype pointsCount = m_points.size();
                const qsizetype activePointCount = activePoints.size();
                if (pointsCount == 2 && activePointCount == 1) { // only two points
                    activePoint = activePoints.contains(0) ? 1 : 0;
                } else {
                    for (qsizetype i = 0; i < pointsCount; ++i) {
                        if (activePoints.contains(i))
                            continue;

                        qreal d = QLineF(point.position(), m_points.at(i)).length();
                        if ((distance < 0 && d < 12 * pointSize) || d < distance) {
                            distance = d;
                            activePoint = i;
                        }

                    }
                }
                if (activePoint != -1) {
                    m_fingerPointMapping.insert(point.id(), activePoint);
                    movePoint(activePoint, point.position());
                }
            }
            break;
            case QEventPoint::Released:
            {
                // move the point and release
                const auto it = m_fingerPointMapping.constFind(id);
                movePoint(it.value(), point.position());
                m_fingerPointMapping.erase(it);
            }
            break;
            case QEventPoint::Updated:
            {
                // move the point
                const qsizetype pointIdx = m_fingerPointMapping.value(id, -1);
                if (pointIdx >= 0) // do we track this point?
                    movePoint(pointIdx, point.position());
            }
            break;
            default:
                break;
            }
        }
        if (m_fingerPointMapping.isEmpty()) {
            event->ignore();
            return false;
        }
        return true;
    }
    case QEvent::TouchEnd:
        if (m_fingerPointMapping.isEmpty()) {
            event->ignore();
            return false;
        }
        return true;

    case QEvent::Resize:
    {
        auto *e = static_cast<const QResizeEvent *>(event);
        if (e->oldSize().width() <= 0 || e->oldSize().height() <= 0)
            break;
        qreal stretch_x = e->size().width() / qreal(e->oldSize().width());
        qreal stretch_y = e->size().height() / qreal(e->oldSize().height());
        for (qsizetype i = 0; i < m_points.size(); ++i) {
            QPointF p = m_points.at(i);
            movePoint(i, QPointF(p.x() * stretch_x, p.y() * stretch_y), false);
        }

        firePointChange();
        break;
    }

    case QEvent::Paint:
    {
        QWidget *that_widget = m_widget;
        m_widget = nullptr;
        QCoreApplication::sendEvent(object, event);
        m_widget = that_widget;
        paintPoints();
        return true;
    }

    default:
        break;
    }

    return false;
}


void HoverPoints::paintPoints()
{
    QPainter p;
#if QT_CONFIG(opengl)
    ArthurFrame *af = qobject_cast<ArthurFrame *>(m_widget);
    if (af && af->usesOpenGL() && af->glWindow()->isValid()) {
        af->glWindow()->makeCurrent();
        p.begin(af->glWindow());
    } else {
        p.begin(m_widget);
    }
#else
    p.begin(m_widget);
#endif

    p.setRenderHint(QPainter::Antialiasing);

    if (m_connectionPen.style() != Qt::NoPen && m_connectionType != NoConnection) {
        p.setPen(m_connectionPen);

        if (m_connectionType == CurveConnection) {
            QPainterPath path;
            path.moveTo(m_points.at(0));
            for (qsizetype i = 1; i < m_points.size(); ++i) {
                QPointF p1 = m_points.at(i - 1);
                QPointF p2 = m_points.at(i);
                qreal distance = p2.x() - p1.x();

                path.cubicTo(p1.x() + distance / 2, p1.y(),
                             p1.x() + distance / 2, p2.y(),
                             p2.x(), p2.y());
            }
            p.drawPath(path);
        } else {
            p.drawPolyline(m_points);
        }
    }

    p.setPen(m_pointPen);
    p.setBrush(m_pointBrush);

    for (const auto &point : std::as_const(m_points)) {
        QRectF bounds = pointBoundingRect(point);
        if (m_shape == CircleShape)
            p.drawEllipse(bounds);
        else
            p.drawRect(bounds);
    }
}

static QPointF bound_point(const QPointF &point, const QRectF &bounds, int lock)
{
    QPointF p = point;

    qreal left = bounds.left();
    qreal right = bounds.right();
    qreal top = bounds.top();
    qreal bottom = bounds.bottom();

    if (p.x() < left || (lock & HoverPoints::LockToLeft)) p.setX(left);
    else if (p.x() > right || (lock & HoverPoints::LockToRight)) p.setX(right);

    if (p.y() < top || (lock & HoverPoints::LockToTop)) p.setY(top);
    else if (p.y() > bottom || (lock & HoverPoints::LockToBottom)) p.setY(bottom);

    return p;
}

void HoverPoints::setPoints(const QPolygonF &points)
{
    if (points.size() != m_points.size())
        m_fingerPointMapping.clear();
    m_points.clear();
    for (qsizetype i = 0; i < points.size(); ++i)
        m_points << bound_point(points.at(i), boundingRect(), 0);

    m_locks.clear();
    if (m_points.size() > 0) {
        m_locks.resize(m_points.size());
        m_locks.fill(0);
    }
}

void HoverPoints::movePoint(qsizetype index, const QPointF &point, bool emitUpdate)
{
    m_points[index] = bound_point(point, boundingRect(), m_locks.at(index));
    if (emitUpdate)
        firePointChange();
}

inline static bool x_less_than(const QPointF &p1, const QPointF &p2)
{
    return p1.x() < p2.x();
}

inline static bool y_less_than(const QPointF &p1, const QPointF &p2)
{
    return p1.y() < p2.y();
}

void HoverPoints::firePointChange()
{
    if (m_sortType != NoSort) {

        QPointF oldCurrent;
        if (m_currentIndex != -1)
            oldCurrent = m_points[m_currentIndex];

        if (m_sortType == XSort)
            std::sort(m_points.begin(), m_points.end(), x_less_than);
        else if (m_sortType == YSort)
            std::sort(m_points.begin(), m_points.end(), y_less_than);

        // Compensate for changed order...
        if (m_currentIndex != -1) {
            for (qsizetype i = 0; i < m_points.size(); ++i) {
                if (m_points[i] == oldCurrent) {
                    m_currentIndex = i;
                    break;
                }
            }
        }
    }

    emit pointsChanged(m_points);
}
