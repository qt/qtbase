/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ACCESSIBILITYSCENEMANAGER_H
#define ACCESSIBILITYSCENEMANAGER_H

#include <QtGui>

#include "optionswidget.h"

QString translateRole(QAccessible::Role role);
class AccessibilitySceneManager : public QObject
{
Q_OBJECT
public:
    AccessibilitySceneManager();
    void setRootWindow(QWindow * window) { m_window = window; }
    void setView(QGraphicsView *view) { m_view = view; }
    void setScene(QGraphicsScene *scene) { m_scene = scene; }
    void setTreeView(QGraphicsView *treeView) { m_treeView = treeView; }
    void setTreeScene(QGraphicsScene *treeScene) { m_treeScene = treeScene; }

    void setOptionsWidget(OptionsWidget *optionsWidget) { m_optionsWidget = optionsWidget; }
public slots:
    void populateAccessibilityScene();
    void updateAccessibilitySceneItemFlags();
    void populateAccessibilityTreeScene();
    void handleUpdate(QAccessibleEvent *event);
    void setSelected(QObject *object);

    void changeScale(int scale);
private:
    void updateItems(QObject *root);
    void updateItem(QObject *object);
    void updateItem(QGraphicsRectItem *item, QAccessibleInterface *interface);
    void updateItemFlags(QGraphicsRectItem *item, QAccessibleInterface *interface);

    void populateAccessibilityScene(QAccessibleInterface * interface, QGraphicsScene *scene);
    QGraphicsRectItem * processInterface(QAccessibleInterface * interface, QGraphicsScene *scene);

    struct TreeItem;
    TreeItem computeLevels(QAccessibleInterface * interface, int level);
    void populateAccessibilityTreeScene(QAccessibleInterface * interface);
    void addGraphicsItems(TreeItem item, int row, int xPos);

    bool isHidden(QAccessibleInterface *interface);

    QWindow *m_window;
    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QGraphicsView *m_treeView;
    QGraphicsScene *m_treeScene;
    QGraphicsItem *m_rootItem;
    OptionsWidget *m_optionsWidget;
    QObject *m_selectedObject;

    QHash<QObject *, QGraphicsRectItem*> m_graphicsItems;
    QSet<QObject *> m_animatedObjects;

    struct TreeItem {
        QList<TreeItem> children;
        int width;
        QString name;
        QString role;
        QString description;
        QRect rect;
        QAccessible::State state;
        QObject *object;
        TreeItem() : width(0) {}
    };

    TreeItem m_rootTreeItem;
    int m_treeItemWidth;
    int m_treeItemHorizontalPadding;
    int m_treeItemHeight;
    int m_treeItemVerticalPadding;
};

#endif // ACCESSIBILITYSCENEMANAGER_H
