/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins module of the Qt Toolkit.
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

#ifndef QCOCOAMENUBAR_H
#define QCOCOAMENUBAR_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenu.h"

QT_BEGIN_NAMESPACE

class QCocoaWindow;

class QCocoaMenuBar : public QPlatformMenuBar
{
    Q_OBJECT
public:
    QCocoaMenuBar();
    ~QCocoaMenuBar();

    void insertMenu(QPlatformMenu *menu, QPlatformMenu* before) override;
    void removeMenu(QPlatformMenu *menu) override;
    void syncMenu(QPlatformMenu *menuItem) override;
    void handleReparent(QWindow *newParentWindow) override;
    QWindow *parentWindow() const override;
    QPlatformMenu *menuForTag(quintptr tag) const override;

    inline NSMenu *nsMenu() const
        { return m_nativeMenu; }

    static void updateMenuBarImmediately();

    QList<QCocoaMenuItem*> merged() const;
    NSMenuItem *itemForRole(QPlatformMenuItem::MenuRole role);
    QCocoaWindow *cocoaWindow() const;

    void syncMenu_helper(QPlatformMenu *menu, bool menubarUpdate);

private:
    static QCocoaWindow *findWindowForMenubar();
    static QCocoaMenuBar *findGlobalMenubar();

    bool needsImmediateUpdate();
    bool shouldDisable(QCocoaWindow *active) const;

    NSMenuItem *nativeItemForMenu(QCocoaMenu *menu) const;

    QList<QPointer<QCocoaMenu> > m_menus;
    NSMenu *m_nativeMenu;
    QPointer<QCocoaWindow> m_window;
};

QT_END_NAMESPACE

#endif
