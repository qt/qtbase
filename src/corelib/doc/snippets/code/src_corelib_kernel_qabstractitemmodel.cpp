// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
beginInsertRows(parent, 2, 4);
//! [0]


//! [1]
beginInsertRows(parent, 4, 5);
//! [1]


//! [2]
beginRemoveRows(parent, 2, 3);
//! [2]


//! [3]
beginInsertColumns(parent, 4, 6);
//! [3]


//! [4]
beginInsertColumns(parent, 6, 8);
//! [4]


//! [5]
beginRemoveColumns(parent, 4, 6);
//! [5]


//! [6]
beginMoveRows(sourceParent, 2, 4, destinationParent, 2);
//! [6]


//! [7]
beginMoveRows(sourceParent, 2, 4, destinationParent, 6);
//! [7]


//! [8]
beginMoveRows(parent, 2, 2, parent, 0);
//! [8]


//! [9]
beginMoveRows(parent, 2, 2, parent, 4);
//! [9]


//! [12]
class CustomDataProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CustomDataProxy(QObject *parent)
      : QSortFilterProxyModel(parent)
    {
    }

    ...

    QVariant data(const QModelIndex &index, int role) override
    {
        if (role != Qt::BackgroundRole)
            return QSortFilterProxyModel::data(index, role);

        if (m_customData.contains(index.row()))
            return m_customData.value(index.row());
        return QSortFilterProxyModel::data(index, role);
    }

private slots:
    void resetInternalData()
    {
        m_customData.clear();
    }

private:
  QHash<int, QVariant> m_customData;
};
//! [12]

//! [13]
QVariant text = model->data(index, Qt::DisplayRole);
QVariant decoration = model->data(index, Qt::DecorationRole);
QVariant checkState = model->data(index, Qt::CheckStateRole);
// etc.
//! [13]

//! [14]
std::array<QModelRoleData, 3> roleData = { {
    QModelRoleData(Qt::DisplayRole),
    QModelRoleData(Qt::DecorationRole),
    QModelRoleData(Qt::CheckStateRole)
} };

// Usually, this is not necessary: A QModelRoleDataSpan
// will be built automatically for you when passing an array-like
// container to multiData().
QModelRoleDataSpan span(roleData);

model->multiData(index, span);

// Use roleData[0].data(), roleData[1].data(), etc.
//! [14]

//! [15]
void MyModel::multiData(const QModelIndex &index, QModelRoleDataSpan roleDataSpan) const
{
    for (QModelRoleData &roleData : roleDataSpan) {
        int role = roleData.role();

        // ... obtain the data for index and role ...

        roleData.setData(result);
    }
}
//! [15]

//! [16]
QVariant MyModel::data(const QModelIndex &index, int role) const
{
    QModelRoleData roleData(role);
    multiData(index, roleData);
    return roleData.data();
}
//! [16]
