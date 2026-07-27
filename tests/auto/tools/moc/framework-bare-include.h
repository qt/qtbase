// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FRAMEWORK_BARE_INCLUDE_H
#define FRAMEWORK_BARE_INCLUDE_H

#if __has_include(<Test/testinterface.h>)
prefixed_framework_include_resolves
#endif

#if __has_include(<Test>)
bare_framework_include_resolves
#endif

#endif // FRAMEWORK_BARE_INCLUDE_H
