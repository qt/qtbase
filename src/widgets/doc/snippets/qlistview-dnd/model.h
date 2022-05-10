// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MODEL_H
#define MODEL_H

#include <QObject>
#include <QStringListModel>

class DragDropListModel : public QStringListModel
{
    Q_OBJECT

public:
    DragDropListModel(const QStringList &strings, QObject *parent = nullptr);

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                         int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
};

#endif
