// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
        , m_state(QEventPoint::State::Pressed)
    {
    }

    int id() const { return m_id; }

    void setX(float x)
    {
        if (state() == QEventPoint::State::Stationary &&
            !qFuzzyCompare(m_x + 2.0, x + 2.0)) { // +2 because 1 is a valid value, and qFuzzyCompare can't cope with 0.0
            setState(QEventPoint::State::Updated);
        }
        m_x = x;
    }
    float x() const { return m_x; }

    void setY(float y)
    {
        if (state() == QEventPoint::State::Stationary &&
            !qFuzzyCompare(m_y + 2.0, y + 2.0)) { // +2 because 1 is a valid value, and qFuzzyCompare can't cope with 0.0
            setState(QEventPoint::State::Updated);
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

    void setState(const QEventPoint::State &state) { m_state = state; }
    QEventPoint::State state() const { return m_state; }

private:
    int m_id;
    float m_x;
    float m_y;
    float m_vx;
    float m_vy;
    float m_acceleration;
    QEventPoint::State m_state;
};
Q_DECLARE_TYPEINFO(QTuioCursor, Q_RELOCATABLE_TYPE); // Q_PRIMITIVE_TYPE: not possible, m_state is = 1, not 0.

QT_END_NAMESPACE

#endif // QTUIOCURSOR_P_H
