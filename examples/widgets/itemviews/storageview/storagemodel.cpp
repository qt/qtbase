// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Ivan Komissarov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "storagemodel.h"

#include <QDir>
#include <QLocale>

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
