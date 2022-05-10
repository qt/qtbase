// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>

// With a QFileSystemModel, set on a view, you will see "Program Files" in the view
// But with this model, you will see "C:\Program Files" in the view.
// We achieve this, by having the data() return the entire file path for
// the display role. Note that the Qt::EditRole over which the QCompleter
// looks for matches is left unchanged
//! [0]
class FileSystemModel : public QFileSystemModel
{
public:
    FileSystemModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};
//! [0]

#endif
