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
    QTextEdit *editor = new QTextEdit();

    QTextCursor cursor(editor->textCursor());
    cursor.movePosition(QTextCursor::Start);

    QTextCharFormat plainFormat(cursor.charFormat());

    QTextCharFormat headingFormat = plainFormat;
    headingFormat.setFontWeight(QFont::Bold);
    headingFormat.setFontPointSize(16);

    QTextCharFormat emphasisFormat = plainFormat;
    emphasisFormat.setFontItalic(true);

    QTextCharFormat qtFormat = plainFormat;
    qtFormat.setForeground(QColor("#990000"));

    QTextCharFormat underlineFormat = plainFormat;
    underlineFormat.setFontUnderline(true);

//! [0]
    cursor.insertText(tr("Character formats"),
                      headingFormat);

    cursor.insertBlock();

    cursor.insertText(tr("Text can be displayed in a variety of "
                                  "different character formats. "), plainFormat);
    cursor.insertText(tr("We can emphasize text by "));
    cursor.insertText(tr("making it italic"), emphasisFormat);
//! [0]
    cursor.insertText(tr(", give it a "), plainFormat);
    cursor.insertText(tr("different color "), qtFormat);
    cursor.insertText(tr("to the default text color, "), plainFormat);
    cursor.insertText(tr("underline it"), underlineFormat);
    cursor.insertText(tr(", and use many other effects."), plainFormat);

    editor->setWindowTitle(tr("Text Document Character Formats"));
    editor->resize(320, 480);
    editor->show();
    return app.exec();
}
