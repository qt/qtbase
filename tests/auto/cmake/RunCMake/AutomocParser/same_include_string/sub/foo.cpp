// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Intentionally uses the bare "moc_foo.cpp" (same as the root source) rather
// than "same_include_string/sub/moc_foo.cpp".
// AUTOMOC rejects the ambiguous include.
#include "foo.h"

#include "moc_foo.cpp"
