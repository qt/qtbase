/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "commands.h"

static constexpr int setShapeRectCommandId = 1;
static constexpr int setShapeColorCommandId = 2;

/******************************************************************************
** AddShapeCommand
*/

AddShapeCommand::AddShapeCommand(Document *doc, const Shape &shape, QUndoCommand *parent)
    : QUndoCommand(parent), m_doc(doc), m_shape(shape)
{
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
    : QUndoCommand(parent), m_doc(doc), m_shape(doc->shape(shapeName))
    , m_shapeName(shapeName)
{
    setText(QObject::tr("Remove %1").arg(shapeName));
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
    : QUndoCommand(parent), m_doc(doc), m_shapeName(shapeName)
    , m_oldColor(doc->shape(shapeName).color()), m_newColor(color)
{
    setText(QObject::tr("Set %1's color").arg(shapeName));
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
    : QUndoCommand(parent), m_doc(doc), m_shapeName(shapeName)
    , m_oldRect(doc->shape(shapeName).rect()), m_newRect(rect)
{
    setText(QObject::tr("Change %1's geometry").arg(shapeName));
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
