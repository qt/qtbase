/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qabstractstate.h"
#include "qabstractstate_p.h"
#include "qstate.h"
#include "qstate_p.h"
#include "qstatemachine.h"
#include "qstatemachine_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QAbstractState
  \inmodule QtCore

  \brief The QAbstractState class is the base class of states of a QStateMachine.

  \since 4.6
  \ingroup statemachine

  The QAbstractState class is the abstract base class of states that are part
  of a QStateMachine. It defines the interface that all state objects have in
  common. QAbstractState is part of \l{The State Machine Framework}.

  The entered() signal is emitted when the state has been entered. The
  exited() signal is emitted when the state has been exited.

  The parentState() function returns the state's parent state. The machine()
  function returns the state machine that the state is part of.

  \section1 Subclassing

  The onEntry() function is called when the state is entered; reimplement this
  function to perform custom processing when the state is entered.

  The onExit() function is called when the state is exited; reimplement this
  function to perform custom processing when the state is exited.
*/

/*!
    \property QAbstractState::active
    \since 5.4

    \brief the active property of this state. A state is active between
    entered() and exited() signals.
*/


QAbstractStatePrivate::QAbstractStatePrivate(StateType type)
    : stateType(type), isMachine(false), active(false), parentState(0)
{
}

QStateMachine *QAbstractStatePrivate::machine() const
{
    QObject *par = parent;
    while (par != 0) {
        if (QStateMachine *mach = qobject_cast<QStateMachine*>(par))
            return mach;
        par = par->parent();
    }
    return 0;
}

void QAbstractStatePrivate::callOnEntry(QEvent *e)
{
    Q_Q(QAbstractState);
    q->onEntry(e);
}

void QAbstractStatePrivate::callOnExit(QEvent *e)
{
    Q_Q(QAbstractState);
    q->onExit(e);
}

void QAbstractStatePrivate::emitEntered()
{
    Q_Q(QAbstractState);
    emit q->entered(QAbstractState::QPrivateSignal());
    if (!active) {
        active = true;
        emit q->activeChanged(true);
    }
}

void QAbstractStatePrivate::emitExited()
{
    Q_Q(QAbstractState);
    if (active) {
        active = false;
        emit q->activeChanged(false);
    }
    emit q->exited(QAbstractState::QPrivateSignal());
}

/*!
  Constructs a new state with the given \a parent state.
*/
QAbstractState::QAbstractState(QState *parent)
    : QObject(*new QAbstractStatePrivate(QAbstractStatePrivate::AbstractState), parent)
{
}

/*!
  \internal
*/
QAbstractState::QAbstractState(QAbstractStatePrivate &dd, QState *parent)
    : QObject(dd, parent)
{
}

/*!
  Destroys this state.
*/
QAbstractState::~QAbstractState()
{
}

/*!
  Returns this state's parent state, or 0 if the state has no parent state.
*/
QState *QAbstractState::parentState() const
{
    Q_D(const QAbstractState);
    if (d->parentState != parent())
        d->parentState = qobject_cast<QState*>(parent());
    return d->parentState;
}

/*!
  Returns the state machine that this state is part of, or 0 if the state is
  not part of a state machine.
*/
QStateMachine *QAbstractState::machine() const
{
    Q_D(const QAbstractState);
    return d->machine();
}

/*!
  Returns whether this state is active.

  \sa activeChanged(bool), entered(), exited()
*/
bool QAbstractState::active() const
{
    Q_D(const QAbstractState);
    return d->active;
}

/*!
  \fn QAbstractState::onExit(QEvent *event)

  This function is called when the state is exited. The given \a event is what
  caused the state to be exited. Reimplement this function to perform custom
  processing when the state is exited.
*/

/*!
  \fn QAbstractState::onEntry(QEvent *event)

  This function is called when the state is entered. The given \a event is
  what caused the state to be entered. Reimplement this function to perform
  custom processing when the state is entered.
*/

/*!
  \fn QAbstractState::entered()

  This signal is emitted when the state has been entered (after onEntry() has
  been called).
*/

/*!
  \fn QAbstractState::exited()

  This signal is emitted when the state has been exited (after onExit() has
  been called).
*/

/*!
  \fn QAbstractState::activeChanged(bool active)
  \since 5.4

  This signal is emitted when the active property is changed with \a active as argument.

  \sa QAbstractState::active, entered(), exited()
*/

/*!
  \reimp
*/
bool QAbstractState::event(QEvent *e)
{
    return QObject::event(e);
}

QT_END_NAMESPACE

#include "moc_qabstractstate.cpp"
