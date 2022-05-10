// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "commands.h"
#include "diagramitem.h"

#include <QGraphicsScene>

//! [0]
MoveCommand::MoveCommand(DiagramItem *diagramItem, const QPointF &oldPos,
                         QUndoCommand *parent)
    : QUndoCommand(parent), myDiagramItem(diagramItem)
    , myOldPos(oldPos), newPos(diagramItem->pos())
{
}
//! [0]

//! [1]
bool MoveCommand::mergeWith(const QUndoCommand *command)
{
    const MoveCommand *moveCommand = static_cast<const MoveCommand *>(command);
    DiagramItem *item = moveCommand->myDiagramItem;

    if (myDiagramItem != item)
        return false;

    newPos = item->pos();
    setText(QObject::tr("Move %1")
        .arg(createCommandString(myDiagramItem, newPos)));

    return true;
}
//! [1]

//! [2]
void MoveCommand::undo()
{
    myDiagramItem->setPos(myOldPos);
    myDiagramItem->scene()->update();
    setText(QObject::tr("Move %1")
        .arg(createCommandString(myDiagramItem, newPos)));
}
//! [2]

//! [3]
void MoveCommand::redo()
{
    myDiagramItem->setPos(newPos);
    setText(QObject::tr("Move %1")
        .arg(createCommandString(myDiagramItem, newPos)));
}
//! [3]

//! [4]
DeleteCommand::DeleteCommand(QGraphicsScene *scene, QUndoCommand *parent)
    : QUndoCommand(parent), myGraphicsScene(scene)
{
    QList<QGraphicsItem *> list = myGraphicsScene->selectedItems();
    list.first()->setSelected(false);
    myDiagramItem = static_cast<DiagramItem *>(list.first());
    setText(QObject::tr("Delete %1")
        .arg(createCommandString(myDiagramItem, myDiagramItem->pos())));
}
//! [4]

//! [5]
void DeleteCommand::undo()
{
    myGraphicsScene->addItem(myDiagramItem);
    myGraphicsScene->update();
}
//! [5]

//! [6]
void DeleteCommand::redo()
{
    myGraphicsScene->removeItem(myDiagramItem);
}
//! [6]

//! [7]
AddCommand::AddCommand(DiagramItem::DiagramType addType,
                       QGraphicsScene *scene, QUndoCommand *parent)
    : QUndoCommand(parent), myGraphicsScene(scene)
{
    static int itemCount = 0;

    myDiagramItem = new DiagramItem(addType);
    initialPosition = QPointF((itemCount * 15) % int(scene->width()),
                              (itemCount * 15) % int(scene->height()));
    scene->update();
    ++itemCount;
    setText(QObject::tr("Add %1")
        .arg(createCommandString(myDiagramItem, initialPosition)));
}
//! [7]

AddCommand::~AddCommand()
{
    if (!myDiagramItem->scene())
        delete myDiagramItem;
}

//! [8]
void AddCommand::undo()
{
    myGraphicsScene->removeItem(myDiagramItem);
    myGraphicsScene->update();
}
//! [8]

//! [9]
void AddCommand::redo()
{
    myGraphicsScene->addItem(myDiagramItem);
    myDiagramItem->setPos(initialPosition);
    myGraphicsScene->clearSelection();
    myGraphicsScene->update();
}
//! [9]

QString createCommandString(DiagramItem *item, const QPointF &pos)
{
    return QObject::tr("%1 at (%2, %3)")
        .arg(item->diagramType() == DiagramItem::Box ? "Box" : "Triangle")
        .arg(pos.x()).arg(pos.y());
}
