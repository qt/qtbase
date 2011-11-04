/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#include <QtWidgets/qwidget.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Test)

#ifdef QT_WIDGETS_LIB
extern Q_GUI_EXPORT void qt_translateRawTouchEvent(QWidget *window,
                                                   QTouchEvent::DeviceType deviceType,
                                                   const QList<QTouchEvent::TouchPoint> &touchPoints,
                                                   ulong timestamp);
#endif

namespace QTest
{

    class QTouchEventSequence
    {
    public:
        ~QTouchEventSequence()
        {
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

        void commit()
        {
            if (!points.isEmpty()) {
                if (targetWindow)
                {
                    QWindowSystemInterface::handleTouchEvent(targetWindow,QEvent::None,deviceType, touchPointList(points.values()));
                    QTest::qWait(10);
                }
#ifdef QT_WIDGETS_LIB
                else if (targetWidget)
                {
                    qt_translateRawTouchEvent(targetWidget, deviceType, points.values(), 0);
                }
#endif
            }
            previousPoints = points;
            points.clear();
        }

    private:
#ifdef QT_WIDGETS_LIB
        QTouchEventSequence(QWidget *widget, QTouchEvent::DeviceType aDeviceType)
            : targetWidget(widget), targetWindow(0), deviceType(aDeviceType)
        {
        }
#endif
        QTouchEventSequence(QWindow *window, QTouchEvent::DeviceType aDeviceType)
            :
#ifdef QT_WIDGETS_LIB
              targetWidget(0),
#endif
              targetWindow(window), deviceType(aDeviceType)
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
        QWindowSystemInterface::TouchPoint touchPoint(const QTouchEvent::TouchPoint& pt)
        {
            QWindowSystemInterface::TouchPoint p;
            p.id = pt.id();
            p.isPrimary = pt.isPrimary();
            p.normalPosition = pt.screenRect().topLeft();
            p.area = pt.screenRect();
            p.pressure = pt.pressure();
            p.state = pt.state();
            return p;
        }
        QList<struct QWindowSystemInterface::TouchPoint> touchPointList(const QList<QTouchEvent::TouchPoint>& pointList)
        {
            QList<struct QWindowSystemInterface::TouchPoint> newList;

            foreach(QTouchEvent::TouchPoint p, pointList)
            {
                newList.append(touchPoint(p));
            }
            return newList;
        }

        QMap<int, QTouchEvent::TouchPoint> previousPoints;
        QMap<int, QTouchEvent::TouchPoint> points;
#ifdef QT_WIDGETS_LIB
        QWidget *targetWidget;
#endif
        QWindow *targetWindow;
        QTouchEvent::DeviceType deviceType;
#ifdef QT_WIDGETS_LIB
        friend QTouchEventSequence touchEvent(QWidget *, QTouchEvent::DeviceType);
#endif
        friend QTouchEventSequence touchEvent(QWindow *, QTouchEvent::DeviceType);
    };

#ifdef QT_WIDGETS_LIB
    inline
    QTouchEventSequence touchEvent(QWidget *widget = 0,
                                   QTouchEvent::DeviceType deviceType = QTouchEvent::TouchScreen)
    {
        return QTouchEventSequence(widget, deviceType);
    }
#endif
    inline
    QTouchEventSequence touchEvent(QWindow *window = 0,
                                   QTouchEvent::DeviceType deviceType = QTouchEvent::TouchScreen)
    {
        return QTouchEventSequence(window, deviceType);
    }

}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTESTTOUCH_H
