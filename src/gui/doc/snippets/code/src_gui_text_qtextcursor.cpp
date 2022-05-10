// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QImage>
#include <QTextCursor>
#include <QTextDocument>

namespace src_gui_text_qtextcursor {
QTextDocument *textDocument = nullptr;

void wrapper0() {
QTextCursor cursor;


//! [0]
cursor.clearSelection();
cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
cursor.insertText("Hello World");
//! [0]


//! [1]
QImage img;
textDocument->addResource(QTextDocument::ImageResource, QUrl("myimage"), img);
cursor.insertImage("myimage");
//! [1]

} // wrapper0


void wrapper1() {
//! [2]
QTextCursor cursor(textDocument);
cursor.beginEditBlock();
cursor.insertText("Hello");
cursor.insertText("World");
cursor.endEditBlock();

textDocument->undo();
//! [2]
} // wrapper1


void wrapper2() {
//! [3]
QTextCursor cursor(textDocument);
cursor.beginEditBlock();
cursor.insertText("Hello");
cursor.insertText("World");
cursor.endEditBlock();

// ...

cursor.joinPreviousEditBlock();
cursor.insertText("Hey");
cursor.endEditBlock();

textDocument->undo();
//! [3]
} // wrapper2

} // src_gui_text_qtextcursor
