// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "treemodelcompleter.h"
#include <QStringList>

//! [0]
TreeModelCompleter::TreeModelCompleter(QObject *parent)
    : QCompleter(parent)
{
}
//! [0]

//! [1]
TreeModelCompleter::TreeModelCompleter(QAbstractItemModel *model, QObject *parent)
    : QCompleter(model, parent)
{
}
//! [1]

void TreeModelCompleter::setSeparator(const QString &separator)
{
    sep = separator;
}

//! [2]
QString TreeModelCompleter::separator() const
{
    return sep;
}
//! [2]

//! [3]
QStringList TreeModelCompleter::splitPath(const QString &path) const
{
    return (sep.isNull() ? QCompleter::splitPath(path) : path.split(sep));
}
//! [3]

//! [4]
QString TreeModelCompleter::pathFromIndex(const QModelIndex &index) const
{
    if (sep.isNull())
        return QCompleter::pathFromIndex(index);

    // navigate up and accumulate data
    QStringList dataList;
    for (QModelIndex i = index; i.isValid(); i = i.parent())
        dataList.prepend(model()->data(i, completionRole()).toString());

    return dataList.join(sep);
}
//! [4]

