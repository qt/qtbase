// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Resolves <marker.h> to first/marker.h, whose #include_next must continue into
// second/marker.h. Run through "moc -E" both markers should appear.
#include <marker.h>
