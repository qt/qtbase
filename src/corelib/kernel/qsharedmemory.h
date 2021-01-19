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

#ifndef QSHAREDMEMORY_H
#define QSHAREDMEMORY_H

#include <QtCore/qglobal.h>
#ifndef QT_NO_QOBJECT
# include <QtCore/qobject.h>
#else
# include <QtCore/qobjectdefs.h>
# include <QtCore/qscopedpointer.h>
# include <QtCore/qstring.h>
#endif
QT_BEGIN_NAMESPACE


#ifndef QT_NO_SHAREDMEMORY

class QSharedMemoryPrivate;

class Q_CORE_EXPORT QSharedMemory
#ifndef QT_NO_QOBJECT
    : public QObject
#endif
{
#ifndef QT_NO_QOBJECT
    Q_OBJECT
#endif
    Q_DECLARE_PRIVATE(QSharedMemory)

public:
    enum AccessMode
    {
        ReadOnly,
        ReadWrite
    };

    enum SharedMemoryError
    {
        NoError,
        PermissionDenied,
        InvalidSize,
        KeyError,
        AlreadyExists,
        NotFound,
        LockError,
        OutOfResources,
        UnknownError
    };

#ifndef QT_NO_QOBJECT
    QSharedMemory(QObject *parent = nullptr);
    QSharedMemory(const QString &key, QObject *parent = nullptr);
#else
    QSharedMemory();
    QSharedMemory(const QString &key);
    static QString tr(const char * str)
    {
        return QString::fromLatin1(str);
    }
#endif
    ~QSharedMemory();

    void setKey(const QString &key);
    QString key() const;
    void setNativeKey(const QString &key);
    QString nativeKey() const;

    bool create(int size, AccessMode mode = ReadWrite);
    int size() const;

    bool attach(AccessMode mode = ReadWrite);
    bool isAttached() const;
    bool detach();

    void *data();
    const void* constData() const;
    const void *data() const;

#ifndef QT_NO_SYSTEMSEMAPHORE
    bool lock();
    bool unlock();
#endif

    SharedMemoryError error() const;
    QString errorString() const;

private:
    Q_DISABLE_COPY(QSharedMemory)
#ifdef QT_NO_QOBJECT
    QScopedPointer<QSharedMemoryPrivate> d_ptr;
#endif
};

#endif // QT_NO_SHAREDMEMORY

QT_END_NAMESPACE

#endif // QSHAREDMEMORY_H

