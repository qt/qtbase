// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SHORTCUTEDITORMODEL_H
#define SHORTCUTEDITORMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

enum class Column : uint8_t {
    Name,
    Shortcut
};

//! [0]
class ShortcutEditorModel : public QAbstractItemModel
{
    Q_OBJECT

    class ShortcutEditorModelItem
    {
    public:
        explicit ShortcutEditorModelItem(const QList<QVariant> &data,
                                         ShortcutEditorModelItem *parentItem = nullptr);
        ~ShortcutEditorModelItem();

        void appendChild(ShortcutEditorModelItem *child);

        ShortcutEditorModelItem *child(int row) const;
        int childCount() const;
        int columnCount() const;
        QVariant data(int column) const;
        int row() const;
        ShortcutEditorModelItem *parentItem() const;
        QAction *action() const;

    private:
        QList<ShortcutEditorModelItem *> m_childItems;
        QList<QVariant> m_itemData;
        ShortcutEditorModelItem *m_parentItem;
    };

public:
    explicit ShortcutEditorModel(QObject *parent = nullptr);
    ~ShortcutEditorModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &index = QModelIndex()) const override;
    int columnCount(const QModelIndex &index = QModelIndex()) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void setActions();

private:
    void setupModelData(ShortcutEditorModelItem *parent);

    ShortcutEditorModelItem *m_rootItem;
};
//! [0]

#endif
