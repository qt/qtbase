/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins module of the Qt Toolkit.
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

#include "qtoucheventsenderqpa.h"
#include <QGuiApplication>
#include <QScreen>
#include <QStringList>
#include <QDebug>

QT_BEGIN_NAMESPACE

//#define POINT_DEBUG

QTouchEventSenderQPA::QTouchEventSenderQPA(const QString &spec)
{
    m_forceToActiveWindow = spec.split(QLatin1Char(':')).contains(QLatin1String("force_window"));
}

void QTouchEventSenderQPA::touch_configure(int x_min, int x_max, int y_min, int y_max)
{
    hw_range_x_min = x_min;
    hw_range_x_max = x_max;
    hw_range_y_min = y_min;
    hw_range_y_max = y_max;
}

void QTouchEventSenderQPA::touch_point(QEvent::Type state,
                                       const QList<QWindowSystemInterface::TouchPoint> &points)
{
    QRect winRect;
    if (m_forceToActiveWindow) {
        QWindow *win = QGuiApplication::activeWindow();
        if (!win)
            return;
        winRect = win->geometry();
    } else {
        winRect = QGuiApplication::primaryScreen()->geometry();
    }

#ifdef POINT_DEBUG
    qDebug() << "QPA: Mapping" << points.size() << "points to" << winRect << state;
#endif

    QList<QWindowSystemInterface::TouchPoint> touchPoints = points;
    // Translate the coordinates and set the normalized position. QPA expects
    // 'area' to be in screen coordinates, while the device reports them in its
    // own system with (0, 0) being the center point of the device.
    for (int i = 0; i < touchPoints.size(); ++i) {
        QWindowSystemInterface::TouchPoint &tp(touchPoints[i]);

        // Translate so that (0, 0) is the top-left corner.
        const int hw_x = qBound(hw_range_x_min, int(tp.area.left()), hw_range_x_max) - hw_range_x_min;
        const int hw_y = qBound(hw_range_y_min, int(tp.area.top()), hw_range_y_max) - hw_range_y_min;

        // Get a normalized position in range 0..1.
        const int hw_w = hw_range_x_max - hw_range_x_min;
        const int hw_h = hw_range_y_max - hw_range_y_min;
        tp.normalPosition = QPointF(hw_x / qreal(hw_w),
                                    hw_y / qreal(hw_h));

        qreal nx = tp.normalPosition.x();
        qreal ny = tp.normalPosition.y();

        // Generate a screen position that is always inside the active window or the default screen.
        const int wx = winRect.left() + int(nx * winRect.width());
        const int wy = winRect.top() + int(ny * winRect.height());
        tp.area.moveTopLeft(QPoint(wx, wy));

#ifdef POINT_DEBUG
        qDebug() << "    " << i << tp.area << tp.state << tp.id << tp.isPrimary << tp.pressure;
#endif
    }

    QWindowSystemInterface::handleTouchEvent(0, state, QTouchEvent::TouchScreen, touchPoints);
}

QT_END_NAMESPACE
