/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef TRACKBALL_H
#define TRACKBALL_H

#include <QtWidgets>

#include <QtGui/qvector3d.h>
#include <QtGui/qquaternion.h>

class TrackBall
{
public:
    enum TrackMode
    {
        Plane,
        Sphere,
    };
    TrackBall(TrackMode mode = Sphere);
    TrackBall(float angularVelocity, const QVector3D& axis, TrackMode mode = Sphere);
    // coordinates in [-1,1]x[-1,1]
    void push(const QPointF& p, const QQuaternion &transformation);
    void move(const QPointF& p, const QQuaternion &transformation);
    void release(const QPointF& p, const QQuaternion &transformation);
    void start(); // starts clock
    void stop(); // stops clock
    QQuaternion rotation() const;
private:
    QQuaternion m_rotation;
    QVector3D m_axis;
    float m_angularVelocity;

    QPointF m_lastPos;
    QTime m_lastTime;
    bool m_paused;
    bool m_pressed;
    TrackMode m_mode;
};

#endif
