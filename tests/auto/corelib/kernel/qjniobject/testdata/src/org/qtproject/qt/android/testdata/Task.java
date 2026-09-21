// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

package org.qtproject.qt.android.testdatapackage;

// Pairs with Tasks. Both build the same cache key, and neither method
// touches instance state, so a wrong ID gives a wrong value not a crash.
public class Task
{
    public int send(int value) { return value + 10; }
}
