/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QNUMERIC_H
#define QNUMERIC_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsInf(double d);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsNaN(double d);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsFinite(double d);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION int qFpClassify(double val);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsInf(float f);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsNaN(float f);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION bool qIsFinite(float f);
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION int qFpClassify(float val);
#if QT_CONFIG(signaling_nan)
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION double qSNaN();
#endif
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION double qQNaN();
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION double qInf();

Q_CORE_EXPORT quint32 qFloatDistance(float a, float b);
Q_CORE_EXPORT quint64 qFloatDistance(double a, double b);

#define Q_INFINITY (QT_PREPEND_NAMESPACE(qInf)())
#if QT_CONFIG(signaling_nan)
#  define Q_SNAN (QT_PREPEND_NAMESPACE(qSNaN)())
#endif
#define Q_QNAN (QT_PREPEND_NAMESPACE(qQNaN)())

QT_END_NAMESPACE

#endif // QNUMERIC_H
