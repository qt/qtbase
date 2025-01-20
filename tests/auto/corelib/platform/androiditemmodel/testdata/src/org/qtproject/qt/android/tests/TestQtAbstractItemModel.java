// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.tests;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.qtproject.qt.android.QtAbstractItemModel;
import org.qtproject.qt.android.QtModelIndex;

public class TestQtAbstractItemModel
        extends QtAbstractItemModel implements QtAbstractItemModel.OnDataChangedListener
{
    static final int QT_USER_ROLE = 0x100;
    static final int ROLE_STRING = QT_USER_ROLE;
    static final int ROLE_BOOLEAN = QT_USER_ROLE + 1;
    static final int ROLE_INTEGER = QT_USER_ROLE + 2;
    static final int ROLE_DOUBLE = QT_USER_ROLE + 3;
    static final int ROLE_LONG = QT_USER_ROLE + 4;

    int m_rows = 0;
    int m_cols = 0;
    int m_dataChangedCount = 0;

    public TestQtAbstractItemModel()
    {
        setOnDataChangedListener(this);
    }

    @Override
    public int columnCount(QtModelIndex parent)
    {
        return parent.isValid() ? 0 : m_cols;
    }

    @Override
    public Object data(QtModelIndex index, int role)
    {
        int r = index.row();
        int c = index.column();
        if (r < 0 || c < 0 || c > m_cols || r > m_rows)
            return null;

        switch (role) {
        case ROLE_STRING:
            return String.format("r%d/c%d", r, c);
        case ROLE_BOOLEAN:
            return new Boolean(((r + c) % 2) == 0);
        case ROLE_INTEGER:
            return new Integer((c << 8) + r);
        case ROLE_DOUBLE:
            return new Double((r + 1.0) / (c + 1.0));
        case ROLE_LONG:
            return new Long((c << 8) * (r << 8));
        default:
            return null;
        }
    }

    @Override
    public QtModelIndex index(int row, int column, QtModelIndex parent)
    {
        return hasIndex(row, column, parent) ? createIndex(row, column, 0) : new QtModelIndex();
    }

    @Override
    public QtModelIndex parent(QtModelIndex qtModelIndex)
    {
        return new QtModelIndex();
    }

    @Override
    public int rowCount(QtModelIndex parent)
    {
        return parent.isValid() ? 0 : m_rows;
    }

    @Override
    public HashMap<Integer, String> roleNames()
    {
        final HashMap<Integer, String> roles = new HashMap<Integer, String>();
        roles.put(ROLE_STRING, "stringRole");
        roles.put(ROLE_BOOLEAN, "booleanRole");
        roles.put(ROLE_INTEGER, "integerRole");
        roles.put(ROLE_DOUBLE, "doubleRole");
        roles.put(ROLE_LONG, "longRole");
        return roles;
    }

    @Override
    public boolean canFetchMore(QtModelIndex parent)
    {
        return !parent.isValid() && (m_rows < 30);
    }

    @Override
    public void fetchMore(QtModelIndex parent)
    {
        if (!canFetchMore(parent))
            return;
        int toAdd = Math.min(10, 30 - rowCount(parent));
        beginInsertRows(new QtModelIndex(), m_rows, m_rows + toAdd - 1);
        m_rows += toAdd;
        endInsertRows();
    }

    @Override
    public boolean setData(QtModelIndex index, Object value, int role)
    {
        dataChanged(index, index , new int[]{role});
        return true;
    }

    @Override
    public void onDataChanged(QtModelIndex topLeft, QtModelIndex bottomRight, int[] roles) {
        m_dataChangedCount++;
    }

    public void addRow()
    {
        beginInsertRows(new QtModelIndex(), m_rows, m_rows);
        m_rows++;
        endInsertRows();
    }

    public void removeRow()
    {
        if (m_rows == 0)
            return;
        beginRemoveRows(new QtModelIndex(), 0, 0);
        m_rows--;
        endRemoveRows();
    }

    public void addCol()
    {
        beginInsertColumns(new QtModelIndex(), m_cols, m_cols);
        m_cols++;
        endInsertColumns();
    }

    public void removeCol()
    {
        if (m_cols == 0)
            return;
        beginRemoveColumns(new QtModelIndex(), 0, 0);
        m_cols--;
        endRemoveColumns();
    }

    public void reset()
    {
        beginResetModel();
        m_rows = 0;
        m_cols = 0;
        endResetModel();
        m_dataChangedCount = 0;
    }
}
