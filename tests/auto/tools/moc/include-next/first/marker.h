// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Forwarder found first on the include path. It must not terminate the search:
// #include_next has to continue into the "second" directory.
int included_marker_from_first;
#include_next <marker.h>
