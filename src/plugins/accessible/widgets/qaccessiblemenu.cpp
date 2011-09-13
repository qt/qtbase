/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblemenu.h"

#include <qmenu.h>
#include <qmenubar.h>
#include <QtWidgets/QAction>
#include <qstyle.h>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

#ifndef QT_NO_MENU

QString Q_GUI_EXPORT qt_accStripAmp(const QString &text);
QString Q_GUI_EXPORT qt_accHotKey(const QString &text);

QAccessibleMenu::QAccessibleMenu(QWidget *w)
: QAccessibleWidgetEx(w)
{
    Q_ASSERT(menu());
}

QMenu *QAccessibleMenu::menu() const
{
    return qobject_cast<QMenu*>(object());
}

int QAccessibleMenu::childCount() const
{
    return menu()->actions().count();
}

QRect QAccessibleMenu::rect(int child) const
{
    if (!child || child > childCount())
        return QAccessibleWidgetEx::rect(child);

    QRect r = menu()->actionGeometry(menu()->actions()[child - 1]);
    QPoint tlp = menu()->mapToGlobal(QPoint(0,0));

    return QRect(tlp.x() + r.x(), tlp.y() + r.y(), r.width(), r.height());
}

int QAccessibleMenu::childAt(int x, int y) const
{
    QAction *act = menu()->actionAt(menu()->mapFromGlobal(QPoint(x,y)));
    if(act && act->isSeparator())
        act = 0;
    return menu()->actions().indexOf(act) + 1;
}

QString QAccessibleMenu::text(Text t, int child) const
{
    QString tx = QAccessibleWidgetEx::text(t, child);
    if (tx.size())
        return tx;

    switch (t) {
    case Name:
        if (!child)
            return menu()->windowTitle();
        return qt_accStripAmp(menu()->actions().at(child-1)->text());
    case Help:
        return child ? menu()->actions().at(child-1)->whatsThis() : tx;
#ifndef QT_NO_SHORTCUT
    case Accelerator:
        return child ? static_cast<QString>(menu()->actions().at(child-1)->shortcut()) : tx;
#endif
    default:
        break;
    }
    return tx;
}

QAccessible::Role QAccessibleMenu::role(int child) const
{
    if (!child)
        return PopupMenu;

    QAction *action = menu()->actions()[child-1];
    if (action && action->isSeparator())
        return Separator;
    return MenuItem;
}

QAccessible::State QAccessibleMenu::state(int child) const
{
    State s = QAccessibleWidgetEx::state(child);
    if (!child)
        return s;

    QAction *action = menu()->actions()[child-1];
    if (!action)
        return s;

    if (menu()->style()->styleHint(QStyle::SH_Menu_MouseTracking))
        s |= HotTracked;
    if (action->isSeparator() || !action->isEnabled())
        s |= Unavailable;
    if (action->isChecked())
        s |= Checked;
    if (menu()->activeAction() == action)
        s |= Focused;

    return s;
}

QString QAccessibleMenu::actionText(int action, QAccessible::Text text, int child) const
{
    if (action == QAccessible::DefaultAction && child && text == QAccessible::Name) {
        QAction *a = menu()->actions().value(child-1, 0);
        if (!a || a->isSeparator())
            return QString();
        if (a->menu()) {
            if (a->menu()->isVisible())
                return QMenu::tr("Close");
            return QMenu::tr("Open");
        }
        return QMenu::tr("Execute");
     }

    return QAccessibleWidgetEx::actionText(action, text, child);
}

bool QAccessibleMenu::doAction(int act, int child, const QVariantList &)
{
    if (!child || act != QAccessible::DefaultAction)
        return false;

    QAction *action = menu()->actions().value(child-1, 0);
    if (!action || !action->isEnabled())
        return false;

    if (action->menu() && action->menu()->isVisible())
        action->menu()->hide();
    else
        menu()->setActiveAction(action);
    return true;
}

int QAccessibleMenu::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    int ret = -1;
    if (entry < 0) {
        *target = 0;
        return ret;
    }

    if (relation == Self || entry == 0) {
        *target = new QAccessibleMenu(menu());
        return 0;
    }

    switch (relation) {
    case Child:
        if (entry <= childCount()) {
            *target = new QAccessibleMenuItem(menu(), menu()->actions().at( entry - 1 ));
            ret = 0;
        }
        break;
    case Ancestor: {
        QAccessibleInterface *iface;
        QWidget *parent = menu()->parentWidget();
        if (qobject_cast<QMenu*>(parent) || qobject_cast<QMenuBar*>(parent)) {
            iface = new QAccessibleMenuItem(parent, menu()->menuAction());
            if (entry == 1) {
                *target = iface;
                ret = 0;
            } else {
                ret = iface->navigate(Ancestor, entry - 1, target);
                delete iface;
            }
        } else {
            return QAccessibleWidgetEx::navigate(relation, entry, target); 
        }
        break;}
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }


    if (ret == -1)
        *target = 0;

    return ret;

}

int QAccessibleMenu::indexOfChild( const QAccessibleInterface *child ) const
{
    int index = -1;
    Role r = child->role(0);
    if ((r == MenuItem || r == Separator) && menu()) {
        index = menu()->actions().indexOf(qobject_cast<QAction*>(child->object()));
        if (index != -1)
            ++index;
    }
    return index;
}

#ifndef QT_NO_MENUBAR
QAccessibleMenuBar::QAccessibleMenuBar(QWidget *w)
: QAccessibleWidgetEx(w)
{
    Q_ASSERT(menuBar());
}

QMenuBar *QAccessibleMenuBar::menuBar() const
{
    return qobject_cast<QMenuBar*>(object());
}

int QAccessibleMenuBar::childCount() const
{
    return menuBar()->actions().count();
}

QRect QAccessibleMenuBar::rect(int child) const
{
    if (!child)
        return QAccessibleWidgetEx::rect(child);

    QRect r = menuBar()->actionGeometry(menuBar()->actions()[child - 1]);
    QPoint tlp = menuBar()->mapToGlobal(QPoint(0,0));
    return QRect(tlp.x() + r.x(), tlp.y() + r.y(), r.width(), r.height());
}

int QAccessibleMenuBar::childAt(int x, int y) const
{
    for (int i = childCount(); i >= 0; --i) {
        if (rect(i).contains(x,y))
            return i;
    }
    return -1;
}

int QAccessibleMenuBar::navigate(RelationFlag relation, int entry, QAccessibleInterface **target) const
{
    int ret = -1;
    if (entry < 0) {
        *target = 0;
        return ret;
    }

    if (relation == Self || entry == 0) {
        *target = new QAccessibleMenuBar(menuBar());
        return 0;
    }

    switch (relation) {
    case Child:
        if (entry <= childCount()) {
            *target = new QAccessibleMenuItem(menuBar(), menuBar()->actions().at( entry - 1 ));
            ret = 0;
        }
        break;
    default:
        return QAccessibleWidgetEx::navigate(relation, entry, target);
    }


    if (ret == -1)
        *target = 0;

    return ret;
}

int QAccessibleMenuBar::indexOfChild( const QAccessibleInterface *child ) const
{
    int index = -1;
    Role r = child->role(0);
    if ((r == MenuItem || r == Separator) && menuBar()) {
        index = menuBar()->actions().indexOf(qobject_cast<QAction*>(child->object()));
        if (index != -1)
            ++index;
    }
    return index;
}

QString QAccessibleMenuBar::text(Text t, int child) const
{
    QString str;

    if (child) {
        if (QAction *action = menuBar()->actions().value(child - 1, 0)) {
            switch (t) {
            case Name:
                return qt_accStripAmp(action->text());
            case Accelerator:
                str = qt_accHotKey(action->text());
                break;
            default:
                break;
            }
        }
    }
    if (str.isEmpty())
        str = QAccessibleWidgetEx::text(t, child);
    return str;
}

QAccessible::Role QAccessibleMenuBar::role(int child) const
{
    if (!child)
        return MenuBar;

    QAction *action = menuBar()->actions()[child-1];
    if (action && action->isSeparator())
        return Separator;
    return MenuItem;
}

QAccessible::State QAccessibleMenuBar::state(int child) const
{
    State s = QAccessibleWidgetEx::state(child);
    if (!child)
        return s;

    QAction *action = menuBar()->actions().value(child-1, 0);
    if (!action)
        return s;

    if (menuBar()->style()->styleHint(QStyle::SH_Menu_MouseTracking))
        s |= HotTracked;
    if (action->isSeparator() || !action->isEnabled())
        s |= Unavailable;
    if (menuBar()->activeAction() == action)
        s |= Focused;

    return s;
}

QString QAccessibleMenuBar::actionText(int action, QAccessible::Text text, int child) const
{
    if (action == QAccessible::DefaultAction && child && text == QAccessible::Name) {
        QAction *a = menuBar()->actions().value(child-1, 0);
        if (!a || a->isSeparator())
            return QString();
        if (a->menu()) {
            if (a->menu()->isVisible())
                return QMenu::tr("Close");
            return QMenu::tr("Open");
        }
        return QMenu::tr("Execute");
    }

    return QAccessibleWidgetEx::actionText(action, text, child);
}

bool QAccessibleMenuBar::doAction(int act, int child, const QVariantList &)
{
    if (act != !child)
        return false;

    QAction *action = menuBar()->actions().value(child-1, 0);
    if (!action || !action->isEnabled())
        return false;
    if (action->menu() && action->menu()->isVisible())
        action->menu()->hide();
    else
        menuBar()->setActiveAction(action);
    return true;
}

#endif // QT_NO_MENUBAR

QAccessibleMenuItem::QAccessibleMenuItem(QWidget *owner, QAction *action) : m_action(action), m_owner(owner)
{
}


QAccessibleMenuItem::~QAccessibleMenuItem()
{}

int QAccessibleMenuItem::childAt(int x, int y ) const
{
    for (int i = childCount(); i >= 0; --i) {
        if (rect(i).contains(x,y))
            return i;
    }
    return -1;
}

int QAccessibleMenuItem::childCount() const
{
    return m_action->menu() ? 1 : 0;
}

QString QAccessibleMenuItem::actionText(int action, Text text, int child ) const
{
    if (text == Name && child == 0) {
        switch (action) {
        case Press:
        case DefaultAction:
            return QMenu::tr("Execute");
            break;
        default:
            break;
        }
    }
    return QString();
}

bool QAccessibleMenuItem::doAction(int action, int child, const QVariantList & /*params = QVariantList()*/ )
{
    if ((action == Press || action == DefaultAction) && child == 0) {
        m_action->trigger();
        return true;
    }
    return false;
}

int QAccessibleMenuItem::indexOfChild( const QAccessibleInterface * child ) const
{
    if (child->role(0) == PopupMenu && child->object() == m_action->menu())
        return 1;

    return -1;
}

bool QAccessibleMenuItem::isValid() const
{
    return m_action ? true : false;
}

int QAccessibleMenuItem::navigate(RelationFlag relation, int entry, QAccessibleInterface ** target ) const
{
    int ret = -1;
    if (entry < 0) {
        *target = 0;
        return ret;
    }

    if (relation == Self || entry == 0) {
        *target = new QAccessibleMenuItem(owner(), action());
        return 0;
    }

    switch (relation) {
    case Child:
        if (entry <= childCount()) {
            *target = new QAccessibleMenu(action()->menu());
            ret = 0;
        }
        break;

    case Ancestor:{
        QWidget *parent = owner();
        QAccessibleInterface *ancestor = parent ? QAccessible::queryAccessibleInterface(parent) : 0;
        if (ancestor) {
            if (entry == 1) {
                *target = ancestor;
                ret = 0;
            } else {
                ret = ancestor->navigate(Ancestor, entry - 1, target);
                delete ancestor;
            }
        }
        break;}
    case Up:
    case Down:{
        QAccessibleInterface *parent = 0;
        int ent = navigate(Ancestor, 1, &parent);
        if (ent == 0) {
            int index = parent->indexOfChild(this);
            if (index != -1) {
                index += (relation == Down ? +1 : -1);
                ret = parent->navigate(Child, index, target);
            }
        }
        delete parent;
        break;}
    case Sibling: {
        QAccessibleInterface *parent = 0;
        int ent = navigate(Ancestor, 1, &parent);
        if (ent == 0) {
            ret = parent->navigate(Child, entry, target);
        }
        delete parent;
        break;}
    default:
        break;

    }
    if (ret == -1)
        *target = 0;
    return ret;
}

QObject *QAccessibleMenuItem::object() const
{
    return m_action;
}

QRect QAccessibleMenuItem::rect (int child ) const
{
    QRect rect;
    if (child == 0) {
        QWidget *own = owner();
#ifndef QT_NO_MENUBAR
        if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
            rect = menuBar->actionGeometry(m_action);
            QPoint globalPos = menuBar->mapToGlobal(QPoint(0,0));
            rect = rect.translated(globalPos);
        } else
#endif // QT_NO_MENUBAR
        if (QMenu *menu = qobject_cast<QMenu*>(own)) {
            rect = menu->actionGeometry(m_action);
            QPoint globalPos = menu->mapToGlobal(QPoint(0,0));
            rect = rect.translated(globalPos);
        }
    } else if (child == 1) {
        QMenu *menu = m_action->menu();
        if (menu) {
            rect = menu->rect();
            QPoint globalPos = menu->mapToGlobal(QPoint(0,0));
            rect = rect.translated(globalPos);
        }
    }
    return rect;
}

QAccessible::Relation QAccessibleMenuItem::relationTo ( int child, const QAccessibleInterface * other, int otherChild ) const
{
    if (other->object() == owner()) {
        return Child;
    }
    Q_UNUSED(child)
    Q_UNUSED(other)
    Q_UNUSED(otherChild)
    // ###
    return Unrelated;
}

QAccessible::Role QAccessibleMenuItem::role(int /*child*/ ) const
{
    return m_action->isSeparator() ? Separator :MenuItem;
}

void QAccessibleMenuItem::setText ( Text /*t*/, int /*child*/, const QString & /*text */)
{

}

QAccessible::State QAccessibleMenuItem::state(int child ) const
{
    QAccessible::State s = Unavailable;

    if (child == 0) {
        s = Normal;
        QWidget *own = owner();

        if (own->testAttribute(Qt::WA_WState_Visible) == false || m_action->isVisible() == false) {
            s |= Invisible;
        }

        if (QMenu *menu = qobject_cast<QMenu*>(own)) {
            if (menu->activeAction() == m_action)
                s |= Focused;
#ifndef QT_NO_MENUBAR
        } else if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(own)) {
            if (menuBar->activeAction() == m_action)
                s |= Focused;
#endif
        }
        if (own->style()->styleHint(QStyle::SH_Menu_MouseTracking))
            s |= HotTracked;
        if (m_action->isSeparator() || !m_action->isEnabled())
            s |= Unavailable;
        if (m_action->isChecked())
            s |= Checked;
    } else if (child == 1) {
        QMenu *menu = m_action->menu();
        if (menu) {
            QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(menu);
            s = iface->state(0);
            delete iface;
        }
    }
    return s;
}

QString QAccessibleMenuItem::text ( Text t, int child ) const
{
    QString str;
    switch (t) {
    case Name:
        if (child == 0) {
            str = m_action->text();
        } else if (child == 1) {
            QMenu *m = m_action->menu();
            if (m)
                str = m->title();
        }
        str = qt_accStripAmp(str);
        break;
    case Accelerator:
        if (child == 0) {
#ifndef QT_NO_SHORTCUT
            QKeySequence key = m_action->shortcut();
            if (!key.isEmpty()) {
                str = key.toString();
            } else
#endif
            {
                str = qt_accHotKey(m_action->text());
            }
        }
        break;
    default:
        break;
    }
    return str;
}

int QAccessibleMenuItem::userActionCount ( int /*child*/ ) const
{
    return 0;
}


QAction *QAccessibleMenuItem::action() const
{
    return m_action;
}

QWidget *QAccessibleMenuItem::owner() const
{
    return m_owner;
}

#endif // QT_NO_MENU

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY

