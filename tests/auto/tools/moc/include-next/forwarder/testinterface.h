// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Bare-include forwarder in a plain -I directory, mirroring Qt's framework
// build layout (include/QtFoo/qfoo.h -> #include_next <QtFoo/qfoo.h>). The
// #include_next must continue past this directory into the -F framework path.
#include_next <Test/testinterface.h>
