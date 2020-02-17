/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qguiactiongroup.h"

#include "qguiaction.h"
#include "qguiaction_p.h"
#include "qguiactiongroup_p.h"
#include "qevent.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

QGuiActionGroupPrivate::QGuiActionGroupPrivate() :
    enabled(1), visible(1)
{
}

QGuiActionGroupPrivate::~QGuiActionGroupPrivate() = default;

void QGuiActionGroup::_q_actionChanged()
{
    Q_D(QGuiActionGroup);
    auto action = qobject_cast<QGuiAction*>(sender());
    Q_ASSERT_X(action != nullptr, "QGuiActionGroup::_q_actionChanged", "internal error");
    if (d->exclusionPolicy != QGuiActionGroup::ExclusionPolicy::None) {
        if (action->isChecked()) {
            if (action != d->current) {
                if (!d->current.isNull())
                    d->current->setChecked(false);
                d->current = action;
            }
        } else if (action == d->current) {
            d->current = nullptr;
        }
    }
}

void QGuiActionGroup::_q_actionTriggered()
{
    Q_D(QGuiActionGroup);
    auto action = qobject_cast<QGuiAction*>(sender());
    Q_ASSERT_X(action != nullptr, "QGuiActionGroup::_q_actionTriggered", "internal error");
    d->emitSignal(QGuiActionGroupPrivate::Triggered, action);
}

void QGuiActionGroup::_q_actionHovered()
{
    Q_D(QGuiActionGroup);
    auto action = qobject_cast<QGuiAction*>(sender());
    Q_ASSERT_X(action != nullptr, "QGuiActionGroup::_q_actionHovered", "internal error");
    d->emitSignal(QGuiActionGroupPrivate::Hovered, action);
}

/*!
    \class QGuiActionGroup
    \brief The QGuiActionGroup class groups actions together.
    \since 6.0

    \inmodule QtGui

    QGuiActionGroup is a base class for classes grouping
    classes inhheriting QGuiAction objects together.

    In some situations it is useful to group QGuiAction objects together.
    For example, if you have a \uicontrol{Left Align} action, a \uicontrol{Right
    Align} action, a \uicontrol{Justify} action, and a \uicontrol{Center} action,
    only one of these actions should be active at any one time. One
    simple way of achieving this is to group the actions together in
    an action group, inheriting QGuiActionGroup.

    \sa QGuiAction
*/

/*!
    \enum QGuiActionGroup::ExclusionPolicy

    This enum specifies the different policies that can be used to
    control how the group performs exclusive checking on checkable actions.

    \value None
           The actions in the group can be checked independently of each other.

    \value Exclusive
           Exactly one action can be checked at any one time.
           This is the default policy.

    \value ExclusiveOptional
           At most one action can be checked at any one time. The actions
           can also be all unchecked.

    \sa exclusionPolicy
*/

/*!
    Constructs an action group for the \a parent object.

    The action group is exclusive by default. Call setExclusive(false)
    to make the action group non-exclusive. To make the group exclusive
    but allow unchecking the active action call instead
    setExclusionPolicy(QGuiActionGroup::ExclusionPolicy::ExclusiveOptional)
*/
QGuiActionGroup::QGuiActionGroup(QObject* parent) :
    QGuiActionGroup(*new QGuiActionGroupPrivate, parent)
{
}

QGuiActionGroup::QGuiActionGroup(QGuiActionGroupPrivate &dd, QObject *parent) :
    QObject(dd, parent)
{
}

/*!
    Destroys the action group.
*/
QGuiActionGroup::~QGuiActionGroup() = default;

/*!
    \fn QGuiAction *QGuiActionGroup::addAction(QGuiAction *action)

    Adds the \a action to this group, and returns it.

    Normally an action is added to a group by creating it with the
    group as its parent, so this function is not usually used.

    \sa QGuiAction::setActionGroup()
*/
QGuiAction *QGuiActionGroup::addAction(QGuiAction* a)
{
    Q_D(QGuiActionGroup);
    if (!d->actions.contains(a)) {
        d->actions.append(a);
        QObject::connect(a, &QGuiAction::triggered, this, &QGuiActionGroup::_q_actionTriggered);
        QObject::connect(a, &QGuiAction::changed, this, &QGuiActionGroup::_q_actionChanged);
        QObject::connect(a, &QGuiAction::hovered, this, &QGuiActionGroup::_q_actionHovered);
    }
    a->d_func()->setEnabled(d->enabled, true);
    if (!a->d_func()->forceInvisible) {
        a->setVisible(d->visible);
        a->d_func()->forceInvisible = false;
    }
    if (a->isChecked())
        d->current = a;
    QGuiActionGroup *oldGroup = a->d_func()->group;
    if (oldGroup != this) {
        if (oldGroup)
            oldGroup->removeAction(a);
        a->d_func()->group = this;
        a->d_func()->sendDataChanged();
    }
    return a;
}

/*!
  Removes the \a action from this group. The action will have no
  parent as a result.

  \sa QGuiAction::setActionGroup()
*/
void QGuiActionGroup::removeAction(QGuiAction *action)
{
    Q_D(QGuiActionGroup);
    if (d->actions.removeAll(action)) {
        if (action == d->current)
            d->current = nullptr;
        QObject::disconnect(action, &QGuiAction::triggered, this, &QGuiActionGroup::_q_actionTriggered);
        QObject::disconnect(action, &QGuiAction::changed, this, &QGuiActionGroup::_q_actionChanged);
        QObject::disconnect(action, &QGuiAction::hovered, this, &QGuiActionGroup::_q_actionHovered);
        action->d_func()->group = nullptr;
    }
}

/*!
    Returns the list of this groups's actions. This may be empty.
*/
QList<QGuiAction*> QGuiActionGroup::guiActions() const
{
    Q_D(const QGuiActionGroup);
    return d->actions;
}

/*!
    \brief Enable or disable the group exclusion checking

    This is a convenience method that calls
    setExclusionPolicy(ExclusionPolicy::Exclusive) when \a b is true,
    else setExclusionPolicy(QActionGroup::ExclusionPolicy::None).

    \sa QGuiActionGroup::exclusionPolicy
*/
void QGuiActionGroup::setExclusive(bool b)
{
    setExclusionPolicy(b ? QGuiActionGroup::ExclusionPolicy::Exclusive
                         : QGuiActionGroup::ExclusionPolicy::None);
}

/*!
    \brief Returns true if the group is exclusive

    The group is exclusive if the ExclusionPolicy is either Exclusive
    or ExclusionOptional.

*/
bool QGuiActionGroup::isExclusive() const
{
    return exclusionPolicy() != QGuiActionGroup::ExclusionPolicy::None;
}

/*!
    \property QGuiActionGroup::exclusionPolicy
    \brief This property holds the group exclusive checking policy

    If exclusionPolicy is set to Exclusive, only one checkable
    action in the action group can ever be active at any time. If the user
    chooses another checkable action in the group, the one they chose becomes
    active and the one that was active becomes inactive. If exclusionPolicy is
    set to ExclusionOptional the group is exclusive but the active checkable
    action in the group can be unchecked leaving the group with no actions
    checked.

    \sa QGuiAction::checkable
*/
void QGuiActionGroup::setExclusionPolicy(QGuiActionGroup::ExclusionPolicy policy)
{
    Q_D(QGuiActionGroup);
    d->exclusionPolicy = policy;
}

QGuiActionGroup::ExclusionPolicy QGuiActionGroup::exclusionPolicy() const
{
    Q_D(const QGuiActionGroup);
    return d->exclusionPolicy;
}

/*!
    \fn void QGuiActionGroup::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action group is disabled; otherwise it is enabled.
*/

/*!
    \property QGuiActionGroup::enabled
    \brief whether the action group is enabled

    Each action in the group will be enabled or disabled unless it
    has been explicitly disabled.

    \sa QGuiAction::setEnabled()
*/
void QGuiActionGroup::setEnabled(bool b)
{
    Q_D(QGuiActionGroup);
    d->enabled = b;
    for (auto action : qAsConst(d->actions)) {
        action->d_func()->setEnabled(b, true);
    }
}

bool QGuiActionGroup::isEnabled() const
{
    Q_D(const QGuiActionGroup);
    return d->enabled;
}

/*!
  Returns the currently checked action in the group, or \nullptr if
  none are checked.
*/
QGuiAction *QGuiActionGroup::checkedGuiAction() const
{
    Q_D(const QGuiActionGroup);
    return d->current.data();
}

/*!
    \property QGuiActionGroup::visible
    \brief whether the action group is visible

    Each action in the action group will match the visible state of
    this group unless it has been explicitly hidden.

    \sa QGuiAction::setEnabled()
*/
void QGuiActionGroup::setVisible(bool b)
{
    Q_D(QGuiActionGroup);
    d->visible = b;
    for (auto action : qAsConst(d->actions)) {
        if (!action->d_func()->forceInvisible) {
            action->setVisible(b);
            action->d_func()->forceInvisible = false;
        }
    }
}

bool QGuiActionGroup::isVisible() const
{
    Q_D(const QGuiActionGroup);
    return d->visible;
}

QT_END_NAMESPACE
