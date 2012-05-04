/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#ifndef QCOCOAMENUBAR_H
#define QCOCOAMENUBAR_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenu.h"

@class NSMenu;
class QCocoaWindow;

class QCocoaMenuBar : public QPlatformMenuBar
{
public:
    QCocoaMenuBar();
    virtual ~QCocoaMenuBar();

    virtual void insertMenu(QPlatformMenu *menu, QPlatformMenu* before);
    virtual void removeMenu(QPlatformMenu *menu);
    virtual void syncMenu(QPlatformMenuItem *menuItem);
    virtual void handleReparent(QWindow *newParentWindow);
    virtual QPlatformMenu *menuForTag(quintptr tag) const;

    inline NSMenu *nsMenu() const
        { return m_nativeMenu; }

    static void updateMenuBarImmediately();

    QList<QCocoaMenuItem*> merged() const;
private:
    static QCocoaWindow *findWindowForMenubar();
    static QCocoaMenuBar *findGlobalMenubar();

    bool shouldDisable(QCocoaWindow *active) const;

    QList<QCocoaMenu*> m_menus;
    NSMenu *m_nativeMenu;
    QCocoaWindow *m_window;
};

#endif
