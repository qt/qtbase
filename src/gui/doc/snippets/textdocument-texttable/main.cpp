// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QTextEdit>
#include <QTextTable>

int main(int argc, char * argv[])
{
    int rows = 6;
    int columns = 2;

    QApplication app(argc, argv);
    QTextEdit *textEdit = new QTextEdit;
    QTextCursor cursor(textEdit->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextTableFormat tableFormat;
    tableFormat.setAlignment(Qt::AlignHCenter);
    tableFormat.setCellPadding(2);
    tableFormat.setCellSpacing(2);
    QTextTable *table = cursor.insertTable(rows, columns);
    table->setFormat(tableFormat);

    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);

    QTextBlockFormat centerFormat;
    centerFormat.setAlignment(Qt::AlignHCenter);
    cursor.mergeBlockFormat(centerFormat);

    cursor = table->cellAt(0, 0).firstCursorPosition();
    cursor.insertText(("Details"), boldFormat);

    cursor = table->cellAt(1, 0).firstCursorPosition();
    cursor.insertText("Alan");

    cursor = table->cellAt(1, 1).firstCursorPosition();
    cursor.insertText("5, Pickety Street");

//! [0]
    table->mergeCells(0, 0, 1, 2);
//! [0] //! [1]
    table->splitCell(0, 0, 1, 1);
//! [1]

    textEdit->show();
    return app.exec();
}
