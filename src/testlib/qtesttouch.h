/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTESTTOUCH_H
#define QTESTTOUCH_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qtest_global.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtGui/QWindowSystemInterface>
#include <QtCore/qmap.h>
#include <QtGui/qevent.h>
#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qwidget.h>
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


namespace QTest
{

    class QTouchEventSequence
    {
    public:
        ~QTouchEventSequence()
        {
            if (commitWhenDestroyed)
                commit();
        }
        QTouchEventSequence& press(int touchId, const QPoint &pt, QWindow *window = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(window, pt));
            p.setState(Qt::TouchPointPressed);
            return *this;
        }
        QTouchEventSequence& move(int touchId, const QPoint &pt, QWindow *window = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(window, pt));
            p.setState(Qt::TouchPointMoved);
            return *this;
        }
        QTouchEventSequence& release(int touchId, const QPoint &pt, QWindow *window = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(window, pt));
            p.setState(Qt::TouchPointReleased);
            return *this;
        }
        QTouchEventSequence& stationary(int touchId)
        {
            QTouchEvent::TouchPoint &p = pointOrPreviousPoint(touchId);
            p.setState(Qt::TouchPointStationary);
            return *this;
        }

#ifdef QT_WIDGETS_LIB
        QTouchEventSequence& press(int touchId, const QPoint &pt, QWidget *widget = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(widget, pt));
            p.setState(Qt::TouchPointPressed);
            return *this;
        }
        QTouchEventSequence& move(int touchId, const QPoint &pt, QWidget *widget = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(widget, pt));
            p.setState(Qt::TouchPointMoved);
            return *this;
        }
        QTouchEventSequence& release(int touchId, const QPoint &pt, QWidget *widget = 0)
        {
            QTouchEvent::TouchPoint &p = point(touchId);
            p.setScreenPos(mapToScreen(widget, pt));
            p.setState(Qt::TouchPointReleased);
            return *this;
        }
#endif

        void commit(bool processEvents = true)
        {
            if (!points.isEmpty()) {
                if (targetWindow)
                {
                    QWindowSystemInterface::handleTouchEvent(targetWindow, device, touchPointList(points.values()));
                }
#ifdef QT_WIDGETS_LIB
                else if (targetWidget)
                {
                    QWindowSystemInterface::handleTouchEvent(targetWidget->windowHandle(), device, touchPointList(points.values()));
                }
#endif
            }
            if (processEvents)
                QCoreApplication::processEvents();
            previousPoints = points;
            points.clear();
        }

        static QWindowSystemInterface::TouchPoint touchPoint(const QTouchEvent::TouchPoint& pt)
        {
            QWindowSystemInterface::TouchPoint p;
            p.id = pt.id();
            p.flags = pt.flags();
            p.normalPosition = pt.normalizedPos();
            p.area = pt.screenRect();
            p.pressure = pt.pressure();
            p.state = pt.state();
            p.velocity = pt.velocity();
            p.rawPositions = pt.rawScreenPositions();
            return p;
        }
        static QList<struct QWindowSystemInterface::TouchPoint> touchPointList(const QList<QTouchEvent::TouchPoint>& pointList)
        {
            QList<struct QWindowSystemInterface::TouchPoint> newList;

            Q_FOREACH (QTouchEvent::TouchPoint p, pointList)
            {
                newList.append(touchPoint(p));
            }
            return newList;
        }

    private:
#ifdef QT_WIDGETS_LIB
        QTouchEventSequence(QWidget *widget, QTouchDevice *aDevice, bool autoCommit)
            : targetWidget(widget), targetWindow(0), device(aDevice), commitWhenDestroyed(autoCommit)
        {
        }
#endif
        QTouchEventSequence(QWindow *window, QTouchDevice *aDevice, bool autoCommit)
            :
#ifdef QT_WIDGETS_LIB
              targetWidget(0),
#endif
              targetWindow(window), device(aDevice), commitWhenDestroyed(autoCommit)
        {
        }

        QTouchEvent::TouchPoint &point(int touchId)
        {
            if (!points.contains(touchId))
                points[touchId] = QTouchEvent::TouchPoint(touchId);
            return points[touchId];
        }

        QTouchEvent::TouchPoint &pointOrPreviousPoint(int touchId)
        {
            if (!points.contains(touchId)) {
                if (previousPoints.contains(touchId))
                    points[touchId] = previousPoints.value(touchId);
                else
                    points[touchId] = QTouchEvent::TouchPoint(touchId);
            }
            return points[touchId];
        }

#ifdef QT_WIDGETS_LIB
        QPoint mapToScreen(QWidget *widget, const QPoint &pt)
        {
            if (widget)
                return widget->mapToGlobal(pt);
            return targetWidget ? targetWidget->mapToGlobal(pt) : pt;
        }
#endif
        QPoint mapToScreen(QWindow *window, const QPoint &pt)
        {
            if(window)
                return window->mapToGlobal(pt);
            return targetWindow ? targetWindow->mapToGlobal(pt) : pt;
        }

        QMap<int, QTouchEvent::TouchPoint> previousPoints;
        QMap<int, QTouchEvent::TouchPoint> points;
#ifdef QT_WIDGETS_LIB
        QWidget *targetWidget;
#endif
        QWindow *targetWindow;
        QTouchDevice *device;
        bool commitWhenDestroyed;
#ifdef QT_WIDGETS_LIB
        friend QTouchEventSequence touchEvent(QWidget *, QTouchDevice*, bool);
#endif
        friend QTouchEventSequence touchEvent(QWindow *, QTouchDevice*, bool);
    };

#ifdef QT_WIDGETS_LIB
    inline
    QTouchEventSequence touchEvent(QWidget *widget,
                                   QTouchDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(widget, device, autoCommit);
    }
#endif
    inline
    QTouchEventSequence touchEvent(QWindow *window,
                                   QTouchDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(window, device, autoCommit);
    }

}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTESTTOUCH_H
