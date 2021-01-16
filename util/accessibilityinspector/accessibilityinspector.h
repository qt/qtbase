/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

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
    explicit AccessibilityInspector(QObject *parent = 0);
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
