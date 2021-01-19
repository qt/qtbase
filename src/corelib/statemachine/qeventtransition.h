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

#ifndef QEVENTTRANSITION_H
#define QEVENTTRANSITION_H

#include <QtCore/qabstracttransition.h>
#include <QtCore/qcoreevent.h>

QT_REQUIRE_CONFIG(qeventtransition);

QT_BEGIN_NAMESPACE

class QEventTransitionPrivate;
class Q_CORE_EXPORT QEventTransition : public QAbstractTransition
{
    Q_OBJECT
    Q_PROPERTY(QObject* eventSource READ eventSource WRITE setEventSource)
    Q_PROPERTY(QEvent::Type eventType READ eventType WRITE setEventType)
public:
    QEventTransition(QState *sourceState = nullptr);
    QEventTransition(QObject *object, QEvent::Type type, QState *sourceState = nullptr);
    ~QEventTransition();

    QObject *eventSource() const;
    void setEventSource(QObject *object);

    QEvent::Type eventType() const;
    void setEventType(QEvent::Type type);

protected:
    bool eventTest(QEvent *event) override;
    void onTransition(QEvent *event) override;

    bool event(QEvent *e) override;

protected:
    QEventTransition(QEventTransitionPrivate &dd, QState *parent);
    QEventTransition(QEventTransitionPrivate &dd, QObject *object,
                     QEvent::Type type, QState *parent);

private:
    Q_DISABLE_COPY(QEventTransition)
    Q_DECLARE_PRIVATE(QEventTransition)
};

QT_END_NAMESPACE

#endif
