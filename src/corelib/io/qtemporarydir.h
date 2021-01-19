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

#ifndef QTEMPORARYDIR_H
#define QTEMPORARYDIR_H

#include <QtCore/qdir.h>
#include <QtCore/qscopedpointer.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TEMPORARYFILE

class QTemporaryDirPrivate;

class Q_CORE_EXPORT QTemporaryDir
{
public:
    QTemporaryDir();
    explicit QTemporaryDir(const QString &templateName);
    ~QTemporaryDir();

    bool isValid() const;
    QString errorString() const;

    bool autoRemove() const;
    void setAutoRemove(bool b);
    bool remove();

    QString path() const;
    QString filePath(const QString &fileName) const;

private:
    QScopedPointer<QTemporaryDirPrivate> d_ptr;

    Q_DISABLE_COPY(QTemporaryDir)
};

#endif // QT_NO_TEMPORARYFILE

QT_END_NAMESPACE

#endif // QTEMPORARYDIR_H
