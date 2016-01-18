/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qanimationstate.h"

#include <QtCore/qstate.h>

QT_BEGIN_NAMESPACE

/*!
\class QAnimationState

\brief The QAnimationState class provides state that handle an animation and emit
a signal when this animation is finished.

\ingroup statemachine

QAnimationState provides a state that handle an animation. It will start this animation
when the state is entered and stop it when it is leaved. When the animation has finished the
state emit animationFinished signal.
QAnimationState is part of \l{The State Machine Framework}.

\code
QStateMachine machine;
QAnimationState *s = new QAnimationState(machine->rootState());
QPropertyAnimation *animation = new QPropertyAnimation(obj, "pos");
s->setAnimation(animation);
QState *s2 = new QState(machine->rootState());
s->addTransition(s, SIGNAL(animationFinished()), s2);
machine.start();
\endcode

\sa QState, {The Animation Framework}
*/


#ifndef QT_NO_ANIMATION

/*!
  Constructs a new state with the given \a parent state.
*/
QAnimationState::QAnimationState(QState *parent)
    : QState(parent), m_animation(0)
{
}

/*!
  Destroys the animation state.
*/
QAnimationState::~QAnimationState()
{
}

/*!
  Set an \a animation for this QAnimationState. If an animation was previously handle by this
  state then it won't emit animationFinished for the old animation. The QAnimationState doesn't
  take the ownership of the animation.
*/
void QAnimationState::setAnimation(QAbstractAnimation *animation)
{
    if (animation == m_animation)
        return;

    //Disconnect from the previous animation if exist
    if(m_animation)
        disconnect(m_animation, SIGNAL(finished()), this, SIGNAL(animationFinished()));

    m_animation = animation;

    if (m_animation) {
        //connect the new animation
        connect(m_animation, SIGNAL(finished()), this, SIGNAL(animationFinished()));
    }
}

/*!
  Returns the animation handle by this animation state, or 0 if there is no animation.
*/
QAbstractAnimation* QAnimationState::animation() const
{
    return m_animation;
}

/*!
  \reimp
*/
void QAnimationState::onEntry(QEvent *)
{
    if (m_animation)
        m_animation->start();
}

/*!
  \reimp
*/
void QAnimationState::onExit(QEvent *)
{
    if (m_animation)
        m_animation->stop();
}

/*!
  \reimp
*/
bool QAnimationState::event(QEvent *e)
{
    return QState::event(e);
}

QT_END_NAMESPACE

#endif
