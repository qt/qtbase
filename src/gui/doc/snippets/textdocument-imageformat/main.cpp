// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QApplication>
#include <QTextBlock>
#include <QTextEdit>

QString tr(const char *text)
{
    return QApplication::translate(text, text);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTextEdit *editor = new QTextEdit;

    QTextDocument *document = new QTextDocument(editor);
    QTextCursor cursor(document);

    QTextImageFormat imageFormat;
    imageFormat.setName(":/images/advert.png");
    cursor.insertImage(imageFormat);

    QTextBlock block = cursor.block();
    QTextFragment fragment;
    QTextBlock::iterator it;

    for (it = block.begin(); !(it.atEnd()); ++it) {
        fragment = it.fragment();

        if (fragment.contains(cursor.position()))
            break;
    }

//! [0]
    if (fragment.isValid()) {
        QTextImageFormat newImageFormat = fragment.charFormat().toImageFormat();

        if (newImageFormat.isValid()) {
            newImageFormat.setName(":/images/newimage.png");
            QTextCursor helper = cursor;

            helper.setPosition(fragment.position());
            helper.setPosition(fragment.position() + fragment.length(),
                                QTextCursor::KeepAnchor);
            helper.setCharFormat(newImageFormat);
//! [0] //! [1]
        }
//! [1] //! [2]
    }
//! [2]

    cursor.insertBlock();
    cursor.insertText("Code less. Create more.");

    editor->setDocument(document);
    editor->setWindowTitle(tr("Text Document Image Format"));
    editor->resize(320, 480);
    editor->show();

    return app.exec();
}
