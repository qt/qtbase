/****************************************************************************
**
** Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QTUIOCURSOR_P_H
#define QTUIOCURSOR_P_H

#include <Qt>

QT_BEGIN_NAMESPACE

class QTuioCursor
{
public:
    QTuioCursor(int id = -1)
        : m_id(id)
        , m_x(0)
        , m_y(0)
        , m_vx(0)
        , m_vy(0)
        , m_acceleration(0)
        , m_state(Qt::TouchPointPressed)
    {
    }

    int id() const { return m_id; }

    void setX(float x)
    {
        if (state() == Qt::TouchPointStationary &&
            !qFuzzyCompare(m_x + 2.0, x + 2.0)) { // +2 because 1 is a valid value, and qFuzzyCompare can't cope with 0.0
            setState(Qt::TouchPointMoved);
        }
        m_x = x;
    }
    float x() const { return m_x; }

    void setY(float y)
    {
        if (state() == Qt::TouchPointStationary &&
            !qFuzzyCompare(m_y + 2.0, y + 2.0)) { // +2 because 1 is a valid value, and qFuzzyCompare can't cope with 0.0
            setState(Qt::TouchPointMoved);
        }
        m_y = y;
    }
    float y() const { return m_y; }

    void setVX(float vx) { m_vx = vx; }
    float vx() const { return m_vx; }

    void setVY(float vy) { m_vy = vy; }
    float vy() const { return m_vy; }

    void setAcceleration(float acceleration) { m_acceleration = acceleration; }
    float acceleration() const { return m_acceleration; }

    void setState(const Qt::TouchPointState &state) { m_state = state; }
    Qt::TouchPointState state() const { return m_state; }

private:
    int m_id;
    float m_x;
    float m_y;
    float m_vx;
    float m_vy;
    float m_acceleration;
    Qt::TouchPointState m_state;
};
Q_DECLARE_TYPEINFO(QTuioCursor, Q_MOVABLE_TYPE); // Q_PRIMITIVE_TYPE: not possible, m_state is = 1, not 0.

QT_END_NAMESPACE

#endif // QTUIOCURSOR_P_H
