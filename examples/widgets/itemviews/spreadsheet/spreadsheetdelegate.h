// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADSHEETDELEGATE_H
#define SPREADSHEETDELEGATE_H

#include <QStyledItemDelegate>

class SpreadSheetDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    SpreadSheetDelegate(QObject *parent = nullptr);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

private slots:
    void commitAndCloseEditor();
};

#endif // SPREADSHEETDELEGATE_H

