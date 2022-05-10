// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "filelistmodel.h"

#include <QGuiApplication>
#include <QDir>
#include <QPalette>

static const int batchSize = 100;

FileListModel::FileListModel(QObject *parent)
    : QAbstractListModel(parent)
{}

//![4]
int FileListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : fileCount;
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    if (row >= fileList.size() || row < 0)
        return {};

    switch (role) {
    case Qt::DisplayRole:
        return fileList.at(row).fileName();
    case Qt::BackgroundRole: {
        const int batch = row / batchSize;
        const QPalette &palette = QGuiApplication::palette();
        return (batch % 2) != 0 ? palette.alternateBase() : palette.base();
    }
    case Qt::DecorationRole:
        return iconProvider.icon(fileList.at(row));
    }
    return {};
}

//![4]

QFileInfo FileListModel::fileInfoAt(const QModelIndex &index) const
{
    return fileList.at(index.row());
}

//![1]
bool FileListModel::canFetchMore(const QModelIndex &parent) const
{
    if (parent.isValid())
        return false;
    return (fileCount < fileList.size());
}
//![1]

//![2]
void FileListModel::fetchMore(const QModelIndex &parent)
{
    if (parent.isValid())
        return;
    const int start = fileCount;
    const int remainder = int(fileList.size()) - start;
    const int itemsToFetch = qMin(batchSize, remainder);

    if (itemsToFetch <= 0)
        return;

    beginInsertRows(QModelIndex(), start, start + itemsToFetch - 1);

    fileCount += itemsToFetch;

    endInsertRows();

    emit numberPopulated(path, start, itemsToFetch, int(fileList.size()));
}
//![2]

//![0]
void FileListModel::setDirPath(const QString &path)
{
    QDir dir(path);

    beginResetModel();
    this->path = path;
    fileList = dir.entryInfoList(QDir::NoDot | QDir::AllEntries, QDir::Name);
    fileCount = 0;
    endResetModel();
}
//![0]

