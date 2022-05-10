// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QTextDocument>
#include <QTextEdit>

namespace doc_src_richtext {

void wrapper() {
//! [0]
QTextDocument *newDocument = new QTextDocument;
//! [0]


//! [1]
QTextEdit *editor = new QTextEdit;
QTextDocument *editorDocument = editor->document();
//! [1]
Q_UNUSED(newDocument);
Q_UNUSED(editorDocument);
} // wrapper

void wrapper2() {
auto parent = new QTextEdit();
QString aStringContainingHTMLtext;
//! [2]
QTextEdit *editor = new QTextEdit(parent);
editor->setHtml(aStringContainingHTMLtext);
editor->show();
//! [2]


//! [3]
QTextDocument *document = editor->document();
//! [3]
Q_UNUSED(document);

//! [4]
QTextCursor cursor = editor->textCursor();
//! [4]


//! [5]
editor->setTextCursor(cursor);
//! [5]


QTextEdit textEdit;
QTextCursor textCursor;
QString paragraphText;
//! [6]
textEdit.show();

textCursor.beginEditBlock();

for (int i = 0; i < 1000; ++i) {
    textCursor.insertBlock();
    textCursor.insertText(paragraphText.at(i));
}

textCursor.endEditBlock();
//! [6]

} // wrapper2
} // doc_src_richtext
