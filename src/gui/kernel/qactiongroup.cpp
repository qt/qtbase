// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qactiongroup.h"

#include "qaction.h"
#include "qaction_p.h"
#include "qactiongroup_p.h"
#include "qevent.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

QActionGroupPrivate::QActionGroupPrivate() :
    enabled(1), visible(1)
{
}

QActionGroupPrivate::~QActionGroupPrivate() = default;

void QActionGroup::_q_actionChanged()
{
    Q_D(QActionGroup);
    auto action = qobject_cast<QAction*>(sender());
    Q_ASSERT_X(action != nullptr, "QActionGroup::_q_actionChanged", "internal error");
    if (d->exclusionPolicy != QActionGroup::ExclusionPolicy::None) {
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

void QActionGroup::_q_actionTriggered()
{
    auto action = qobject_cast<QAction*>(sender());
    Q_ASSERT_X(action != nullptr, "QActionGroup::_q_actionTriggered", "internal error");
    emit triggered(action);
}

void QActionGroup::_q_actionHovered()
{
    auto action = qobject_cast<QAction*>(sender());
    Q_ASSERT_X(action != nullptr, "QActionGroup::_q_actionHovered", "internal error");
    emit hovered(action);
}

/*!
    \class QActionGroup
    \brief The QActionGroup class groups actions together.
    \since 6.0

    \inmodule QtGui

    QActionGroup is a base class for classes grouping
    classes inhheriting QAction objects together.

    In some situations it is useful to group QAction objects together.
    For example, if you have a \uicontrol{Left Align} action, a \uicontrol{Right
    Align} action, a \uicontrol{Justify} action, and a \uicontrol{Center} action,
    only one of these actions should be active at any one time. One
    simple way of achieving this is to group the actions together in
    an action group, inheriting QActionGroup.

    \sa QAction
*/

/*!
    \enum QActionGroup::ExclusionPolicy

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
    setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional)
*/
QActionGroup::QActionGroup(QObject* parent) :
    QActionGroup(*new QActionGroupPrivate, parent)
{
}

QActionGroup::QActionGroup(QActionGroupPrivate &dd, QObject *parent) :
    QObject(dd, parent)
{
}

/*!
    Destroys the action group.
*/
QActionGroup::~QActionGroup() = default;

/*!
    \fn QAction *QActionGroup::addAction(QAction *action)

    Adds the \a action to this group, and returns it.

    Normally an action is added to a group by creating it with the
    group as its parent, so this function is not usually used.

    \sa QAction::setActionGroup()
*/
QAction *QActionGroup::addAction(QAction* a)
{
    Q_D(QActionGroup);
    if (!d->actions.contains(a)) {
        d->actions.append(a);
        QObject::connect(a, &QAction::triggered, this, &QActionGroup::_q_actionTriggered);
        QObject::connect(a, &QAction::changed, this, &QActionGroup::_q_actionChanged);
        QObject::connect(a, &QAction::hovered, this, &QActionGroup::_q_actionHovered);
    }
    a->d_func()->setEnabled(d->enabled, true);
    if (!a->d_func()->forceInvisible)
        a->d_func()->setVisible(d->visible);
    if (a->isChecked())
        d->current = a;
    QActionGroup *oldGroup = a->d_func()->group;
    if (oldGroup != this) {
        if (oldGroup)
            oldGroup->removeAction(a);
        a->d_func()->group = this;
        a->d_func()->sendDataChanged();
    }
    return a;
}

/*!
    Creates and returns an action with \a text.  The newly created
    action is a child of this action group.

    Normally an action is added to a group by creating it with the
    group as parent, so this function is not usually used.

    \sa QAction::setActionGroup()
*/
QAction *QActionGroup::addAction(const QString &text)
{
    return new QAction(text, this);
}

/*!
    Creates and returns an action with \a text and an \a icon. The
    newly created action is a child of this action group.

    Normally an action is added to a group by creating it with the
    group as its parent, so this function is not usually used.

    \sa QAction::setActionGroup()
*/
QAction *QActionGroup::addAction(const QIcon &icon, const QString &text)
{
    return new QAction(icon, text, this);
}

/*!
  Removes the \a action from this group. The action will have no
  parent as a result.

  \sa QAction::setActionGroup()
*/
void QActionGroup::removeAction(QAction *action)
{
    Q_D(QActionGroup);
    if (d->actions.removeAll(action)) {
        if (action == d->current)
            d->current = nullptr;
        QObject::disconnect(action, &QAction::triggered, this, &QActionGroup::_q_actionTriggered);
        QObject::disconnect(action, &QAction::changed, this, &QActionGroup::_q_actionChanged);
        QObject::disconnect(action, &QAction::hovered, this, &QActionGroup::_q_actionHovered);
        action->d_func()->group = nullptr;
    }
}

/*!
    Returns the list of this groups's actions. This may be empty.
*/
QList<QAction*> QActionGroup::actions() const
{
    Q_D(const QActionGroup);
    return d->actions;
}

/*!
    \brief Enable or disable the group exclusion checking

    This is a convenience method that calls
    setExclusionPolicy(ExclusionPolicy::Exclusive) when \a b is true,
    else setExclusionPolicy(QActionGroup::ExclusionPolicy::None).

    \sa QActionGroup::exclusionPolicy
*/
void QActionGroup::setExclusive(bool b)
{
    setExclusionPolicy(b ? QActionGroup::ExclusionPolicy::Exclusive
                         : QActionGroup::ExclusionPolicy::None);
}

/*!
    \brief Returns true if the group is exclusive

    The group is exclusive if the ExclusionPolicy is either Exclusive
    or ExclusionOptional.

*/
bool QActionGroup::isExclusive() const
{
    return exclusionPolicy() != QActionGroup::ExclusionPolicy::None;
}

/*!
    \property QActionGroup::exclusionPolicy
    \brief This property holds the group exclusive checking policy

    If exclusionPolicy is set to Exclusive, only one checkable
    action in the action group can ever be active at any time. If the user
    chooses another checkable action in the group, the one they chose becomes
    active and the one that was active becomes inactive. If exclusionPolicy is
    set to ExclusionOptional the group is exclusive but the active checkable
    action in the group can be unchecked leaving the group with no actions
    checked.

    \sa QAction::checkable
*/
void QActionGroup::setExclusionPolicy(QActionGroup::ExclusionPolicy policy)
{
    Q_D(QActionGroup);
    d->exclusionPolicy = policy;
}

QActionGroup::ExclusionPolicy QActionGroup::exclusionPolicy() const
{
    Q_D(const QActionGroup);
    return d->exclusionPolicy;
}

/*!
    \fn void QActionGroup::setDisabled(bool b)

    This is a convenience function for the \l enabled property, that
    is useful for signals--slots connections. If \a b is true the
    action group is disabled; otherwise it is enabled.
*/

/*!
    \property QActionGroup::enabled
    \brief whether the action group is enabled

    Each action in the group will be enabled or disabled unless it
    has been explicitly disabled.

    \sa QAction::setEnabled()
*/
void QActionGroup::setEnabled(bool b)
{
    Q_D(QActionGroup);
    d->enabled = b;
    for (auto action : std::as_const(d->actions)) {
        action->d_func()->setEnabled(b, true);
    }
}

bool QActionGroup::isEnabled() const
{
    Q_D(const QActionGroup);
    return d->enabled;
}

/*!
  Returns the currently checked action in the group, or \nullptr if
  none are checked.
*/
QAction *QActionGroup::checkedAction() const
{
    Q_D(const QActionGroup);
    return d->current.data();
}

/*!
    \property QActionGroup::visible
    \brief whether the action group is visible

    Each action in the action group will match the visible state of
    this group unless it has been explicitly hidden.

    \sa QAction::setEnabled()
*/
void QActionGroup::setVisible(bool b)
{
    Q_D(QActionGroup);
    d->visible = b;
    for (auto action : std::as_const(d->actions)) {
        if (!action->d_func()->forceInvisible)
            action->d_func()->setVisible(b);
    }
}

bool QActionGroup::isVisible() const
{
    Q_D(const QActionGroup);
    return d->visible;
}

QT_END_NAMESPACE

#include "moc_qactiongroup.cpp"
