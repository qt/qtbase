// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
class AppendText : public QUndoCommand
{
public:
    AppendText(QString *doc, const QString &text)
        : m_document(doc), m_text(text) { setText("append text"); }
    void undo() override
        { m_document->chop(m_text.length()); }
    void redo() override
        { m_document->append(m_text); }
private:
    QString *m_document;
    QString m_text;
};
//! [0]


//! [1]
MyCommand *command1 = new MyCommand();
stack->push(command1);
MyCommand *command2 = new MyCommand();
stack->push(command2);

stack->undo();

MyCommand *command3 = new MyCommand();
stack->push(command3); // command2 gets deleted
//! [1]


//! [2]
QUndoCommand *insertRed = new QUndoCommand(); // an empty command
insertRed->setText("insert red text");

new InsertText(document, idx, text, insertRed); // becomes child of insertRed
new SetColor(document, idx, text.length(), Qt::red, insertRed);

stack.push(insertRed);
//! [2]


//! [3]
bool AppendText::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id()) // make sure other is also an AppendText command
        return false;
    m_text += static_cast<const AppendText*>(other)->m_text;
    return true;
}
//! [3]


//! [4]
stack.beginMacro("insert red text");
stack.push(new InsertText(document, idx, text));
stack.push(new SetColor(document, idx, text.length(), Qt::red));
stack.endMacro(); // indexChanged() is emitted
//! [4]


//! [5]
QUndoCommand *insertRed = new QUndoCommand(); // an empty command
insertRed->setText("insert red text");

new InsertText(document, idx, text, insertRed); // becomes child of insertRed
new SetColor(document, idx, text.length(), Qt::red, insertRed);

stack.push(insertRed);
//! [5]
