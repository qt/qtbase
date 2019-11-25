/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qactiongroup.h"
#include <QtGui/private/qguiactiongroup_p.h>

#include "qaction.h"

QT_BEGIN_NAMESPACE

class QActionGroupPrivate : public QGuiActionGroupPrivate
{
    Q_DECLARE_PUBLIC(QActionGroup)
public:
     void emitSignal(Signal, QGuiAction *) override;
};

void QActionGroupPrivate::emitSignal(Signal s, QGuiAction *action)
{
    Q_Q(QActionGroup);
    switch (s) {
    case QGuiActionGroupPrivate::Triggered:
        emit q->triggered(static_cast<QAction *>(action));
        break;
    case QGuiActionGroupPrivate::Hovered:
        emit q->hovered(static_cast<QAction *>(action));
        break;
    }
}

/*!
    \class QActionGroup
    \brief The QActionGroup class groups actions together.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    In some situations it is useful to group QAction objects together.
    For example, if you have a \uicontrol{Left Align} action, a \uicontrol{Right
    Align} action, a \uicontrol{Justify} action, and a \uicontrol{Center} action,
    only one of these actions should be active at any one time. One
    simple way of achieving this is to group the actions together in
    an action group.

    Here's a example (from the \l{mainwindows/menus}{Menus} example):

    \snippet mainwindows/menus/mainwindow.cpp 6

    Here we create a new action group. Since the action group is
    exclusive by default, only one of the actions in the group is
    checked at any one time.

    \image qactiongroup-align.png Alignment options in a QMenu

    A QActionGroup emits an triggered() signal when one of its
    actions is chosen. Each action in an action group emits its
    triggered() signal as usual.

    As stated above, an action group is exclusive by default; it
    ensures that at most only one checkable action is active at any one time.
    If you want to group checkable actions without making them
    exclusive, you can turn off exclusiveness by calling
    setExclusive(false).

    By default the active action of an exclusive group cannot be unchecked.
    In some cases it may be useful to allow unchecking all the actions,
    you can allow this by calling
    setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional).

    Actions can be added to an action group using addAction(), but it
    is usually more convenient to specify a group when creating
    actions; this ensures that actions are automatically created with
    a parent. Actions can be visually separated from each other by
    adding a separator action to the group; create an action and use
    QAction's \l {QAction::}{setSeparator()} function to make it
    considered a separator. Action groups are added to widgets with
    the QWidget::addActions() function.

    \sa QAction
*/

/*!
    Constructs an action group for the \a parent object.

    The action group is exclusive by default. Call setExclusive(false)
    to make the action group non-exclusive. To make the group exclusive
    but allow unchecking the active action call instead
    setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional)
*/
QActionGroup::QActionGroup(QObject* parent) :
    QGuiActionGroup(*new QActionGroupPrivate, parent)
{
}

/*!
    Destroys the action group.
*/
QActionGroup::~QActionGroup() = default;

QAction *QActionGroup::checkedAction() const
{
    return static_cast<QAction *>(checkedGuiAction());
}

QAction *QActionGroup::addAction(QAction *a)
{
    QGuiActionGroup::addAction(a);
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
    Returns the list of this groups's actions. This may be empty.
*/
QList<QAction*> QActionGroup::actions() const
{
    QList<QAction*> result;
    const auto baseActions = guiActions();
    result.reserve(baseActions.size());
    for (auto baseAction : baseActions)
        result.append(static_cast<QAction*>(baseAction));
    return result;
}

QT_END_NAMESPACE
