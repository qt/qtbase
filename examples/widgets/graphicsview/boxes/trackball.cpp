/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "trackball.h"
#include "scene.h"

//============================================================================//
//                                  TrackBall                                 //
//============================================================================//

TrackBall::TrackBall(TrackMode mode)
    : m_angularVelocity(0)
    , m_paused(false)
    , m_pressed(false)
    , m_mode(mode)
{
    m_axis = QVector3D(0, 1, 0);
    m_rotation = QQuaternion();
    m_lastTime = QTime::currentTime();
}

TrackBall::TrackBall(float angularVelocity, const QVector3D& axis, TrackMode mode)
    : m_axis(axis)
    , m_angularVelocity(angularVelocity)
    , m_paused(false)
    , m_pressed(false)
    , m_mode(mode)
{
    m_rotation = QQuaternion();
    m_lastTime = QTime::currentTime();
}

void TrackBall::push(const QPointF& p, const QQuaternion &)
{
    m_rotation = rotation();
    m_pressed = true;
    m_lastTime = QTime::currentTime();
    m_lastPos = p;
    m_angularVelocity = 0.0f;
}

void TrackBall::move(const QPointF& p, const QQuaternion &transformation)
{
    if (!m_pressed)
        return;

    QTime currentTime = QTime::currentTime();
    int msecs = m_lastTime.msecsTo(currentTime);
    if (msecs <= 20)
        return;

    switch (m_mode) {
    case Plane:
        {
            QLineF delta(m_lastPos, p);
            m_angularVelocity = 180*delta.length() / (PI*msecs);
            m_axis = QVector3D(-delta.dy(), delta.dx(), 0.0f).normalized();
            m_axis = transformation.rotatedVector(m_axis);
            m_rotation = QQuaternion::fromAxisAndAngle(m_axis, 180 / PI * delta.length()) * m_rotation;
        }
        break;
    case Sphere:
        {
            QVector3D lastPos3D = QVector3D(m_lastPos.x(), m_lastPos.y(), 0.0f);
            float sqrZ = 1 - QVector3D::dotProduct(lastPos3D, lastPos3D);
            if (sqrZ > 0)
                lastPos3D.setZ(sqrt(sqrZ));
            else
                lastPos3D.normalize();

            QVector3D currentPos3D = QVector3D(p.x(), p.y(), 0.0f);
            sqrZ = 1 - QVector3D::dotProduct(currentPos3D, currentPos3D);
            if (sqrZ > 0)
                currentPos3D.setZ(sqrt(sqrZ));
            else
                currentPos3D.normalize();

            m_axis = QVector3D::crossProduct(lastPos3D, currentPos3D);
            float angle = 180 / PI * asin(sqrt(QVector3D::dotProduct(m_axis, m_axis)));

            m_angularVelocity = angle / msecs;
            m_axis.normalize();
            m_axis = transformation.rotatedVector(m_axis);
            m_rotation = QQuaternion::fromAxisAndAngle(m_axis, angle) * m_rotation;
        }
        break;
    }


    m_lastPos = p;
    m_lastTime = currentTime;
}

void TrackBall::release(const QPointF& p, const QQuaternion &transformation)
{
    // Calling move() caused the rotation to stop if the framerate was too low.
    move(p, transformation);
    m_pressed = false;
}

void TrackBall::start()
{
    m_lastTime = QTime::currentTime();
    m_paused = false;
}

void TrackBall::stop()
{
    m_rotation = rotation();
    m_paused = true;
}

QQuaternion TrackBall::rotation() const
{
    if (m_paused || m_pressed)
        return m_rotation;

    QTime currentTime = QTime::currentTime();
    float angle = m_angularVelocity * m_lastTime.msecsTo(currentTime);
    return QQuaternion::fromAxisAndAngle(m_axis, angle) * m_rotation;
}

