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

#ifndef QABSTRACTTRANSITION_P_H
#define QABSTRACTTRANSITION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qobject_p.h>

#include <QtCore/qlist.h>
#include <QtCore/qvector.h>
#include <QtCore/qsharedpointer.h>

QT_REQUIRE_CONFIG(statemachine);

QT_BEGIN_NAMESPACE

class QAbstractState;
class QState;
class QStateMachine;

class QAbstractTransition;
class Q_CORE_EXPORT QAbstractTransitionPrivate
    : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QAbstractTransition)
public:
    QAbstractTransitionPrivate();

    static QAbstractTransitionPrivate *get(QAbstractTransition *q)
    { return q->d_func(); }

    bool callEventTest(QEvent *e);
    virtual void callOnTransition(QEvent *e);
    QState *sourceState() const;
    QStateMachine *machine() const;
    void emitTriggered();

    QVector<QPointer<QAbstractState> > targetStates;
    QAbstractTransition::TransitionType transitionType;

#if QT_CONFIG(animation)
    QList<QAbstractAnimation*> animations;
#endif
};

QT_END_NAMESPACE

#endif
