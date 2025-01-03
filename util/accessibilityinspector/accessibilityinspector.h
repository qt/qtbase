// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef ACCESSIBILITYINSPECTOR_H
#define ACCESSIBILITYINSPECTOR_H

#include <QObject>
#include <qgraphicsscene.h>
#include <QAccessible>

QString translateRole(QAccessible::Role role);

class OptionsWidget;
class MouseInterceptingGraphicsScene;
class QGraphicsView;
class QGraphicsScene;
class AccessibilitySceneManager;
class ScreenReader;
class AccessibilityInspector : public QObject
{
    Q_OBJECT
public:
    explicit AccessibilityInspector(QObject *parent = nullptr);
    ~AccessibilityInspector();
    void inspectWindow(QWindow *window);
    void saveWindowGeometry();
signals:

private:
    OptionsWidget *optionsWidget;
    MouseInterceptingGraphicsScene *accessibilityScene;
    QGraphicsView *accessibilityView;
    QGraphicsScene *accessibilityTreeScene;
    QGraphicsView *accessibilityTreeView;
    ScreenReader *screenReader;
};

class MouseInterceptingGraphicsScene : public QGraphicsScene
{
Q_OBJECT
protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
signals:
    void mousePressed(const QPoint point);
    void mouseDobleClicked();
};

#endif // ACCESSIBILITYINSPECTOR_H
