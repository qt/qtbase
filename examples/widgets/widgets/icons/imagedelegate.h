// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGEDELEGATE_H
#define IMAGEDELEGATE_H

#include <QStyledItemDelegate>

//! [0]
class ImageDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ImageDelegate(QObject *parent = nullptr);
//! [0]

//! [1]
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override;

//! [1] //! [2]
private slots:
    void emitCommitData();
//! [2]
};

#endif
