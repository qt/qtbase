// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/q23utility.h>

QT_BEGIN_NAMESPACE

#define CHECK2(cvref_in, cvref_out) \
    static_assert(std::is_same_v< \
            decltype(q23::forward_like<int cvref_in >(std::declval<long&>())), \
            long cvref_out \
        >, "oops: cvref '" #cvref_in "' doesn't work") \
    /* end */
#define CHECK(cvref) CHECK2(cvref, cvref)
CHECK2(/**/, &&);
CHECK(&);
CHECK(&&);
CHECK2(const, const &&);
CHECK(const &);
CHECK(const &&);
// volatile is not supported
#undef CHECK
#undef CHECK2

QT_END_NAMESPACE
