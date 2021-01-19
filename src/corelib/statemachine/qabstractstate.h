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

#ifndef QABSTRACTSTATE_H
#define QABSTRACTSTATE_H

#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(statemachine);

QT_BEGIN_NAMESPACE

class QState;
class QStateMachine;

class QAbstractStatePrivate;
class Q_CORE_EXPORT QAbstractState : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
public:
    ~QAbstractState();

    QState *parentState() const;
    QStateMachine *machine() const;

    bool active() const;

Q_SIGNALS:
    void entered(QPrivateSignal);
    void exited(QPrivateSignal);
    void activeChanged(bool active);

protected:
    QAbstractState(QState *parent = nullptr);

    virtual void onEntry(QEvent *event) = 0;
    virtual void onExit(QEvent *event) = 0;

    bool event(QEvent *e) override;

protected:
    QAbstractState(QAbstractStatePrivate &dd, QState *parent);

private:
    Q_DISABLE_COPY(QAbstractState)
    Q_DECLARE_PRIVATE(QAbstractState)
};

QT_END_NAMESPACE

#endif
