// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "textedit.h"
#include <QtGui>

TextEdit::TextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

//! [0]
bool TextEdit::canInsertFromMimeData( const QMimeData *source ) const
{
    if (source->hasImage())
        return true;
    else
        return QTextEdit::canInsertFromMimeData(source);
}
//! [0]

//! [1]
void TextEdit::insertFromMimeData( const QMimeData *source )
{
    if (source->hasImage())
    {
        QImage image = qvariant_cast<QImage>(source->imageData());
        QTextCursor cursor = this->textCursor();
        QTextDocument *document = this->document();
        document->addResource(QTextDocument::ImageResource, QUrl("image"), image);
        cursor.insertImage("image");
    }
}
//! [1]
