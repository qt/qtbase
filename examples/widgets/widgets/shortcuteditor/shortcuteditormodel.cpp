// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "shortcuteditormodel.h"

#include "actionmanager.h"
#include "application.h"

#include <QAction>
#include <QModelIndex>

// List of actions for all categories
using CategoryActionsMap = QMap<QString, QList<QAction *>>;

// List of categories for all contexts
using ActionsMap = QMap<QString, CategoryActionsMap>;


ShortcutEditorModel::ShortcutEditorModelItem::ShortcutEditorModelItem(const QList<QVariant> &data, ShortcutEditorModelItem *parent)
    : m_itemData(data)
    , m_parentItem(parent)
{
}

ShortcutEditorModel::ShortcutEditorModelItem::~ShortcutEditorModelItem()
{
    qDeleteAll(m_childItems);
}

void ShortcutEditorModel::ShortcutEditorModelItem::appendChild(ShortcutEditorModelItem *item)
{
    m_childItems.push_back(item);
}

ShortcutEditorModel::ShortcutEditorModelItem *ShortcutEditorModel::ShortcutEditorModelItem::child(int row) const
{
    if (row < 0 || row >= m_childItems.size())
        return nullptr;

    return m_childItems.at(row);
}

int ShortcutEditorModel::ShortcutEditorModelItem::childCount() const
{
    return m_childItems.count();
}

int ShortcutEditorModel::ShortcutEditorModelItem::columnCount() const
{
    return m_itemData.count();
}

QVariant ShortcutEditorModel::ShortcutEditorModelItem::data(int column) const
{
    if (column < 0 || column >= m_itemData.size())
        return QVariant();

    QVariant columnVariant = m_itemData.at(column);
    if (column != static_cast<int>(Column::Shortcut) || columnVariant.canConvert<QString>())
        return columnVariant;

    QAction *action = static_cast<QAction *>(columnVariant.value<void *>());
    if (!action)
        return QVariant();

    QKeySequence keySequence = action->shortcut();
    QString keySequenceString = keySequence.toString(QKeySequence::NativeText);
    return keySequenceString;
}

ShortcutEditorModel::ShortcutEditorModelItem *ShortcutEditorModel::ShortcutEditorModelItem::parentItem() const
{
    return m_parentItem;
}

int ShortcutEditorModel::ShortcutEditorModelItem::row() const
{
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<ShortcutEditorModelItem*>(this));

    return 0;
}

QAction *ShortcutEditorModel::ShortcutEditorModelItem::action() const
{
    QVariant actionVariant = m_itemData.at(static_cast<int>(Column::Shortcut));
    return static_cast<QAction*>(actionVariant.value<void *>());
}


//! [0]
ShortcutEditorModel::ShortcutEditorModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    m_rootItem = new ShortcutEditorModelItem({tr("Name"), tr("Shortcut")});
}
//! [0]

//! [1]
ShortcutEditorModel::~ShortcutEditorModel()
{
    delete m_rootItem;
}
//! [1]

//! [2]
void ShortcutEditorModel::setActions()
{
    beginResetModel();
    setupModelData(m_rootItem);
    endResetModel();
}
//! [2]

//! [3]
QModelIndex ShortcutEditorModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    ShortcutEditorModelItem *parentItem;
    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ShortcutEditorModelItem*>(parent.internalPointer());

    ShortcutEditorModelItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}
//! [3]

//! [4]
QModelIndex ShortcutEditorModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    ShortcutEditorModelItem *childItem = static_cast<ShortcutEditorModelItem*>(index.internalPointer());
    ShortcutEditorModelItem *parentItem = childItem->parentItem();

    if (parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}
//! [4]

//! [5]
int ShortcutEditorModel::rowCount(const QModelIndex &parent) const
{
    ShortcutEditorModelItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<ShortcutEditorModelItem*>(parent.internalPointer());

    return parentItem->childCount();
}
//! [5]

//! [6]
int ShortcutEditorModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<ShortcutEditorModelItem*>(parent.internalPointer())->columnCount();

    return m_rootItem->columnCount();
}
//! [6]

//! [7]
QVariant ShortcutEditorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    ShortcutEditorModelItem *item = static_cast<ShortcutEditorModelItem*>(index.internalPointer());
    return item->data(index.column());
}
//! [7]

//! [8]
Qt::ItemFlags ShortcutEditorModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    Qt::ItemFlags modelFlags = QAbstractItemModel::flags(index);
    if (index.column() == static_cast<int>(Column::Shortcut))
        modelFlags |= Qt::ItemIsEditable;

    return modelFlags;
}
//! [8]

//! [9]
QVariant ShortcutEditorModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_rootItem->data(section);
    }

    return QVariant();
}
//! [9]

//! [10]
void ShortcutEditorModel::setupModelData(ShortcutEditorModelItem *parent)
{
    ActionsMap actionsMap;
    Application *application = static_cast<Application *>(QCoreApplication::instance());
    ActionManager *actionManager = application->actionManager();
    const QList<QAction *> registeredActions = actionManager->registeredActions();
    for (QAction *action : registeredActions) {
        QString context = actionManager->contextForAction(action);
        QString category = actionManager->categoryForAction(action);
        actionsMap[context][category].append(action);
    }

    QAction *nullAction = nullptr;
    const QString contextIdPrefix = "root";
    // Go through each context, one context - many categories each iteration
    for (const auto &contextLevel : actionsMap.keys()) {
        ShortcutEditorModelItem *contextLevelItem = new ShortcutEditorModelItem({contextLevel, QVariant::fromValue(nullAction)}, parent);
        parent->appendChild(contextLevelItem);

        // Go through each category, one category - many actions each iteration
        for (const auto &categoryLevel : actionsMap[contextLevel].keys()) {
            ShortcutEditorModelItem *categoryLevelItem = new ShortcutEditorModelItem({categoryLevel, QVariant::fromValue(nullAction)}, contextLevelItem);
            contextLevelItem->appendChild(categoryLevelItem);
            for (QAction *action : actionsMap[contextLevel][categoryLevel]) {
                QString name = action->text();
                if (name.isEmpty() || !action)
                    continue;

                ShortcutEditorModelItem *actionLevelItem = new ShortcutEditorModelItem({name, QVariant::fromValue(reinterpret_cast<void *>(action))}, categoryLevelItem);
                categoryLevelItem->appendChild(actionLevelItem);
            }
        }
    }
}
//! [10]

//! [11]
bool ShortcutEditorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole && index.column() == static_cast<int>(Column::Shortcut)) {
        QString keySequenceString = value.toString();
        ShortcutEditorModelItem *item = static_cast<ShortcutEditorModelItem *>(index.internalPointer());
        QAction *itemAction = item->action();
        if (itemAction) {
            if (keySequenceString == itemAction->shortcut().toString(QKeySequence::NativeText))
                return true;
            itemAction->setShortcut(keySequenceString);
        }
        Q_EMIT dataChanged(index, index);

        if (keySequenceString.isEmpty())
            return true;
    }

    return QAbstractItemModel::setData(index, value, role);
}
//! [11]
