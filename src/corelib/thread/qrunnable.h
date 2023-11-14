// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
