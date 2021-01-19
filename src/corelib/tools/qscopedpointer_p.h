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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qglobal_p.h>

#ifndef QSCOPEDPOINTER_P_H
#define QSCOPEDPOINTER_P_H

#include "QtCore/qscopedpointer.h"

QT_BEGIN_NAMESPACE


/* Internal helper class - exposes the data through data_ptr (legacy from QShared).
   Required for some internal Qt classes, do not use otherwise. */
template <typename T, typename Cleanup = QScopedPointerDeleter<T> >
class QCustomScopedPointer : public QScopedPointer<T, Cleanup>
{
public:
    explicit inline QCustomScopedPointer(T *p = 0)
        : QScopedPointer<T, Cleanup>(p)
    {
    }

    inline T *&data_ptr()
    {
        return this->d;
    }

    inline bool operator==(const QCustomScopedPointer<T, Cleanup> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QCustomScopedPointer<T, Cleanup> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QCustomScopedPointer)
};

/* Internal helper class - a handler for QShared* classes, to be used in QCustomScopedPointer */
template <typename T>
class QScopedPointerSharedDeleter
{
public:
    static inline void cleanup(T *d)
    {
        if (d && !d->ref.deref())
            delete d;
    }
};

/* Internal.
   This class is basically a scoped pointer pointing to a ref-counted object
 */
template <typename T>
class QScopedSharedPointer : public QCustomScopedPointer<T, QScopedPointerSharedDeleter<T> >
{
public:
    explicit inline QScopedSharedPointer(T *p = 0)
        : QCustomScopedPointer<T, QScopedPointerSharedDeleter<T> >(p)
    {
    }

    inline void detach()
    {
        qAtomicDetach(this->d);
    }

    inline void assign(T *other)
    {
        if (this->d == other)
            return;
        if (other)
            other->ref.ref();
        T *oldD = this->d;
        this->d = other;
        QScopedPointerSharedDeleter<T>::cleanup(oldD);
    }

    inline bool operator==(const QScopedSharedPointer<T> &other) const
    {
        return this->d == other.d;
    }

    inline bool operator!=(const QScopedSharedPointer<T> &other) const
    {
        return this->d != other.d;
    }

private:
    Q_DISABLE_COPY(QScopedSharedPointer)
};


QT_END_NAMESPACE

#endif
