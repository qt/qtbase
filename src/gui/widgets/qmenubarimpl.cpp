/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qmenubarimpl_p.h"

#ifndef QT_NO_MENUBAR

#include "qapplication.h"
#include "qdebug.h"
#include "qevent.h"
#include "qmenu.h"
#include "qmenubar.h"

QT_BEGIN_NAMESPACE

QMenuBarImpl::~QMenuBarImpl()
{
#ifdef Q_WS_MAC
    macDestroyMenuBar();
#endif
#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile())
        wceDestroyMenuBar();
#endif
#ifdef Q_WS_S60
    symbianDestroyMenuBar();
#endif
}

void QMenuBarImpl::init(QMenuBar *_menuBar)
{
    nativeMenuBar = -1;
    menuBar = _menuBar;
#if defined(Q_WS_MAC) || defined(Q_OS_WINCE) || defined(Q_WS_S60)
    adapter = 0;
#endif
#ifdef Q_WS_MAC
    macCreateMenuBar(menuBar->parentWidget());
    if(adapter)
        menuBar->hide();
#endif
#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile()) {
        wceCreateMenuBar(menuBar->parentWidget());
        if(adapter)
            menuBar->hide();
    }
    else {
        QApplication::setAttribute(Qt::AA_DontUseNativeMenuBar, true);
    }
#endif
}

bool QMenuBarImpl::allowSetVisible() const
{
#if defined(Q_WS_MAC) || defined(Q_OS_WINCE) || defined(Q_WS_S60)
    // FIXME: Port this to a setVisible() method
    /*
    if (isNativeMenuBar()) {
        if (!visible)
            QWidget::setVisible(false);
        return;
    }
    */
    return !isNativeMenuBar();
#endif
    return true;
}

void QMenuBarImpl::actionEvent(QActionEvent *e)
{
#if defined(Q_WS_MAC) || defined(Q_OS_WINCE) || defined(Q_WS_S60)
    if (adapter) {
        if(e->type() == QEvent::ActionAdded)
            adapter->addAction(e->action(), e->before());
        else if(e->type() == QEvent::ActionRemoved)
            adapter->removeAction(e->action());
        else if(e->type() == QEvent::ActionChanged)
            adapter->syncAction(e->action());
    }
#else
    Q_UNUSED(e);
#endif
}

void QMenuBarImpl::handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow)
{
#ifdef Q_WS_X11
    Q_UNUSED(oldParent)
    Q_UNUSED(newParent)
    Q_UNUSED(oldWindow)
    Q_UNUSED(newWindow)
#endif

#ifdef Q_WS_MAC
    if (isNativeMenuBar() && !macWidgetHasNativeMenubar(newParent)) {
        // If the new parent got a native menubar from before, keep that
        // menubar rather than replace it with this one (because a parents
        // menubar has precedence over children menubars).
        macDestroyMenuBar();
        macCreateMenuBar(newParent);
    }
#endif
#ifdef Q_WS_WINCE
    if (qt_wince_is_mobile() && nativeMenuBarAdapter())
        adapter->rebuild();
#endif
#ifdef Q_WS_S60

    // Construct d->impl->nativeMenuBarAdapter() when this code path is entered first time
    // and when newParent != NULL
    if (!adapter)
        symbianCreateMenuBar(newParent);

    // Reparent and rebuild menubar when parent is changed
    if (adapter) {
        if (oldParent != newParent)
            reparentMenuBar(oldParent, newParent);
        menuBar->hide();
        adapter->rebuild();
    }

#ifdef QT_SOFTKEYS_ENABLED
    // Constuct menuBarAction when this code path is entered first time
    if (!menuBarAction) {
        if (newParent) {
            menuBarAction = QSoftKeyManager::createAction(QSoftKeyManager::MenuSoftKey, newParent);
            newParent->addAction(menuBarAction);
        }
    } else {
        // If reparenting i.e. we already have menuBarAction, remove it from old parent
        // and add for a new parent
        if (oldParent)
            oldParent->removeAction(menuBarAction);
        if (newParent)
            newParent->addAction(menuBarAction);
    }
#endif // QT_SOFTKEYS_ENABLED
#endif // Q_WS_S60
}

bool QMenuBarImpl::allowCornerWidgets() const
{
    return true;
}

void QMenuBarImpl::popupAction(QAction *)
{
}

void QMenuBarImpl::setNativeMenuBar(bool value)
{
    if (nativeMenuBar == -1 || (value != bool(nativeMenuBar))) {
        nativeMenuBar = value;
#ifdef Q_WS_MAC
        if (!nativeMenuBar) {
            extern void qt_mac_clear_menubar();
            qt_mac_clear_menubar();
            macDestroyMenuBar();
            const QList<QAction *> &menubarActions = actions();
            for (int i = 0; i < menubarActions.size(); ++i) {
                const QAction *action = menubarActions.at(i);
                if (QMenu *menu = action->menu()) {
                    delete menu->d_func()->mac_menu;
                    menu->d_func()->mac_menu = 0;
                }
            }
        } else {
            macCreateMenuBar(parentWidget());
        }
        macUpdateMenuBar();
        updateGeometry();
        if (!nativeMenuBar && parentWidget())
            setVisible(true);
#endif
    }
}

bool QMenuBarImpl::isNativeMenuBar() const
{
#if defined(Q_WS_MAC) || defined(Q_OS_WINCE) || defined(Q_WS_S60)
    if (nativeMenuBar == -1) {
        return !QApplication::instance()->testAttribute(Qt::AA_DontUseNativeMenuBar);
    }
    return nativeMenuBar;
#else
    return false;
#endif
}

bool QMenuBarImpl::shortcutsHandledByNativeMenuBar() const
{
#ifdef Q_WS_MAC
    return true;
#else
    return false;
#endif
}

bool QMenuBarImpl::menuBarEventFilter(QObject *, QEvent *)
{
    return false;
}

QT_END_NAMESPACE

#endif // QT_NO_MENUBAR
