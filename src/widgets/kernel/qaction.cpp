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

#include "qaction.h"
#include "qactiongroup.h"

#include "qaction_p.h"
#include "qapplication.h"
#include "qevent.h"
#include "qlist.h"
#include "qstylehints.h"
#if QT_CONFIG(shortcut)
#  include <private/qshortcutmap_p.h>
#endif
#include <private/qguiapplication_p.h>
#if QT_CONFIG(menu)
#include <private/qmenu_p.h>
#endif
#include <private/qdebug_p.h>


QT_BEGIN_NAMESPACE

#if QT_CONFIG(shortcut)
QShortcutMap::ContextMatcher QActionPrivate::contextMatcher() const
{
    return qWidgetShortcutContextMatcher;
}
#endif // QT_CONFIG(shortcut)

bool QActionPrivate::showStatusText(QWidget *widget, const QString &str)
{
#if !QT_CONFIG(statustip)
    Q_UNUSED(widget);
    Q_UNUSED(str);
#else
    if(QObject *object = widget ? widget : parent) {
        QStatusTipEvent tip(str);
        QCoreApplication::sendEvent(object, &tip);
        return true;
    }
#endif
    return false;
}

/*!
    \class QAction
    \brief The QAction class provides an abstract user interface
    action that can be inserted into widgets.

    \ingroup mainwindow-classes
    \inmodule QtWidgets

    \omit
        * parent and widget are different
        * parent does not define context
    \endomit

    In applications many common commands can be invoked via menus,
    toolbar buttons, and keyboard shortcuts. Since the user expects
    each command to be performed in the same way, regardless of the
    user interface used, it is useful to represent each command as
    an \e action.

    Actions can be added to menus and toolbars, and will
    automatically keep them in sync. For example, in a word processor,
    if the user presses a Bold toolbar button, the Bold menu item
    will automatically be checked.

    Actions can be created as independent objects, but they may
    also be created during the construction of menus; the QMenu class
    contains convenience functions for creating actions suitable for
    use as menu items.

    A QAction may contain an icon, menu text, a shortcut, status text,
    "What's This?" text, and a tooltip. Most of these can be set in
    the constructor. They can also be set independently with
    setIcon(), setText(), setIconText(), setShortcut(),
    setStatusTip(), setWhatsThis(), and setToolTip(). For menu items,
    it is possible to set an individual font with setFont().

    Actions are added to widgets using QWidget::addAction() or
    QGraphicsWidget::addAction(). Note that an action must be added to a
    widget before it can be used; this is also true when the shortcut should
    be global (i.e., Qt::ApplicationShortcut as Qt::ShortcutContext).

    Once a QAction has been created it should be added to the relevant
    menu and toolbar, then connected to the slot which will perform
    the action. For example:

    \snippet mainwindows/application/mainwindow.cpp 19

    We recommend that actions are created as children of the window
    they are used in. In most cases actions will be children of
    the application's main window.

    \sa QMenu, QToolBar, {Application Example}
*/

/*!
    \fn void QAction::trigger()

    This is a convenience slot that calls activate(Trigger).
*/

/*!
    \fn void QAction::hover()

    This is a convenience slot that calls activate(Hover).
*/

/*!
    \enum QAction::MenuRole

    This enum describes how an action should be moved into the application menu on \macos.

    \value NoRole This action should not be put into the application menu
    \value TextHeuristicRole This action should be put in the application menu based on the action's text
           as described in the QMenuBar documentation.
    \value ApplicationSpecificRole This action should be put in the application menu with an application specific role
    \value AboutQtRole This action handles the "About Qt" menu item.
    \value AboutRole This action should be placed where the "About" menu item is in the application menu. The text of
           the menu item will be set to "About <application name>". The application name is fetched from the
           \c{Info.plist} file in the application's bundle (See \l{Qt for macOS - Deployment}).
    \value PreferencesRole This action should be placed where the  "Preferences..." menu item is in the application menu.
    \value QuitRole This action should be placed where the Quit menu item is in the application menu.

    Setting this value only has effect on items that are in the immediate menus
    of the menubar, not the submenus of those menus. For example, if you have
    File menu in your menubar and the File menu has a submenu, setting the
    MenuRole for the actions in that submenu have no effect. They will never be moved.
*/

/*!
    Constructs an action with \a parent. If \a parent is an action
    group the action will be automatically inserted into the group.

    \note The \a parent argument is optional since Qt 5.7.
*/
QAction::QAction(QObject* parent)
    : QAction(*new QActionPrivate, parent)
{
}


/*!
    Constructs an action with some \a text and \a parent. If \a
    parent is an action group the action will be automatically
    inserted into the group.

    The action uses a stripped version of \a text (e.g. "\&Menu
    Option..." becomes "Menu Option") as descriptive text for
    tool buttons. You can override this by setting a specific
    description with setText(). The same text will be used for
    tooltips unless you specify a different text using
    setToolTip().

*/
QAction::QAction(const QString &text, QObject* parent)
    : QAction(parent)
{
    Q_D(QAction);
    d->text = text;
}

/*!
    Constructs an action with an \a icon and some \a text and \a
    parent. If \a parent is an action group the action will be
    automatically inserted into the group.

    The action uses a stripped version of \a text (e.g. "\&Menu
    Option..." becomes "Menu Option") as descriptive text for
    tool buttons. You can override this by setting a specific
    description with setText(). The same text will be used for
    tooltips unless you specify a different text using
    setToolTip().
*/
QAction::QAction(const QIcon &icon, const QString &text, QObject* parent)
    : QAction(text, parent)
{
    Q_D(QAction);
    d->icon = icon;
}

/*!
    \internal
*/
QAction::QAction(QActionPrivate &dd, QObject *parent)
    : QGuiAction(dd, parent)
{
}

/*!
  \reimp
*/

bool QAction::event(QEvent *e)
{
    Q_D(QAction);
    if (e->type() == QEvent::ActionChanged) {
        for (auto w : qAsConst(d->widgets))
            QCoreApplication::sendEvent(w, e);
#if QT_CONFIG(graphicsview)
        for (auto gw :  qAsConst(d->graphicsWidgets))
            QCoreApplication::sendEvent(gw, e);
#endif
    }
    return QGuiAction::event(e);
}

/*!
    Returns the parent widget.
*/
QWidget *QAction::parentWidget() const
{
    QObject *ret = parent();
    while (ret && !ret->isWidgetType())
        ret = ret->parent();
    return static_cast<QWidget*>(ret);
}

/*!
  \since 4.2
  Returns a list of widgets this action has been added to.

  \sa QWidget::addAction(), associatedGraphicsWidgets()
*/
QList<QWidget *> QAction::associatedWidgets() const
{
    Q_D(const QAction);
    return d->widgets;
}

#if QT_CONFIG(graphicsview)
/*!
  \since 4.5
  Returns a list of widgets this action has been added to.

  \sa QWidget::addAction(), associatedWidgets()
*/
QList<QGraphicsWidget *> QAction::associatedGraphicsWidgets() const
{
    Q_D(const QAction);
    return d->graphicsWidgets;
}
#endif

QAction::~QAction()
{
    Q_D(QAction);
    for (int i = d->widgets.size()-1; i >= 0; --i) {
        QWidget *w = d->widgets.at(i);
        w->removeAction(this);
    }
#if QT_CONFIG(graphicsview)
    for (int i = d->graphicsWidgets.size()-1; i >= 0; --i) {
        QGraphicsWidget *w = d->graphicsWidgets.at(i);
        w->removeAction(this);
    }
#endif
}

/*!
   Returns the action group for this action. If no action group manages
   this action then \nullptr will be returned.

   \sa QActionGroup, QAction::setActionGroup()
 */
QActionGroup *QAction::actionGroup() const
{
    return static_cast<QActionGroup *>(guiActionGroup());
}

#if QT_CONFIG(menu)
/*!
  Returns the menu contained by this action. Actions that contain
  menus can be used to create menu items with submenus, or inserted
  into toolbars to create buttons with popup menus.

  \sa QMenu::addAction()
*/
QMenu *QAction::menu() const
{
    Q_D(const QAction);
    return d->menu;
}

/*!
    Sets the menu contained by this action to the specified \a menu.
*/
void QAction::setMenu(QMenu *menu)
{
    Q_D(QAction);
    if (d->menu)
        d->menu->d_func()->setOverrideMenuAction(0); //we reset the default action of any previous menu
    d->menu = menu;
    if (menu)
        menu->d_func()->setOverrideMenuAction(this);
    d->sendDataChanged();
}
#endif // QT_CONFIG(menu)

/*!
  Updates the relevant status bar for the \a widget specified by sending a
  QStatusTipEvent to its parent widget. Returns \c true if an event was sent;
  otherwise returns \c false.

  If a null widget is specified, the event is sent to the action's parent.

  \sa statusTip
*/
bool
QAction::showStatusText(QWidget *widget)
{
    return d_func()->showStatusText(widget, statusTip());
}

QT_END_NAMESPACE
