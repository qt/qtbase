// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

package org.qtproject.qt.android;

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

class QtAndroidItemModelProxy extends QtAbstractItemModel
{
    @Override public int columnCount(QtModelIndex parent) { return jni_columnCount(parent); }

    @Override public Object data(QtModelIndex index, int role) { return jni_data(index, role); }
    @Override public QtModelIndex index(int row, int column, QtModelIndex parent)
    {
        return (QtModelIndex)jni_index(row, column, parent);
    }
    @Override public QtModelIndex parent(QtModelIndex index)
    {
        return (QtModelIndex)jni_parent(index);
    }
    @Override public int rowCount(QtModelIndex parent) { return jni_rowCount(parent); }

    private native int jni_columnCount(QtModelIndex parent);
    private native Object jni_data(QtModelIndex index, int role);
    private native Object jni_index(int row, int column, QtModelIndex parent);
    private native Object jni_parent(QtModelIndex index);
    private native int jni_rowCount(QtModelIndex parent);
}
