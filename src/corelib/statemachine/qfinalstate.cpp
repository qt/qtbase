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

#include "qfinalstate_p.h"

QT_BEGIN_NAMESPACE

/*!
  \class QFinalState
  \inmodule QtCore

  \brief The QFinalState class provides a final state.

  \since 4.6
  \ingroup statemachine

  A final state is used to communicate that (part of) a QStateMachine has
  finished its work. When a final top-level state is entered, the state
  machine's \l{QStateMachine::finished()}{finished}() signal is emitted. In
  general, when a final substate (a child of a QState) is entered, the parent
  state's \l{QState::finished()}{finished}() signal is emitted.  QFinalState
  is part of \l{The State Machine Framework}.

  To use a final state, you create a QFinalState object and add a transition
  to it from another state. Example:

  \code
  QPushButton button;

  QStateMachine machine;
  QState *s1 = new QState();
  QFinalState *s2 = new QFinalState();
  s1->addTransition(&button, SIGNAL(clicked()), s2);
  machine.addState(s1);
  machine.addState(s2);

  QObject::connect(&machine, SIGNAL(finished()), QApplication::instance(), SLOT(quit()));
  machine.setInitialState(s1);
  machine.start();
  \endcode

  \sa QState::finished()
*/

QFinalStatePrivate::QFinalStatePrivate()
    : QAbstractStatePrivate(FinalState)
{
}

QFinalStatePrivate::~QFinalStatePrivate()
{
    // to prevent vtables being generated in every file that includes the private header
}

/*!
  Constructs a new QFinalState object with the given \a parent state.
*/
QFinalState::QFinalState(QState *parent)
    : QAbstractState(*new QFinalStatePrivate, parent)
{
}

/*!
  \internal
 */
QFinalState::QFinalState(QFinalStatePrivate &dd, QState *parent)
    : QAbstractState(dd, parent)
{
}


/*!
  Destroys this final state.
*/
QFinalState::~QFinalState()
{
}

/*!
  \reimp
*/
void QFinalState::onEntry(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
void QFinalState::onExit(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
  \reimp
*/
bool QFinalState::event(QEvent *e)
{
    return QAbstractState::event(e);
}

QT_END_NAMESPACE
