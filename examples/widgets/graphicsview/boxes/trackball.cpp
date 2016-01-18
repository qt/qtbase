/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "trackball.h"
#include "scene.h"
#include <cmath>

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
                lastPos3D.setZ(std::sqrt(sqrZ));
            else
                lastPos3D.normalize();

            QVector3D currentPos3D = QVector3D(p.x(), p.y(), 0.0f);
            sqrZ = 1 - QVector3D::dotProduct(currentPos3D, currentPos3D);
            if (sqrZ > 0)
                currentPos3D.setZ(std::sqrt(sqrZ));
            else
                currentPos3D.normalize();

            m_axis = QVector3D::crossProduct(lastPos3D, currentPos3D);
            float angle = 180 / PI * std::asin(std::sqrt(QVector3D::dotProduct(m_axis, m_axis)));

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

