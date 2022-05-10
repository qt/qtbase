// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MIMETYPEMODEL_H
#define MIMETYPEMODEL_H

#include <QStandardItemModel>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QMimeType)

class MimetypeModel : public QStandardItemModel
{
    Q_OBJECT
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
