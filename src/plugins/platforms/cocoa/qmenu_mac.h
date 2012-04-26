/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qt_mac_p.h"
#include <QtCore/qpointer.h>
#include <QtWidgets/qmenu.h>
#include <QtWidgets/qmenubar.h>
#include <qpa/qplatformmenu.h>

@class NSMenuItem;

QT_BEGIN_NAMESPACE

class QCocoaMenuAction : public QPlatformMenuAction
{
public:
    QCocoaMenuAction();
    ~QCocoaMenuAction();

    NSMenuItem *menuItem;
    uchar ignore_accel : 1;
    uchar merged : 1;
    OSMenuRef menu;
    QPointer<QMenu> qtMenu;
};

struct QMenuMergeItem
{
    inline QMenuMergeItem(NSMenuItem *c, QCocoaMenuAction *a) : menuItem(c), action(a) { }
    NSMenuItem *menuItem;
    QCocoaMenuAction *action;
};
typedef QList<QMenuMergeItem> QMenuMergeList;

class QCocoaMenu : public QPlatformMenu
{
public:
    QCocoaMenu(QMenu *qtMenu);
    ~QCocoaMenu();

    OSMenuRef macMenu(OSMenuRef merge = 0);
    void syncSeparatorsCollapsible(bool collapse);
    void setMenuEnabled(bool enable);

    void addAction(QAction *action, QAction *before);
    void syncAction(QAction *action);
    void removeAction(QAction *action);

    void addAction(QCocoaMenuAction *action, QCocoaMenuAction *before);
    void syncAction(QCocoaMenuAction *action);
    void removeAction(QCocoaMenuAction *action);
    bool merged(const QAction *action) const;
    QCocoaMenuAction *findAction(QAction *action) const;

    OSMenuRef menu;
    static QHash<OSMenuRef, OSMenuRef> mergeMenuHash;
    static QHash<OSMenuRef, QMenuMergeList*> mergeMenuItemsHash;
    QList<QCocoaMenuAction*> actionItems;
    QMenu *qtMenu;
};

class QCocoaMenuBar : public QPlatformMenuBar
{
public:
    QCocoaMenuBar(QMenuBar *qtMenuBar);
    ~QCocoaMenuBar();

    void handleReparent(QWidget *newParent);

    void addAction(QAction *action, QAction *before);
    void syncAction(QAction *action);
    void removeAction(QAction *action);

    void addAction(QCocoaMenuAction *action, QCocoaMenuAction *before);
    void syncAction(QCocoaMenuAction *action);
    void removeAction(QCocoaMenuAction *action);

    bool macWidgetHasNativeMenubar(QWidget *widget);
    void macCreateMenuBar(QWidget *parent);
    void macDestroyMenuBar();
    OSMenuRef macMenu();
    static bool macUpdateMenuBarImmediatly();
    static void macUpdateMenuBar();
    QCocoaMenuAction *findAction(QAction *action) const;

    OSMenuRef menu;
    OSMenuRef apple_menu;
    QList<QCocoaMenuAction*> actionItems;
    QMenuBar *qtMenuBar;
};

QT_END_NAMESPACE
