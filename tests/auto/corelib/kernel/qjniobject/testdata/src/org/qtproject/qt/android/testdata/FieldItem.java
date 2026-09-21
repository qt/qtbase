// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.testdatapackage;

// The instance id hides the static one from FieldBase, so both resolve
// through this class and build the same cache key.
public class FieldItem extends FieldBase
{
    public int id = 200;
}
