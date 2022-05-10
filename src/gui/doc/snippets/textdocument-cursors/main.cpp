// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QTextEdit>

QString tr(const char *text)
{
    return QApplication::translate(text, text);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextEdit *editor = new QTextEdit;

//! [0]
    QTextDocument *document = editor->document();
    QTextCursor redCursor(document);
//! [0] //! [1]
    QTextCursor blueCursor(document);
//! [1]

    QTextCharFormat redFormat(redCursor.charFormat());
    redFormat.setForeground(Qt::red);
    QTextCharFormat blueFormat(blueCursor.charFormat());
    blueFormat.setForeground(Qt::blue);

    redCursor.setCharFormat(redFormat);
    blueCursor.setCharFormat(blueFormat);

    for (int i = 0; i < 20; ++i) {
        if (i % 2 == 0)
            redCursor.insertText(tr("%1 ").arg(i), redFormat);
        if (i % 5 == 0)
            blueCursor.insertText(tr("%1 ").arg(i), blueFormat);
    }

    editor->setWindowTitle(tr("Text Document Cursors"));
    editor->resize(320, 480);
    editor->show();
    return app.exec();
}
