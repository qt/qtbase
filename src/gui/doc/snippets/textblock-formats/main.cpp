// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QTextEdit>

QString tr(const char *text)
{
    return QApplication::translate(text, text);
}

int main_textblock_formats(int argc, char *argv[])
{
    QApplication app(argc, argv);

//! [0]
    QTextEdit *editor = new QTextEdit();
    QTextCursor cursor(editor->textCursor());
//! [0]
    cursor.movePosition(QTextCursor::Start);

    QTextBlockFormat blockFormat = cursor.blockFormat();
    blockFormat.setTopMargin(4);
    blockFormat.setLeftMargin(4);
    blockFormat.setRightMargin(4);
    blockFormat.setBottomMargin(4);

    cursor.setBlockFormat(blockFormat);
    cursor.insertText(tr("This contains plain text inside a "
                         "text block with margins to keep it separate "
                         "from other parts of the document."));

    cursor.insertBlock();

//! [1]
    QTextBlockFormat backgroundFormat = blockFormat;
    backgroundFormat.setBackground(QColor("lightGray"));

    cursor.setBlockFormat(backgroundFormat);
//! [1]
    cursor.insertText(tr("The background color of a text block can be "
                         "changed to highlight text."));

    cursor.insertBlock();

    QTextBlockFormat rightAlignedFormat = blockFormat;
    rightAlignedFormat.setAlignment(Qt::AlignRight);

    cursor.setBlockFormat(rightAlignedFormat);
    cursor.insertText(tr("The alignment of the text within a block is "
                         "controlled by the alignment properties of "
                         "the block itself. This text block is "
                         "right-aligned."));

    cursor.insertBlock();

    QTextBlockFormat paragraphFormat = blockFormat;
    paragraphFormat.setAlignment(Qt::AlignJustify);
    paragraphFormat.setTextIndent(32);

    cursor.setBlockFormat(paragraphFormat);
    cursor.insertText(tr("Text can be formatted so that the first "
                         "line in a paragraph has its own margin. "
                         "This makes the text more readable."));

    cursor.insertBlock();

    QTextBlockFormat reverseFormat = blockFormat;
    reverseFormat.setAlignment(Qt::AlignJustify);
    reverseFormat.setTextIndent(32);

    cursor.setBlockFormat(reverseFormat);
    cursor.insertText(tr("The direction of the text can be reversed. "
                         "This is useful for right-to-left "
                         "languages."));

    editor->setWindowTitle(tr("Text Block Formats"));
    editor->resize(480, 480);
    editor->show();

    return app.exec();
}
