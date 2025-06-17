// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblemenu_p.h"

#if QT_CONFIG(menu)
#include <qmenu.h>
#endif
#if QT_CONFIG(menubar)
#include <qmenubar.h>
#endif
#include <qstyle.h>
#include <private/qwidget_p.h>

#if QT_CONFIG(accessibility)

#include <QtGui/private/qaccessiblehelper_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(menu)

QString qt_accHotKey(const QString &text);

QAccessibleInterface *getOrCreateMenu(QWidget *menu, QAction *action)
{
    QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(action);
    if (!iface) {
        iface = new QAccessibleMenuItem(menu, action);
        QAccessible::registerAccessibleInterface(iface);
    }
    return iface;
}

QAccessibleMenu::QAccessibleMenu(QWidget *w)
: QAccessibleWidgetV2(w)
{
    Q_ASSERT(menu());
}

QMenu *QAccessibleMenu::menu() const
{
    return qobject_cast<QMenu*>(object());
}

int QAccessibleMenu::childCount() const
{
    return menu()->actions().size();
}

QAccessibleInterface *QAccessibleMenu::childAt(int x, int y) const
{
    QAction *act = menu()->actionAt(menu()->mapFromGlobal(QPoint(x,y)));
    if (act && act->isSeparator())
        act = nullptr;
    return act ? getOrCreateMenu(menu(), act) : nullptr;
}

QString QAccessibleMenu::text(QAccessible::Text t) const
{
    QString tx = QAccessibleWidgetV2::text(t);
    if (!tx.isEmpty())
        return tx;

    if (t == QAccessible::Name)
        return menu()->windowTitle();
    return tx;
}

QAccessible::Role QAccessibleMenu::role() const
{
    return QAccessible::PopupMenu;
}

QAccessibleInterface *QAccessibleMenu::child(int index) const
{
    if (index < childCount())
        return getOrCreateMenu(menu(), menu()->actions().at(index));
    return nullptr;
}

QAccessibleInterface *QAccessibleMenu::parent() const
{
    if (QAction *menuAction = menu()->menuAction()) {
        QList<QObject *> parentCandidates;
        const QList<QObject *> associatedObjects = menuAction->associatedObjects();
        parentCandidates.reserve(associatedObjects.size() + 1);
        parentCandidates << menu()->parentWidget() << associatedObjects;
        for (QObject *object : std::as_const(parentCandidates)) {
            if (qobject_cast<QMenu*>(object)
#if QT_CONFIG(menubar)
                || qobject_cast<QMenuBar*>(object)
#endif
                ) {
                QWidget *widget = static_cast<QWidget*>(object);
                if (widget->actions().indexOf(menuAction) != -1)
                    return getOrCreateMenu(widget, menuAction);
            }
        }
    }
    return QAccessibleWidgetV2::parent();
}

int QAccessibleMenu::indexOfChild( const QAccessibleInterface *child) const
{
    QAccessible::Role r = child->role();
    if ((r == QAccessible::MenuItem || r == QAccessible::Separator) && menu()) {
        return menu()->actions().indexOf(qobject_cast<QAction*>(child->object()));
    }
    return -1;
}

#if QT_CONFIG(menubar)
QAccessibleMenuBar::QAccessibleMenuBar(QWidget *w)
    : QAccessibleWidgetV2(w, QAccessible::MenuBar)
{
    Q_ASSERT(menuBar());
}

QMenuBar *QAccessibleMenuBar::menuBar() const
{
    return qobject_cast<QMenuBar*>(object());
}

int QAccessibleMenuBar::childCount() const
{
    return menuBar()->actions().size();
}

QAccessibleInterface *QAccessibleMenuBar::child(int index) const
{
    if (index < childCount()) {
        return getOrCreateMenu(menuBar(), menuBar()->actions().at(index));
    }
    return nullptr;
}

int QAccessibleMenuBar::indexOfChild(const QAccessibleInterface *child) const
{
    QAccessible::Role r = child->role();
    if ((r == QAccessible::MenuItem || r == QAccessible::Separator) && menuBar()) {
        return menuBar()->actions().indexOf(qobject_cast<QAction*>(child->object()));
    }
    return -1;
}

#endif // QT_CONFIG(menubar)

QAccessibleMenuItem::QAccessibleMenuItem(QWidget *owner, QAction *action)
: m_action(action), m_owner(owner)
{
}

QAccessibleMenuItem::~QAccessibleMenuItem()
{}

QAccessibleInterface *QAccessibleMenuItem::childAt(int x, int y ) const
{
    for (int i = childCount() - 1; i >= 0; --i) {
        QAccessibleInterface *childInterface = child(i);
        if (childInterface->rect().contains(x,y)) {
            return childInterface;
        }
    }
    return nullptr;
}

int QAccessibleMenuItem::childCount() const
{
    return m_action->menu() ? 1 : 0;
}

int QAccessibleMenuItem::indexOfChild(const QAccessibleInterface * child) const
{
    if (child && child->role() == QAccessible::PopupMenu && child->object() == m_action->menu())
        return 0;
    return -1;
}

bool QAccessibleMenuItem::isValid() const
{
    return m_action && m_owner;
}

QAccessibleInterface *QAccessibleMenuItem::parent() const
{
    return QAccessible::queryAccessibleInterface(owner());
}

QAccessibleInterface *QAccessibleMenuItem::child(int index) const
{
    if (index == 0 && action()->menu())
        return QAccessible::queryAccessibleInterface(action()->menu());
    return nullptr;
}

void *QAccessibleMenuItem::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ActionInterface)
        return static_cast<QAccessibleActionInterface*>(this);
    return nullptr;
}

QObject *QAccessibleMenuItem::object() const
{
    return m_action;
}

/*! \reimp */
QWindow *QAccessibleMenuItem::window() const
{
    return m_owner.isNull()
        ? nullptr
        : qt_widget_private(m_owner.data())->windowHandle(QWidgetPrivate::WindowHandleMode::Closest);
}

QRect QAccessibleMenuItem::rect() const
{
    QRect rect;
    QWidget *own = owner();
#if QT_CONFIG(menubar)
    if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
        rect = menuBar->actionGeometry(m_action);
        QPoint globalPos = menuBar->mapToGlobal(QPoint(0,0));
        rect = rect.translated(globalPos);
    } else
#endif // QT_CONFIG(menubar)
    if (QMenu *menu = qobject_cast<QMenu*>(own)) {
        rect = menu->actionGeometry(m_action);
        QPoint globalPos = menu->mapToGlobal(QPoint(0,0));
        rect = rect.translated(globalPos);
    }
    return rect;
}

QAccessible::Role QAccessibleMenuItem::role() const
{
    return m_action->isSeparator() ? QAccessible::Separator : QAccessible::MenuItem;
}

void QAccessibleMenuItem::setText(QAccessible::Text /*t*/, const QString & /*text */)
{
}

QAccessible::State QAccessibleMenuItem::state() const
{
    QAccessible::State s;
    QWidget *own = owner();

    if (own && (own->testAttribute(Qt::WA_WState_Visible) == false || m_action->isVisible() == false)) {
        s.invisible = true;
    }

    if (QMenu *menu = qobject_cast<QMenu*>(own)) {
        if (menu->activeAction() == m_action)
            s.focused = true;
#if QT_CONFIG(menubar)
    } else if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
        if (menuBar->activeAction() == m_action)
            s.focused = true;
#endif
    }
    if (own && own->style()->styleHint(QStyle::SH_Menu_MouseTracking, nullptr, own))
        s.hotTracked = true;
    if (m_action->isSeparator() || !m_action->isEnabled())
        s.disabled = true;
    if (m_action->isChecked())
        s.checked = true;
    if (m_action->isCheckable())
        s.checkable = true;

    return s;
}

QString QAccessibleMenuItem::text(QAccessible::Text t) const
{
    QString str;
    switch (t) {
    case QAccessible::Name:
        str = qt_accStripAmp(m_action->text());
        break;
    case QAccessible::Accelerator: {
#ifndef QT_NO_SHORTCUT
        QKeySequence key = m_action->shortcut();
        if (!key.isEmpty()) {
            str = key.toString();
        } else
#endif
        {
            str = qt_accHotKey(m_action->text());
        }
        break;
    }
    default:
        break;
    }
    return str;
}

QStringList QAccessibleMenuItem::actionNames() const
{
    QStringList actions;
    if (!m_action || m_action->isSeparator())
        return actions;

    if (m_action->menu()) {
        actions << showMenuAction();
    } else {
        actions << pressAction();
    }
    return actions;
}

void QAccessibleMenuItem::doAction(const QString &actionName)
{
    if (!m_action->isEnabled())
        return;

    if (actionName == pressAction()) {
        m_action->trigger();
    } else if (actionName == showMenuAction()) {
#if QT_CONFIG(menubar)
        if (QMenuBar *bar = qobject_cast<QMenuBar*>(owner())) {
            if (m_action->menu() && m_action->menu()->isVisible()) {
                m_action->menu()->hide();
            } else {
                bar->setActiveAction(m_action);
            }
        } else
#endif
          if (QMenu *menu = qobject_cast<QMenu*>(owner())){
            if (m_action->menu() && m_action->menu()->isVisible()) {
                m_action->menu()->hide();
            } else {
                menu->setActiveAction(m_action);
            }
        }
    }
}

QStringList QAccessibleMenuItem::keyBindingsForAction(const QString &) const
{
    return QStringList();
}


QAction *QAccessibleMenuItem::action() const
{
    return m_action;
}

QWidget *QAccessibleMenuItem::owner() const
{
    return m_owner;
}

#endif // QT_CONFIG(menu)

QT_END_NAMESPACE

#endif // QT_CONFIG(accessibility)

