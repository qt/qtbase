// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rectanimation.h"
#include "dummyobject.h"

static inline int interpolateInteger(int from, int to, qreal progress)
{
   return from + (to - from) * progress;
}


RectAnimation::RectAnimation(DummyObject *obj) : m_object(obj), m_dura(250)
{
}

void RectAnimation::setEndValue(const QRect &rect)
{
    m_end = rect;
}

void RectAnimation::setStartValue(const QRect &rect)
{
    m_start = rect;
}

void RectAnimation::setDuration(int d)
{
    m_dura = d;
}

int RectAnimation::duration() const
{
    return m_dura;
}


void RectAnimation::updateCurrentTime(int currentTime)
{
    qreal progress = m_easing.valueForProgress( currentTime / qreal(m_dura) );
    QRect now;
    now.setCoords(interpolateInteger(m_start.left(), m_end.left(), progress),
                  interpolateInteger(m_start.top(), m_end.top(), progress),
                  interpolateInteger(m_start.right(), m_end.right(), progress),
                  interpolateInteger(m_start.bottom(), m_end.bottom(), progress));

    bool changed = (now != m_current);
    if (changed)
        m_current = now;

    if (state() == Stopped)
        return;

    if (m_object)
        m_object->setRect(m_current);
}
