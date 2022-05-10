// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

class TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit TextEdit(QWidget *parent = nullptr);
    bool canInsertFromMimeData( const QMimeData *source ) const override;
    void insertFromMimeData( const QMimeData *source ) override;
};

#endif
