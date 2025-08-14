// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QtGlobal>

int core_helper_func();

// Exported function needed to create .lib on Windows.
Q_DECL_EXPORT int gui_helper_func() { return core_helper_func() + 1; }
