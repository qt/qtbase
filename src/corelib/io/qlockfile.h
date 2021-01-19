/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
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

#ifndef QLOCKFILE_H
#define QLOCKFILE_H

#include <QtCore/qstring.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE

class QLockFilePrivate;

class Q_CORE_EXPORT QLockFile
{
public:
    QLockFile(const QString &fileName);
    ~QLockFile();

    bool lock();
    bool tryLock(int timeout = 0);
    void unlock();

    void setStaleLockTime(int);
    int staleLockTime() const;

    bool isLocked() const;
    bool getLockInfo(qint64 *pid, QString *hostname, QString *appname) const;
    bool removeStaleLockFile();

    enum LockError {
        NoError = 0,
        LockFailedError = 1,
        PermissionError = 2,
        UnknownError = 3
    };
    LockError error() const;

protected:
    QScopedPointer<QLockFilePrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(QLockFile)
    Q_DISABLE_COPY(QLockFile)
};

QT_END_NAMESPACE

#endif // QLOCKFILE_H
