/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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
#include <QtGui/qtguiglobal.h>

#include <QtCore/qvariantanimation.h>
#include <private/qvariantanimation_p.h>
#include <QtGui/qcolor.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>
#include <QtGui/qquaternion.h>

QT_BEGIN_NAMESPACE

template<> Q_INLINE_TEMPLATE QColor _q_interpolate(const QColor &f,const QColor &t, qreal progress)
{
    return QColor(qBound(0,_q_interpolate(f.red(), t.red(), progress),255),
                  qBound(0,_q_interpolate(f.green(), t.green(), progress),255),
                  qBound(0,_q_interpolate(f.blue(), t.blue(), progress),255),
                  qBound(0,_q_interpolate(f.alpha(), t.alpha(), progress),255));
}

template<> Q_INLINE_TEMPLATE QQuaternion _q_interpolate(const QQuaternion &f,const QQuaternion &t, qreal progress)
{
    return QQuaternion::slerp(f, t, progress);
}

void qRegisterGuiGetInterpolator()
{
    qRegisterAnimationInterpolator<QColor>(_q_interpolateVariant<QColor>);
    qRegisterAnimationInterpolator<QVector2D>(_q_interpolateVariant<QVector2D>);
    qRegisterAnimationInterpolator<QVector3D>(_q_interpolateVariant<QVector3D>);
    qRegisterAnimationInterpolator<QVector4D>(_q_interpolateVariant<QVector4D>);
    qRegisterAnimationInterpolator<QQuaternion>(_q_interpolateVariant<QQuaternion>);
}
Q_CONSTRUCTOR_FUNCTION(qRegisterGuiGetInterpolator)

static void qUnregisterGuiGetInterpolator()
{
    // casts required by Sun CC 5.5
    qRegisterAnimationInterpolator<QColor>(
        (QVariant (*)(const QColor &, const QColor &, qreal))0);
    qRegisterAnimationInterpolator<QVector2D>(
        (QVariant (*)(const QVector2D &, const QVector2D &, qreal))0);
    qRegisterAnimationInterpolator<QVector3D>(
        (QVariant (*)(const QVector3D &, const QVector3D &, qreal))0);
    qRegisterAnimationInterpolator<QVector4D>(
        (QVariant (*)(const QVector4D &, const QVector4D &, qreal))0);
    qRegisterAnimationInterpolator<QQuaternion>(
        (QVariant (*)(const QQuaternion &, const QQuaternion &, qreal))0);
}
Q_DESTRUCTOR_FUNCTION(qUnregisterGuiGetInterpolator)

QT_END_NAMESPACE
