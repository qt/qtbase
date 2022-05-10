// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QTextEdit>

void mergeFormat(QTextEdit *edit)
{
//! [0]
QTextDocument *document = edit->document();
QTextCursor cursor(document);

cursor.movePosition(QTextCursor::Start);
cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);

QTextCharFormat format;
format.setFontWeight(QFont::Bold);

cursor.mergeCharFormat(format);
//! [0]
}

int main(int argc, char *argv[])
{
QWidget *parent = nullptr;
QString aStringContainingHTMLtext("<h1>Scribe Overview</h1>");

QApplication app(argc, argv);

//! [1]
QTextEdit *editor = new QTextEdit(parent);
editor->setHtml(aStringContainingHTMLtext);
editor->show();
//! [1]

return app.exec();
}
