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

    QTextDocument *document = new QTextDocument(editor);
    QTextCursor cursor(document);

    QImage image(64, 64, QImage::Format_RGB32);
    image.fill(qRgb(255, 160, 128));

//! [Adding a resource]
    document->addResource(QTextDocument::ImageResource,
        QUrl("mydata://image.png"), QVariant(image));
//! [Adding a resource]

//! [Inserting an image with a cursor]
    QTextImageFormat imageFormat;
    imageFormat.setName("mydata://image.png");
    cursor.insertImage(imageFormat);
//! [Inserting an image with a cursor]

    cursor.insertBlock();
    cursor.insertText("Code less. Create more.");

    editor->setDocument(document);
    editor->setWindowTitle(tr("Text Document Images"));
    editor->resize(320, 480);
    editor->show();

//! [Inserting an image using HTML]
    editor->append("<img src=\"mydata://image.png\" />");
//! [Inserting an image using HTML]

    return app.exec();
}
