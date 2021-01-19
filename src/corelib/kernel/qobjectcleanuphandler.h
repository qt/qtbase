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

#ifndef QOBJECTCLEANUPHANDLER_H
#define QOBJECTCLEANUPHANDLER_H

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QObjectCleanupHandler : public QObject
{
    Q_OBJECT

public:
    QObjectCleanupHandler();
    ~QObjectCleanupHandler();

    QObject* add(QObject* object);
    void remove(QObject *object);
    bool isEmpty() const;
    void clear();

private:
    // ### move into d pointer
    QObjectList cleanupObjects;

private Q_SLOTS:
    void objectDestroyed(QObject *);
};

QT_END_NAMESPACE

#endif // QOBJECTCLEANUPHANDLER_H
