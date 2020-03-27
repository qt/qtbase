/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
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

#ifndef QTESTTOUCH_H
#define QTESTTOUCH_H

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

#include <QtTest/qttestglobal.h>
#include <QtTest/qtestassert.h>
#include <QtTest/qtestsystem.h>
#include <QtTest/qtestspontaneevent.h>
#include <QtCore/qmap.h>
#include <QtGui/qevent.h>
#include <QtGui/qpointingdevice.h>
#include <QtGui/qwindow.h>
#include <QtGui/qpointingdevice.h>
#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qwidget.h>
#endif

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT  void qt_handleTouchEvent(QWindow *w, const QPointingDevice *device,
                                const QList<QEventPoint> &points,
                                Qt::KeyboardModifiers mods = Qt::NoModifier);


namespace QTest
{
    Q_GUI_EXPORT QPointingDevice * createTouchDevice(QInputDevice::DeviceType devType = QInputDevice::DeviceType::TouchScreen,
                                                     QInputDevice::Capabilities caps = QInputDevice::Capability::Position);


    class QTouchEventSequence
    {
    public:
        ~QTouchEventSequence()
        {
            if (commitWhenDestroyed)
                commit();
        }
        QTouchEventSequence& press(int touchId, const QPoint &pt, QWindow *window = nullptr)
        {
            QEventPoint &p = point(touchId);
            p.m_globalPos = mapToScreen(window, pt);
            p.m_state = QEventPoint::State::Pressed;
            return *this;
        }
        QTouchEventSequence& move(int touchId, const QPoint &pt, QWindow *window = nullptr)
        {
            QEventPoint &p = point(touchId);
            p.m_globalPos = mapToScreen(window, pt);
            p.m_state = QEventPoint::State::Updated;
            return *this;
        }
        QTouchEventSequence& release(int touchId, const QPoint &pt, QWindow *window = nullptr)
        {
            QEventPoint &p = point(touchId);
            p.m_globalPos = mapToScreen(window, pt);
            p.m_state = QEventPoint::State::Released;
            return *this;
        }
        QTouchEventSequence& stationary(int touchId)
        {
            QEventPoint &p = pointOrPreviousPoint(touchId);
            p.m_state = QEventPoint::State::Stationary;
            return *this;
        }

#ifdef QT_WIDGETS_LIB
        QTouchEventSequence& press(int touchId, const QPoint &pt, QWidget *widget = nullptr)
        {
            QEventPoint &p = point(touchId);
            p.m_globalPos = mapToScreen(widget, pt);
            p.m_state = QEventPoint::State::Pressed;
            return *this;
        }
        QTouchEventSequence& move(int touchId, const QPoint &pt, QWidget *widget = nullptr)
        {
            QEventPoint &p = point(touchId);
            p.m_globalPos = mapToScreen(widget, pt);
            p.m_state = QEventPoint::State::Updated;
            return *this;
        }
        QTouchEventSequence& release(int touchId, const QPoint &pt, QWidget *widget = nullptr)
        {
            QEventPoint &p = point(touchId);
            p.m_globalPos = mapToScreen(widget, pt);
            p.m_state = QEventPoint::State::Released;
            return *this;
        }
#endif

        void commit(bool processEvents = true)
        {
            if (!points.isEmpty()) {
                qSleep(1);
                if (targetWindow)
                {
                    qt_handleTouchEvent(targetWindow, device, points.values());
                }
#ifdef QT_WIDGETS_LIB
                else if (targetWidget)
                {
                    qt_handleTouchEvent(targetWidget->windowHandle(), device, points.values());
                }
#endif
            }
            if (processEvents)
                QCoreApplication::processEvents();
            previousPoints = points;
            points.clear();
        }

private:
#ifdef QT_WIDGETS_LIB
        QTouchEventSequence(QWidget *widget, QPointingDevice *aDevice, bool autoCommit)
            : targetWidget(widget), targetWindow(nullptr), device(aDevice), commitWhenDestroyed(autoCommit)
        {
        }
#endif
        QTouchEventSequence(QWindow *window, QPointingDevice *aDevice, bool autoCommit)
            :
#ifdef QT_WIDGETS_LIB
              targetWidget(nullptr),
#endif
              targetWindow(window), device(aDevice), commitWhenDestroyed(autoCommit)
        {
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

        QMap<int, QEventPoint> previousPoints;
        QMap<int, QEventPoint> points;
#ifdef QT_WIDGETS_LIB
        QWidget *targetWidget;
#endif
        QWindow *targetWindow;
        QPointingDevice *device;
        bool commitWhenDestroyed;
#if defined(QT_WIDGETS_LIB) || defined(Q_CLANG_QDOC)
        friend QTouchEventSequence touchEvent(QWidget *widget, QPointingDevice *device, bool autoCommit);
#endif
        friend QTouchEventSequence touchEvent(QWindow *window, QPointingDevice *device, bool autoCommit);

    protected:
        // These don't make sense for public testing API,
        // because we are getting rid of most public setters in QEventPoint.
        // Each of these constructs a QEventPoint with null parent; in normal usage,
        // the QTouchEvent constructor will set the points' parents to itself, later on.
        QEventPoint &point(int touchId)
        {
            if (!points.contains(touchId))
                points[touchId] = QEventPoint(touchId);
            return points[touchId];
        }

        QEventPoint &pointOrPreviousPoint(int touchId)
        {
            if (!points.contains(touchId)) {
                if (previousPoints.contains(touchId))
                    points[touchId] = previousPoints.value(touchId);
                else
                    points[touchId] = QEventPoint(touchId);
            }
            return points[touchId];
        }
    };

#if defined(QT_WIDGETS_LIB) || defined(Q_CLANG_QDOC)
    inline
    QTouchEventSequence touchEvent(QWidget *widget,
                                   QPointingDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(widget, device, autoCommit);
    }
#endif
    inline
    QTouchEventSequence touchEvent(QWindow *window,
                                   QPointingDevice *device,
                                   bool autoCommit = true)
    {
        return QTouchEventSequence(window, device, autoCommit);
    }

}

QT_END_NAMESPACE

#endif // QTESTTOUCH_H
