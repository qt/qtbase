// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Found relative to the includer (index 0 = "first"), whose position it
// inherits, so this #include_next must resume after "first": reach
// second/marker.h directly, skipping first/marker.h, and not warn.
#include_next <marker.h>
