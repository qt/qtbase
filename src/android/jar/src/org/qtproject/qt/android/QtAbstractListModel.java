// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

public abstract class QtAbstractListModel extends QtAbstractItemModel
{
    public QtAbstractListModel(){}

    @Override public final int columnCount(QtModelIndex parent) { return parent.isValid() ? 0 : 1; }

    @Override public QtModelIndex index(int row, int column, QtModelIndex parent)
    {
        return hasIndex(row, column, parent) ? createIndex(row, column, 0) : new QtModelIndex();
    }

    @Override public final QtModelIndex parent(QtModelIndex index) { return new QtModelIndex(); }

    @Override public final boolean hasChildren(QtModelIndex parent)
    {
        return !parent.isValid() && (rowCount(new QtModelIndex()) > 0);
    }

    @Override public QtModelIndex sibling(int row, int column, QtModelIndex parent)
    {
        return index(row, column, new QtModelIndex());
    }
}
