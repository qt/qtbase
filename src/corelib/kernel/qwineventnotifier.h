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

#ifndef QWINEVENTNOTIFIER_H
#define QWINEVENTNOTIFIER_H

#include "QtCore/qobject.h"

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)

QT_BEGIN_NAMESPACE

class QWinEventNotifierPrivate;
class Q_CORE_EXPORT QWinEventNotifier : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QWinEventNotifier)
    typedef Qt::HANDLE HANDLE;

public:
    explicit QWinEventNotifier(QObject *parent = nullptr);
    explicit QWinEventNotifier(HANDLE hEvent, QObject *parent = nullptr);
    ~QWinEventNotifier();

    void setHandle(HANDLE hEvent);
    HANDLE handle() const;

    bool isEnabled() const;

public Q_SLOTS:
    void setEnabled(bool enable);

Q_SIGNALS:
    void activated(HANDLE hEvent, QPrivateSignal);

protected:
    bool event(QEvent *e) override;
};

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QWINEVENTNOTIFIER_H
