// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "droparea.h"

#include <QDragEnterEvent>
#include <QMimeData>

using namespace Qt::StringLiterals;

//! [DropArea constructor]
DropArea::DropArea(QWidget *parent)
    : QLabel(parent)
{
    setMinimumSize(200, 200);
    setFrameStyle(QFrame::Sunken | QFrame::StyledPanel);
    setAlignment(Qt::AlignCenter);
    setAcceptDrops(true);
    setAutoFillBackground(true);
    clear();
}
//! [DropArea constructor]

//! [dragEnterEvent() function]
void DropArea::dragEnterEvent(QDragEnterEvent *event)
{
    setText(tr("<drop content>"));
    setBackgroundRole(QPalette::Highlight);

    event->acceptProposedAction();
    emit changed(event->mimeData());
}
//! [dragEnterEvent() function]

//! [dragMoveEvent() function]
void DropArea::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}
//! [dragMoveEvent() function]

//! [dropEvent() function part1]
void DropArea::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
//! [dropEvent() function part1]

//! [dropEvent() function part2]
    if (mimeData->hasImage()) {
        setPixmap(qvariant_cast<QPixmap>(mimeData->imageData()));
    } else if (mimeData->hasFormat(u"text/markdown"_s)) {
        setText(QString::fromUtf8(mimeData->data(u"text/markdown"_s)));
        setTextFormat(Qt::MarkdownText);
    } else if (mimeData->hasHtml()) {
        setText(mimeData->html());
        setTextFormat(Qt::RichText);
    } else if (mimeData->hasText()) {
        setText(mimeData->text());
        setTextFormat(Qt::PlainText);
    } else if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        QString text;
        for (qsizetype i = 0, count = qMin(urlList.size(), qsizetype(32)); i < count; ++i)
            text += urlList.at(i).path() + u'\n';
        setText(text);
    } else {
        setText(tr("Cannot display data"));
    }
//! [dropEvent() function part2]

//! [dropEvent() function part3]
    setBackgroundRole(QPalette::Dark);
    event->acceptProposedAction();
}
//! [dropEvent() function part3]

//! [dragLeaveEvent() function]
void DropArea::dragLeaveEvent(QDragLeaveEvent *event)
{
    clear();
    event->accept();
}
//! [dragLeaveEvent() function]

//! [clear() function]
void DropArea::clear()
{
    setText(tr("<drop content>"));
    setBackgroundRole(QPalette::Dark);

    emit changed();
}
//! [clear() function]
