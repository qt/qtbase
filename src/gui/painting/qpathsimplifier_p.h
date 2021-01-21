/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QPATHSIMPLIFIER_P_H
#define QPATHSIMPLIFIER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtGui/private/qtguiglobal_p.h>
#include <QtGui/qpainterpath.h>
#include <QtGui/private/qdatabuffer_p.h>
#include <QtGui/private/qvectorpath_p.h>

QT_BEGIN_NAMESPACE

// The returned vertices are in 8:8 fixed point format. The path is assumed to be in the range (-128, 128)x(-128, 128).
void qSimplifyPath(const QVectorPath &path, QDataBuffer<QPoint> &vertices, QDataBuffer<quint32> &indices, const QTransform &matrix = QTransform());
void qSimplifyPath(const QPainterPath &path, QDataBuffer<QPoint> &vertices, QDataBuffer<quint32> &indices, const QTransform &matrix = QTransform());

QT_END_NAMESPACE

#endif
