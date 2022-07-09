// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "shortcuteditordelegate.h"

#include <QAbstractItemModel>
#include <QKeySequenceEdit>

//! [0]
ShortcutEditorDelegate::ShortcutEditorDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}
//! [0]

//! [1]
QWidget *ShortcutEditorDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem &/*option*/,
                                              const QModelIndex &/*index*/) const
{
    QKeySequenceEdit *editor = new QKeySequenceEdit(parent);
    connect(editor, &QKeySequenceEdit::editingFinished, this, &ShortcutEditorDelegate::commitAndCloseEditor);
    return editor;
}
//! [1]

//! [2]
void ShortcutEditorDelegate::commitAndCloseEditor()
{
    QKeySequenceEdit *editor = static_cast<QKeySequenceEdit *>(sender());
    Q_EMIT commitData(editor);
    Q_EMIT closeEditor(editor);
}
//! [2]

//! [3]
void ShortcutEditorDelegate::setEditorData(QWidget *editor,
                                           const QModelIndex &index) const
{
    if (!editor || !index.isValid())
        return;

    QString value = index.model()->data(index, Qt::EditRole).toString();

    QKeySequenceEdit *keySequenceEdit = static_cast<QKeySequenceEdit *>(editor);
    keySequenceEdit->setKeySequence(value);
}
//! [3]

//! [4]
void ShortcutEditorDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                          const QModelIndex &index) const
{
    if (!editor || !model || !index.isValid())
        return;

    const QKeySequenceEdit *keySequenceEdit = static_cast<QKeySequenceEdit *>(editor);
    const QKeySequence keySequence = keySequenceEdit->keySequence();
    QString keySequenceString = keySequence.toString(QKeySequence::NativeText);
    model->setData(index, keySequenceString);
}
//! [4]

//! [5]
void ShortcutEditorDelegate::updateEditorGeometry(QWidget *editor,
                                                  const QStyleOptionViewItem &option,
                                                  const QModelIndex &/*index*/) const
{
    editor->setGeometry(option.rect);
}
//! [5]
