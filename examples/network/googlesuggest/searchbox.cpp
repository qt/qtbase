// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QDesktopServices>
#include <QUrl>

#include "searchbox.h"
#include "googlesuggest.h"

const QString gsearchUrl = QStringLiteral("http://www.google.com/search?q=%1");

//! [1]
SearchBox::SearchBox(QWidget *parent)
    : QLineEdit(parent)
    , completer(new GSuggestCompletion(this))
{
    connect(this, &SearchBox::returnPressed, this, &SearchBox::doSearch);

    setWindowTitle("Search with Google");

    adjustSize();
    resize(400, height());
    setFocus();
}
//! [1]

//! [2]
void SearchBox::doSearch()
{
    completer->preventSuggest();
    QString url = gsearchUrl.arg(text());
    QDesktopServices::openUrl(url);
}
//! [2]

