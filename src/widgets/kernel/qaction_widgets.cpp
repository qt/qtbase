/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <private/qapplication_p.h>
#include <private/qwidget_p.h>
#include "qaction_widgets_p.h"
#if QT_CONFIG(menu)
#include <private/qmenu_p.h>
#endif
#if QT_CONFIG(graphicsview)
#include "qgraphicswidget.h"
#endif


QT_BEGIN_NAMESPACE

QActionPrivate *QApplicationPrivate::createActionPrivate() const
{
    return new QtWidgetsActionPrivate;
}

QtWidgetsActionPrivate::~QtWidgetsActionPrivate() = default;

// we can't do this in the destructor, as it would only be called by ~QObject
void QtWidgetsActionPrivate::destroy()
{
    Q_Q(QAction);
    const auto objects = associatedObjects;
    for (int i = objects.size()-1; i >= 0; --i) {
        QObject *object = objects.at(i);
        if (QWidget *widget = qobject_cast<QWidget*>(object))
            widget->removeAction(q);
#if QT_CONFIG(graphicsview)
        else if (QGraphicsWidget *graphicsWidget = qobject_cast<QGraphicsWidget*>(object))
            graphicsWidget->removeAction(q);
#endif
    }
}

#if QT_CONFIG(shortcut)
QShortcutMap::ContextMatcher QtWidgetsActionPrivate::contextMatcher() const
{
    return qWidgetShortcutContextMatcher;
}
#endif

#if QT_CONFIG(menu)
QObject *QtWidgetsActionPrivate::menu() const
{
    return m_menu;
}

void QtWidgetsActionPrivate::setMenu(QObject *menu)
{
    Q_Q(QAction);
    QMenu *theMenu = qobject_cast<QMenu*>(menu);
    Q_ASSERT_X(!menu || theMenu, "QAction::setMenu",
               "QAction::setMenu expects a QMenu* in widget applications");
    if (m_menu)
        m_menu->d_func()->setOverrideMenuAction(nullptr); //we reset the default action of any previous menu
    m_menu = theMenu;
    if (m_menu)
        m_menu->d_func()->setOverrideMenuAction(q);
    sendDataChanged();
}
#endif // QT_CONFIG(menu)

QT_END_NAMESPACE
