/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
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
