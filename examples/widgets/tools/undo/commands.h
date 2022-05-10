// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef COMMANDS_H
#define COMMANDS_H

#include <QUndoCommand>
#include "document.h"

class AddShapeCommand : public QUndoCommand
{
public:
    AddShapeCommand(Document *doc, const Shape &shape,
                    QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    Document *m_doc;
    Shape m_shape;
    QString m_shapeName;
};

class RemoveShapeCommand : public QUndoCommand
{
public:
    RemoveShapeCommand(Document *doc, const QString &shapeName,
                       QUndoCommand *parent = nullptr);
    void undo() override;
    void redo() override;

private:
    Document *m_doc;
    Shape m_shape;
    QString m_shapeName;
};

class SetShapeColorCommand : public QUndoCommand
{
public:
    SetShapeColorCommand(Document *doc, const QString &shapeName,
                         const QColor &color, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override;

private:
    Document *m_doc;
    QString m_shapeName;
    QColor m_oldColor;
    QColor m_newColor;
};

class SetShapeRectCommand : public QUndoCommand
{
public:
    SetShapeRectCommand(Document *doc, const QString &shapeName,
                        const QRect &rect, QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

    bool mergeWith(const QUndoCommand *command) override;
    int id() const override;

private:
    Document *m_doc;
    QString m_shapeName;
    QRect m_oldRect;
    QRect m_newRect;
};

#endif // COMMANDS_H
