/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#ifndef QVECTOR_H
#define QVECTOR_H

#include <QtCore/qlist.h>
#include <QtCore/qcontainerfwd.h>

#if 0
#pragma qt_class(QVector)
#pragma qt_class(QMutableVectorIterator)
#pragma qt_class(QVectorIterator)
#endif

QT_BEGIN_NAMESPACE

#if !defined(QT_NO_JAVA_STYLE_ITERATORS)
template<typename T>
using QMutableVectorIterator = QMutableListIterator<T>;
template<typename T>
using QVectorIterator = QListIterator<T>;
#endif

QT_END_NAMESPACE

#endif // QVECTOR_H
