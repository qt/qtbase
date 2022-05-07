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

#ifndef QRUNNABLE_H
#define QRUNNABLE_H

#include <QtCore/qglobal.h>
#include <functional>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRunnable
{
    bool m_autoDelete = true;

    Q_DISABLE_COPY(QRunnable)
public:
    virtual void run() = 0;

    constexpr QRunnable() noexcept = default;
    virtual ~QRunnable();
    static QRunnable *create(std::function<void()> functionToRun);

    bool autoDelete() const { return m_autoDelete; }
    void setAutoDelete(bool autoDelete) { m_autoDelete = autoDelete; }
};

QT_END_NAMESPACE

#endif
