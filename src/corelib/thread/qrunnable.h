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

#ifndef QRUNNABLE_H
#define QRUNNABLE_H

#include <QtCore/qglobal.h>
#include <functional>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRunnable
{
    int ref; // Qt6: Make this a bool, or make autoDelete() virtual.

    friend class QThreadPool;
    friend class QThreadPoolPrivate;
    friend class QThreadPoolThread;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    Q_DISABLE_COPY(QRunnable)
#endif
public:
    virtual void run() = 0;

    QRunnable() : ref(0) { }
    virtual ~QRunnable();
    static QRunnable *create(std::function<void()> functionToRun);

    bool autoDelete() const { return ref != -1; }
    void setAutoDelete(bool _autoDelete) { ref = _autoDelete ? 0 : -1; }
};

QT_END_NAMESPACE

#endif
