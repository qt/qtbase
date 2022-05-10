// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QQuaternion>
#include <QVector3D>


namespace src_gui_math3d_qquaternion {
QQuaternion q;
QVector3D vector;
void wrapper0() {

//! [0]
QVector3D result = q.rotatedVector(vector);
//! [0]

Q_UNUSED(result);
} // wrapper0


void wrapper1() {

//! [1]
QVector3D result = (q * QQuaternion(0, vector) * q.conjugated()).vector();
//! [1]

Q_UNUSED(result);
} // wrapper1
} // src_gui_math3d_qquaternion
