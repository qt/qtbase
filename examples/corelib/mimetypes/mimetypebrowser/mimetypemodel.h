// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MIMETYPEMODEL_H
#define MIMETYPEMODEL_H

#include <QCoreApplication>
#include <QHash>
#include <QMimeType>
#include <QStandardItemModel>

class MimetypeModel : public QStandardItemModel
{
    Q_DECLARE_TR_FUNCTIONS(MimetypeModel)
public:
    enum Columns { NameColumn, ColumnCount };

    explicit MimetypeModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;

    QMimeType mimeType(const QModelIndex &) const;

    QModelIndex indexForMimeType(const QString &name) const
    { return m_nameIndexHash.value(name); }

    static QString formatMimeTypeInfo(const QMimeType &);

private:
    typedef QHash<QString, QModelIndex> NameIndexHash;

    void populate();

    NameIndexHash m_nameIndexHash;
};

#endif // MIMETYPEMODEL_H
