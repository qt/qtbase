/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Ivan Komissarov
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

#include "storagemodel.h"

#include <QDir>
#include <QLocale>
#include <qmath.h>
#include <algorithm>
#include <cmath>

StorageModel::StorageModel(QObject *parent) :
    QAbstractTableModel(parent)
{
}

void StorageModel::refresh()
{
    beginResetModel();
    m_volumes = QStorageInfo::mountedVolumes();
    std::sort(m_volumes.begin(), m_volumes.end(),
              [](const QStorageInfo &st1, const QStorageInfo &st2) {
                  static const QString rootSortString = QStringLiteral(" ");
                  return (st1.isRoot() ? rootSortString : st1.rootPath())
                       < (st2.isRoot() ? rootSortString : st2.rootPath());
              });
    endResetModel();
}

int StorageModel::columnCount(const QModelIndex &/*parent*/) const
{
    return ColumnCount;
}

int StorageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_volumes.count();
}

Qt::ItemFlags StorageModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags result = QAbstractTableModel::flags(index);
    switch (index.column()) {
    case ColumnAvailable:
    case ColumnIsReady:
    case ColumnIsReadOnly:
    case ColumnIsValid:
        result |= Qt::ItemIsUserCheckable;
        break;
    default:
        break;
    }
    return result;
}

QVariant StorageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        const QStorageInfo &volume = m_volumes.at(index.row());
        switch (index.column()) {
        case ColumnRootPath:
            return QDir::toNativeSeparators(volume.rootPath());
        case ColumnName:
            return volume.name();
        case ColumnDevice:
            return volume.device();
        case ColumnFileSystemName:
            return volume.fileSystemType();
        case ColumnTotal:
            return QLocale().formattedDataSize(volume.bytesTotal());
        case ColumnFree:
            return QLocale().formattedDataSize(volume.bytesFree());
        case ColumnAvailable:
            return QLocale().formattedDataSize(volume.bytesAvailable());
        default:
            break;
        }
    } else if (role == Qt::CheckStateRole) {
        const QStorageInfo &volume = m_volumes.at(index.row());
        switch (index.column()) {
        case ColumnIsReady:
            return volume.isReady();
        case ColumnIsReadOnly:
            return volume.isReadOnly();
        case ColumnIsValid:
            return volume.isValid();
        default:
            break;
        }
    } else if (role == Qt::TextAlignmentRole) {
        switch (index.column()) {
        case ColumnTotal:
        case ColumnFree:
        case ColumnAvailable:
            return Qt::AlignTrailing;
        default:
            break;
        }
        return Qt::AlignLeading;
    } else if (role == Qt::ToolTipRole) {
        QLocale locale;
        const QStorageInfo &volume = m_volumes.at(index.row());
        return tr("Root path : %1\n"
                  "Name: %2\n"
                  "Display Name: %3\n"
                  "Device: %4\n"
                  "FileSystem: %5\n"
                  "Total size: %6\n"
                  "Free size: %7\n"
                  "Available size: %8\n"
                  "Is Ready: %9\n"
                  "Is Read-only: %10\n"
                  "Is Valid: %11\n"
                  "Is Root: %12"
                  ).
                arg(QDir::toNativeSeparators(volume.rootPath())).
                arg(volume.name()).
                arg(volume.displayName()).
                arg(QString::fromUtf8(volume.device())).
                arg(QString::fromUtf8(volume.fileSystemType())).
                arg(locale.formattedDataSize(volume.bytesTotal())).
                arg(locale.formattedDataSize(volume.bytesFree())).
                arg(locale.formattedDataSize(volume.bytesAvailable())).
                arg(volume.isReady() ? tr("true") : tr("false")).
                arg(volume.isReadOnly() ? tr("true") : tr("false")).
                arg(volume.isValid() ? tr("true") : tr("false")).
                arg(volume.isRoot() ? tr("true") : tr("false"));
    }
    return QVariant();
}

QVariant StorageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case ColumnRootPath:
        return tr("Root Path");
    case ColumnName:
        return tr("Volume Name");
    case ColumnDevice:
        return tr("Device");
    case ColumnFileSystemName:
        return tr("File System");
    case ColumnTotal:
        return tr("Total");
    case ColumnFree:
        return tr("Free");
    case ColumnAvailable:
        return tr("Available");
    case ColumnIsReady:
        return tr("Ready");
    case ColumnIsReadOnly:
        return tr("Read-only");
    case ColumnIsValid:
        return tr("Valid");
    default:
        break;
    }

    return QVariant();
}
