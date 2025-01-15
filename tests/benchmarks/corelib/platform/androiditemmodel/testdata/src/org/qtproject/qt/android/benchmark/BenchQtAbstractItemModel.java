// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.benchmark;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import org.qtproject.qt.android.QtAbstractItemModel;
import org.qtproject.qt.android.QtModelIndex;

public class BenchQtAbstractItemModel
        extends QtAbstractItemModel
{
    int m_rows = 1;
    int m_cols = 1;

    @Override
    public int columnCount(QtModelIndex parent)
    {
        return m_cols;
    }

    @Override
    public Object data(QtModelIndex index, int role)
    {
        return null;
    }

    @Override
    public QtModelIndex index(int row, int column, QtModelIndex parent)
    {
        return createIndex(row, column, 0);
    }

    @Override
    public QtModelIndex parent(QtModelIndex qtModelIndex)
    {
        return new QtModelIndex();
    }

    @Override
    public int rowCount(QtModelIndex parent)
    {
         return m_rows;
    }

    @Override
    public HashMap<Integer, String> roleNames()
    {
        final HashMap<Integer, String> roles = new HashMap<Integer, String>();
        roles.put(0, "integerRole");
        return roles;
    }
}
