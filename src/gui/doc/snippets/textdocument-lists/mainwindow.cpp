// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QTextEdit>
#include <QTextList>

namespace textdocument_lists {
struct MainWindow
{
    void insertList();

private:
    QTextEdit *editor = nullptr;
};

void MainWindow::insertList()
{
    QTextCursor cursor = editor->textCursor();
    QTextList *list = cursor.currentList();

//! [0]
    QTextListFormat listFormat;
    if (list) {
        listFormat = list->format();
        listFormat.setIndent(listFormat.indent() + 1);
    }

    listFormat.setStyle(QTextListFormat::ListDisc);
    cursor.insertList(listFormat);
//! [0]
}

} //textdocument_lists
