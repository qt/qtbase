// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSYSTEMSEMAPHORE_H
#define QSYSTEMSEMAPHORE_H

#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE


#if QT_CONFIG(systemsemaphore)

class QSystemSemaphorePrivate;

class Q_CORE_EXPORT QSystemSemaphore
{
    Q_GADGET
    Q_DECLARE_TR_FUNCTIONS(QSystemSemaphore)
public:
    enum AccessMode
    {
        Open,
        Create
    };
    Q_ENUM(AccessMode)

    enum SystemSemaphoreError
    {
        NoError,
        PermissionDenied,
        KeyError,
        AlreadyExists,
        NotFound,
        OutOfResources,
        UnknownError
    };

    QSystemSemaphore(const QString &key, int initialValue = 0, AccessMode mode = Open);
    ~QSystemSemaphore();

    void setKey(const QString &key, int initialValue = 0, AccessMode mode = Open);
    QString key() const;

    bool acquire();
    bool release(int n = 1);

    SystemSemaphoreError error() const;
    QString errorString() const;

private:
    Q_DISABLE_COPY(QSystemSemaphore)
    QScopedPointer<QSystemSemaphorePrivate> d;
};

#endif // QT_CONFIG(systemsemaphore)

QT_END_NAMESPACE

#endif // QSYSTEMSEMAPHORE_H

