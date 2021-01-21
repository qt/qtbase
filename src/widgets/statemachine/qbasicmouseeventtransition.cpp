/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qbasicmouseeventtransition_p.h"
#include <QtGui/qevent.h>
#include <QtGui/qpainterpath.h>
#include <qdebug.h>
#include <private/qabstracttransition_p.h>

QT_BEGIN_NAMESPACE

/*!
  \internal
  \class QBasicMouseEventTransition
  \since 4.6
  \ingroup statemachine

  \brief The QBasicMouseEventTransition class provides a transition for Qt mouse events.
*/

class QBasicMouseEventTransitionPrivate : public QAbstractTransitionPrivate
{
    Q_DECLARE_PUBLIC(QBasicMouseEventTransition)
public:
    QBasicMouseEventTransitionPrivate();

    static QBasicMouseEventTransitionPrivate *get(QBasicMouseEventTransition *q);

    QEvent::Type eventType;
    Qt::MouseButton button;
    Qt::KeyboardModifiers modifierMask;
    QPainterPath path;
};

QBasicMouseEventTransitionPrivate::QBasicMouseEventTransitionPrivate()
{
    eventType = QEvent::None;
    button = Qt::NoButton;
}

QBasicMouseEventTransitionPrivate *QBasicMouseEventTransitionPrivate::get(QBasicMouseEventTransition *q)
{
    return q->d_func();
}

/*!
  Constructs a new mouse event transition with the given \a sourceState.
*/
QBasicMouseEventTransition::QBasicMouseEventTransition(QState *sourceState)
    : QAbstractTransition(*new QBasicMouseEventTransitionPrivate, sourceState)
{
}

/*!
  Constructs a new mouse event transition for events of the given \a type.
*/
QBasicMouseEventTransition::QBasicMouseEventTransition(QEvent::Type type,
                                                       Qt::MouseButton button,
                                                       QState *sourceState)
    : QAbstractTransition(*new QBasicMouseEventTransitionPrivate, sourceState)
{
    Q_D(QBasicMouseEventTransition);
    d->eventType = type;
    d->button = button;
}

/*!
  Destroys this mouse event transition.
*/
QBasicMouseEventTransition::~QBasicMouseEventTransition()
{
}

/*!
  Returns the event type that this mouse event transition is associated with.
*/
QEvent::Type QBasicMouseEventTransition::eventType() const
{
    Q_D(const QBasicMouseEventTransition);
    return d->eventType;
}

/*!
  Sets the event \a type that this mouse event transition is associated with.
*/
void QBasicMouseEventTransition::setEventType(QEvent::Type type)
{
    Q_D(QBasicMouseEventTransition);
    d->eventType = type;
}

/*!
  Returns the button that this mouse event transition checks for.
*/
Qt::MouseButton QBasicMouseEventTransition::button() const
{
    Q_D(const QBasicMouseEventTransition);
    return d->button;
}

/*!
  Sets the button that this mouse event transition will check for.
*/
void QBasicMouseEventTransition::setButton(Qt::MouseButton button)
{
    Q_D(QBasicMouseEventTransition);
    d->button = button;
}

/*!
  Returns the keyboard modifier mask that this mouse event transition checks
  for.
*/
Qt::KeyboardModifiers QBasicMouseEventTransition::modifierMask() const
{
    Q_D(const QBasicMouseEventTransition);
    return d->modifierMask;
}

/*!
  Sets the keyboard modifier mask that this mouse event transition will check
  for.
*/
void QBasicMouseEventTransition::setModifierMask(Qt::KeyboardModifiers modifierMask)
{
    Q_D(QBasicMouseEventTransition);
    d->modifierMask = modifierMask;
}

/*!
  Returns the hit test path for this mouse event transition.
*/
QPainterPath QBasicMouseEventTransition::hitTestPath() const
{
    Q_D(const QBasicMouseEventTransition);
    return d->path;
}

/*!
  Sets the hit test path for this mouse event transition.
*/
void QBasicMouseEventTransition::setHitTestPath(const QPainterPath &path)
{
    Q_D(QBasicMouseEventTransition);
    d->path = path;
}

/*!
  \reimp
*/
bool QBasicMouseEventTransition::eventTest(QEvent *event)
{
    Q_D(const QBasicMouseEventTransition);
    if (event->type() == d->eventType) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        return (me->button() == d->button)
            && ((me->modifiers() & d->modifierMask) == d->modifierMask)
            && (d->path.isEmpty() || d->path.contains(me->pos()));
    }
    return false;
}

/*!
  \reimp
*/
void QBasicMouseEventTransition::onTransition(QEvent *)
{
}

QT_END_NAMESPACE

#include "moc_qbasicmouseeventtransition_p.cpp"
