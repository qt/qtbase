// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//! [0]
static constexpr auto matcher = qMakeStaticCaseSensitiveLatin1StringViewMatcher("needle");
//! [0]
//! [1]
static constexpr auto matcher = qMakeStaticCaseInsensitiveLatin1StringViewMatcher("needle");
//! [1]
