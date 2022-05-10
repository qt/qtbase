// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "accessibilityinspector.h"

#include "screenreader.h"
#include "optionswidget.h"
#include "accessibilityscenemanager.h"

#include <QtDeclarative/QtDeclarative>

void MouseInterceptingGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    emit mousePressed(event->scenePos().toPoint());
    QGraphicsScene::mousePressEvent(event);
}

void MouseInterceptingGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    emit mouseDobleClicked();
    QGraphicsScene::mouseDoubleClickEvent(event);
}

AccessibilitySceneManager *sceneManager = 0;
QAccessible::UpdateHandler previousUpdateHandler = 0;
bool updateHandlerRecursion = false;
void accessibilityUpdateHandler(QAccessibleEvent *event)
{
    if (updateHandlerRecursion)
        return;

    updateHandlerRecursion = true;

    if (sceneManager) {
        sceneManager->handleUpdate(event);

        //qDebug() << "update";
    }

    if (previousUpdateHandler) // call prev just to be sure.
        previousUpdateHandler(event);

    updateHandlerRecursion = false;
}

AccessibilityInspector::AccessibilityInspector(QObject *parent) :
    QObject(parent)
{
}

AccessibilityInspector::~AccessibilityInspector()
{
    delete optionsWidget;
    delete accessibilityScene;
    delete accessibilityView;
    delete accessibilityTreeScene;
    delete accessibilityTreeView;
    delete screenReader;
}

void AccessibilityInspector::inspectWindow(QWindow *window)
{
    qDebug() << "AccessibilityInspector::inspectWindow()" << window;
    if (window->parent() || window->transientParent())
        return;

    optionsWidget = new OptionsWidget();

    accessibilityScene = new MouseInterceptingGraphicsScene();

    accessibilityView = new QGraphicsView();
    accessibilityView->setScene(accessibilityScene);
    accessibilityView->resize(640, 480);
    accessibilityView->scale(1.3, 1.3);

    accessibilityTreeScene = new QGraphicsScene();

    accessibilityTreeView = new QGraphicsView();
    accessibilityTreeView->setScene(accessibilityTreeScene);
    accessibilityTreeView->resize(640, 480);

    sceneManager = new AccessibilitySceneManager();
    QObject::connect(optionsWidget, SIGNAL(optionsChanged()), sceneManager, SLOT(updateAccessibilitySceneItemFlags()));
    QObject::connect(optionsWidget, SIGNAL(refreshClicked()), sceneManager, SLOT(populateAccessibilityScene()));
    QObject::connect(optionsWidget, SIGNAL(refreshClicked()), sceneManager, SLOT(populateAccessibilityTreeScene()));
    QObject::connect(optionsWidget, SIGNAL(scaleChanged(int)), sceneManager, SLOT(changeScale(int)));

    sceneManager->setOptionsWidget(optionsWidget);
    sceneManager->setRootWindow(window);
    sceneManager->setScene(accessibilityScene);
    sceneManager->setView(accessibilityView);
    sceneManager->setTreeScene(accessibilityTreeScene);
    sceneManager->setTreeView(accessibilityTreeView);

    screenReader = new ScreenReader;
    QObject::connect(accessibilityScene, SIGNAL(mousePressed(QPoint)), screenReader, SLOT(touchPoint(QPoint)));
    QObject::connect(accessibilityScene, SIGNAL(mouseDobleClicked()), screenReader, SLOT(activate()));
    QObject::connect(screenReader, SIGNAL(selected(QObject*)), sceneManager, SLOT(setSelected(QObject*)));
    screenReader->setRootObject(window);
    screenReader->setOptionsWidget(optionsWidget);

    previousUpdateHandler = QAccessible::installUpdateHandler(accessibilityUpdateHandler);

    QTimer::singleShot(100, sceneManager, SLOT(populateAccessibilityScene()));
    QTimer::singleShot(100, sceneManager, SLOT(populateAccessibilityTreeScene()));

    QSettings settings;
    accessibilityView->restoreGeometry(settings.value("accessiblityGeometry").toByteArray());
    accessibilityView->setObjectName(QLatin1String("accessibilityInspectorView"));
    accessibilityView->show();


    accessibilityTreeView->restoreGeometry(settings.value("treeGeometry").toByteArray());
    accessibilityTreeView->setObjectName(QLatin1String("accessibilityInspectorTreeView"));
    accessibilityTreeView->show();
    optionsWidget->restoreGeometry(settings.value("optionsGeometry").toByteArray());
    optionsWidget->setObjectName(QLatin1String("accessibilityInspectorOptions"));
    optionsWidget->show();
}

void AccessibilityInspector::saveWindowGeometry()
{
    QSettings settings;
    settings.setValue("accessiblityGeometry", accessibilityView->saveGeometry());
    settings.setValue("treeGeometry", accessibilityTreeView->saveGeometry());
    settings.setValue("optionsGeometry", optionsWidget->saveGeometry());
}

QString translateRole(QAccessible::Role role)
{
    return qAccessibleRoleString(role);
}

