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
#ifndef QMENUBARIMPL_P_H
#define QMENUBARIMPL_P_H

#ifndef QT_NO_MENUBAR

#include "qabstractmenubarimpl_p.h"

QT_BEGIN_NAMESPACE

class QMenuBar;

class QMenuBarImpl : public QAbstractMenuBarImpl
{
public:
    ~QMenuBarImpl();

    virtual void init(QMenuBar *);

    virtual bool allowSetVisible() const;

    virtual void actionEvent(QActionEvent *e);

    virtual void handleReparent(QWidget *oldParent, QWidget *newParent, QWidget *oldWindow, QWidget *newWindow);

    virtual bool allowCornerWidgets() const;

    virtual void popupAction(QAction*);

    virtual void setNativeMenuBar(bool);
    virtual bool isNativeMenuBar() const;

    virtual bool shortcutsHandledByNativeMenuBar() const;
    virtual bool menuBarEventFilter(QObject *, QEvent *event);

private:
    QMenuBar *menuBar;
    int nativeMenuBar : 3;  // Only has values -1, 0, and 1

#ifdef Q_WS_MAC
    //mac menubar binding
    struct QMacMenuBarPrivate {
        QList<QMacMenuAction*> actionItems;
        OSMenuRef menu, apple_menu;
        QMacMenuBarPrivate();
        ~QMacMenuBarPrivate();

        void addAction(QAction *, QAction* =0);
        void addAction(QMacMenuAction *, QMacMenuAction* =0);
        void syncAction(QMacMenuAction *);
        inline void syncAction(QAction *a) { syncAction(findAction(a)); }
        void removeAction(QMacMenuAction *);
        inline void removeAction(QAction *a) { removeAction(findAction(a)); }
        inline QMacMenuAction *findAction(QAction *a) {
            for(int i = 0; i < actionItems.size(); i++) {
                QMacMenuAction *act = actionItems[i];
                if(a == act->action)
                    return act;
            }
            return 0;
        }
    } adapter;
    static bool macUpdateMenuBarImmediatly();
    bool macWidgetHasNativeMenubar(QWidget *widget);
    void macCreateMenuBar(QWidget *);
    void macDestroyMenuBar();
    OSMenuRef macMenu();
#endif
#ifdef Q_WS_WINCE
    void wceCreateMenuBar(QWidget *);
    void wceDestroyMenuBar();
    struct QWceMenuBarPrivate {
        QList<QWceMenuAction*> actionItems;
        QList<QWceMenuAction*> actionItemsLeftButton;
        QList<QList<QWceMenuAction*>> actionItemsClassic;
        HMENU menuHandle;
        HMENU leftButtonMenuHandle;
        HWND menubarHandle;
        HWND parentWindowHandle;
        bool leftButtonIsMenu;
        QPointer<QAction> leftButtonAction;
        QMenuBarPrivate *d;
        int leftButtonCommand;

        QWceMenuBarPrivate(QMenuBarPrivate *menubar);
        ~QWceMenuBarPrivate();
        void addAction(QAction *, QAction* =0);
        void addAction(QWceMenuAction *, QWceMenuAction* =0);
        void syncAction(QWceMenuAction *);
        inline void syncAction(QAction *a) { syncAction(findAction(a)); }
        void removeAction(QWceMenuAction *);
        void rebuild();
        inline void removeAction(QAction *a) { removeAction(findAction(a)); }
        inline QWceMenuAction *findAction(QAction *a) {
            for(int i = 0; i < actionItems.size(); i++) {
                QWceMenuAction *act = actionItems[i];
                if(a == act->action)
                    return act;
            }
            return 0;
        }
    } adapter;
    bool wceClassicMenu;
    void wceCommands(uint command);
    void wceRefresh();
    bool wceEmitSignals(QList<QWceMenuAction*> actions, uint command);
#endif
#ifdef Q_WS_S60
    void symbianCreateMenuBar(QWidget *);
    void symbianDestroyMenuBar();
    void reparentMenuBar(QWidget *oldParent, QWidget *newParent);
    struct QSymbianMenuBarPrivate {
        QList<QSymbianMenuAction*> actionItems;
        QMenuBarPrivate *d;
        QSymbianMenuBarPrivate(QMenuBarPrivate *menubar);
        ~QSymbianMenuBarPrivate();
        void addAction(QAction *, QAction* =0);
        void addAction(QSymbianMenuAction *, QSymbianMenuAction* =0);
        void syncAction(QSymbianMenuAction *);
        inline void syncAction(QAction *a) { syncAction(findAction(a)); }
        void removeAction(QSymbianMenuAction *);
        void rebuild();
        inline void removeAction(QAction *a) { removeAction(findAction(a)); }
        inline QSymbianMenuAction *findAction(QAction *a) {
            for(int i = 0; i < actionItems.size(); i++) {
                QSymbianMenuAction *act = actionItems[i];
                if(a == act->action)
                    return act;
            }
            return 0;
        }
        void insertNativeMenuItems(const QList<QAction*> &actions);

    } adapter;
    static int symbianCommands(int command);
#endif
};

QMenuBarImplFactoryInterface *qt_guiMenuBarImplFactory();

QT_END_NAMESPACE

#endif // QT_NO_MENUBAR

#endif /* QMENUBARIMPL_P_H */
