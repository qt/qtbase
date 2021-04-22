/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

