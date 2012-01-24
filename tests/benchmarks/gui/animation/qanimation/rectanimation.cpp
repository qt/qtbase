/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite module of the Qt Toolkit.
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
