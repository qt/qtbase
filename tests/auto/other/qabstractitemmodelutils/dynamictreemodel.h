// Copyright (C) 2009 Stephen Kelly <steveire@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DYNAMICTREEMODEL_H
#define DYNAMICTREEMODEL_H

#include <QtCore/QAbstractItemModel>

#include <QtCore/QHash>
#include <QtCore/QList>

class DynamicTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    DynamicTreeModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &index = QModelIndex()) const override;
    int columnCount(const QModelIndex &index = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void clear();

protected slots:

    /**
    Finds the parent id of the string with id @p searchId.

    Returns -1 if not found.
    */
    qint64 findParentId(qint64 searchId) const;

private:
    QHash<qint64, QString> m_items;
    QHash<qint64, QList<QList<qint64> > > m_childItems;
    qint64 nextId;
    qint64 newId()
    {
        return nextId++;
    }

    QModelIndex m_nextParentIndex;
    int m_nextRow;

    int m_depth;
    int maxDepth;

    friend class ModelInsertCommand;
    friend class ModelMoveCommand;
    friend class ModelResetCommand;
    friend class ModelResetCommandFixed;
    friend class ModelChangeChildrenLayoutsCommand;
};

class ModelChangeCommand : public QObject
{
    Q_OBJECT
public:

    ModelChangeCommand(DynamicTreeModel *model, QObject *parent = nullptr);

    virtual ~ModelChangeCommand()
    {
    }

    void setAncestorRowNumbers(QList<int> rowNumbers)
    {
        m_rowNumbers = rowNumbers;
    }

    QModelIndex findIndex(const QList<int> &rows) const;

    void setStartRow(int row)
    {
        m_startRow = row;
    }

    void setEndRow(int row)
    {
        m_endRow = row;
    }

    void setNumCols(int cols)
    {
        m_numCols = cols;
    }

    virtual void doCommand() = 0;

protected:
    DynamicTreeModel *m_model;
    QList<int> m_rowNumbers;
    int m_numCols;
    int m_startRow;
    int m_endRow;
};

typedef QList<ModelChangeCommand *> ModelChangeCommandList;

class ModelInsertCommand : public ModelChangeCommand
{
    Q_OBJECT

public:

    ModelInsertCommand(DynamicTreeModel *model, QObject *parent = nullptr);
    virtual ~ModelInsertCommand()
    {
    }

    virtual void doCommand() override;
};

class ModelMoveCommand : public ModelChangeCommand
{
    Q_OBJECT
public:
    ModelMoveCommand(DynamicTreeModel *model, QObject *parent);

    virtual ~ModelMoveCommand()
    {
    }

    virtual bool emitPreSignal(const QModelIndex &srcParent, int srcStart, int srcEnd,
                               const QModelIndex &destParent, int destRow);

    virtual void doCommand() override;

    virtual void emitPostSignal();

    void setDestAncestors(QList<int> rows)
    {
        m_destRowNumbers = rows;
    }

    void setDestRow(int row)
    {
        m_destRow = row;
    }

protected:
    QList<int> m_destRowNumbers;
    int m_destRow;
};

/**
  A command which does a move and emits a reset signal.
*/
class ModelResetCommand : public ModelMoveCommand
{
    Q_OBJECT
public:
    ModelResetCommand(DynamicTreeModel *model, QObject *parent = nullptr);

    virtual ~ModelResetCommand();

    virtual bool emitPreSignal(const QModelIndex &srcParent, int srcStart, int srcEnd,
                               const QModelIndex &destParent, int destRow) override;
    virtual void emitPostSignal() override;
};

/**
  A command which does a move and emits a beginResetModel and endResetModel signals.
*/
class ModelResetCommandFixed : public ModelMoveCommand
{
    Q_OBJECT
public:
    ModelResetCommandFixed(DynamicTreeModel *model, QObject *parent = nullptr);

    virtual ~ModelResetCommandFixed();

    virtual bool emitPreSignal(const QModelIndex &srcParent, int srcStart, int srcEnd,
                               const QModelIndex &destParent, int destRow) override;
    virtual void emitPostSignal() override;
};

class ModelChangeChildrenLayoutsCommand : public ModelChangeCommand
{
    Q_OBJECT
public:
    ModelChangeChildrenLayoutsCommand(DynamicTreeModel *model, QObject *parent);

    virtual ~ModelChangeChildrenLayoutsCommand()
    {
    }

    virtual void doCommand() override;

    void setSecondAncestorRowNumbers(QList<int> rows)
    {
        m_secondRowNumbers = rows;
    }

protected:
    QList<int> m_secondRowNumbers;
    int m_destRow;
};

#endif
