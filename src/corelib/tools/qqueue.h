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

#ifndef QQUEUE_H
#define QQUEUE_H

#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE


template <class T>
class QQueue : public QList<T>
{
public:
    // compiler-generated special member functions are fine!
    inline void swap(QQueue<T> &other) noexcept { QList<T>::swap(other); } // prevent QList<->QQueue swaps
#if QT_DEPRECATED_SINCE(5, 14) && !defined(Q_QDOC)
    // NOT using QList<T>::swap; it would make swap(QList&) available.
    QT_DEPRECATED_VERSION_X_5_14("Use swapItemsAt(i, j) instead")
    inline void swap(int i, int j) { QList<T>::swapItemsAt(i, j); }
#endif
    inline void enqueue(const T &t) { QList<T>::append(t); }
    inline T dequeue() { return QList<T>::takeFirst(); }
    inline T &head() { return QList<T>::first(); }
    inline const T &head() const { return QList<T>::first(); }
};

QT_END_NAMESPACE

#endif // QQUEUE_H
