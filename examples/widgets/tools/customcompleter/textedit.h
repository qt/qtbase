// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TEXTEDIT_H
#define TEXTEDIT_H

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QCompleter;
QT_END_NAMESPACE

//! [0]
class TextEdit : public QTextEdit
{
    Q_OBJECT

public:
    TextEdit(QWidget *parent = nullptr);
    ~TextEdit();

    void setCompleter(QCompleter *c);
    QCompleter *completer() const;

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;

private slots:
    void insertCompletion(const QString &completion);

private:
    QString textUnderCursor() const;

private:
    QCompleter *c = nullptr;
};
//! [0]

#endif // TEXTEDIT_H

