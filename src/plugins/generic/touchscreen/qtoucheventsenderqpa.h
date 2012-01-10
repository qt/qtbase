/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QTOUCHEVENTSENDERQPA_H
#define QTOUCHEVENTSENDERQPA_H

#include "qtouchscreen.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QTouchDevice;

class QTouchEventSenderQPA : public QTouchScreenObserver
{
public:
    QTouchEventSenderQPA(const QString &spec = QString());
    void touch_configure(int x_min, int x_max, int y_min, int y_max,
                         int pressure_min, int pressure_max, const QString &dev_name);
    void touch_point(const QList<QWindowSystemInterface::TouchPoint> &points);

private:
    bool m_forceToActiveWindow;
    int hw_range_x_min;
    int hw_range_x_max;
    int hw_range_y_min;
    int hw_range_y_max;
    int hw_pressure_min;
    int hw_pressure_max;
    QString hw_dev_name;
    QTouchDevice *m_device;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTOUCHEVENTSENDERQPA_H
