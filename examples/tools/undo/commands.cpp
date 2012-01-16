/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the demonstration applications of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "commands.h"

static const int setShapeRectCommandId = 1;
static const int setShapeColorCommandId = 2;

/******************************************************************************
** AddShapeCommand
*/

AddShapeCommand::AddShapeCommand(Document *doc, const Shape &shape, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    m_doc = doc;
    m_shape = shape;
}

void AddShapeCommand::undo()
{
    m_doc->deleteShape(m_shapeName);
}

void AddShapeCommand::redo()
{
    // A shape only gets a name when it is inserted into a document
    m_shapeName = m_doc->addShape(m_shape);
    setText(QObject::tr("Add %1").arg(m_shapeName));
}

/******************************************************************************
** RemoveShapeCommand
*/

RemoveShapeCommand::RemoveShapeCommand(Document *doc, const QString &shapeName,
                                        QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText(QObject::tr("Remove %1").arg(shapeName));
    m_doc = doc;
    m_shape = doc->shape(shapeName);
    m_shapeName = shapeName;
}

void RemoveShapeCommand::undo()
{
    m_shapeName = m_doc->addShape(m_shape);
}

void RemoveShapeCommand::redo()
{
    m_doc->deleteShape(m_shapeName);
}

/******************************************************************************
** SetShapeColorCommand
*/

SetShapeColorCommand::SetShapeColorCommand(Document *doc, const QString &shapeName,
                                            const QColor &color, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText(QObject::tr("Set %1's color").arg(shapeName));

    m_doc = doc;
    m_shapeName = shapeName;
    m_oldColor = doc->shape(shapeName).color();
    m_newColor = color;
}

void SetShapeColorCommand::undo()
{
    m_doc->setShapeColor(m_shapeName, m_oldColor);
}

void SetShapeColorCommand::redo()
{
    m_doc->setShapeColor(m_shapeName, m_newColor);
}

bool SetShapeColorCommand::mergeWith(const QUndoCommand *command)
{
    if (command->id() != setShapeColorCommandId)
        return false;

    const SetShapeColorCommand *other = static_cast<const SetShapeColorCommand*>(command);
    if (m_shapeName != other->m_shapeName)
        return false;

    m_newColor = other->m_newColor;
    return true;
}

int SetShapeColorCommand::id() const
{
    return setShapeColorCommandId;
}

/******************************************************************************
** SetShapeRectCommand
*/

SetShapeRectCommand::SetShapeRectCommand(Document *doc, const QString &shapeName,
                                            const QRect &rect, QUndoCommand *parent)
    : QUndoCommand(parent)
{
    setText(QObject::tr("Change %1's geometry").arg(shapeName));

    m_doc = doc;
    m_shapeName = shapeName;
    m_oldRect = doc->shape(shapeName).rect();
    m_newRect = rect;
}

void SetShapeRectCommand::undo()
{
    m_doc->setShapeRect(m_shapeName, m_oldRect);
}

void SetShapeRectCommand::redo()
{
    m_doc->setShapeRect(m_shapeName, m_newRect);
}

bool SetShapeRectCommand::mergeWith(const QUndoCommand *command)
{
    if (command->id() != setShapeRectCommandId)
        return false;

    const SetShapeRectCommand *other = static_cast<const SetShapeRectCommand*>(command);
    if (m_shapeName != other->m_shapeName)
        return false;

    m_newRect = other->m_newRect;
    return true;
}

int SetShapeRectCommand::id() const
{
    return setShapeRectCommandId;
}
