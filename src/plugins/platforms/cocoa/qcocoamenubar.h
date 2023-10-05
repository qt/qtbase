// Copyright (C) 2018 The Qt Company Ltd.
// Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author James Turner <james.turner@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOAMENUBAR_H
#define QCOCOAMENUBAR_H

#include <QtCore/QList>
#include <qpa/qplatformmenu.h>
#include "qcocoamenu.h"

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QCocoaWindow;

class QCocoaMenuBar : public QPlatformMenuBar
                    , public QNativeInterface::Private::QCocoaMenuBar
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

    NSMenu *nsMenu() const override { return m_nativeMenu; }

    static void updateMenuBarImmediately();
    static void insertWindowMenu();

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
