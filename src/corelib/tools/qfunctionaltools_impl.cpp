// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/qfunctionaltools_impl.h>

// Remove this file once we have tests that implicitly test all aspects of
// CompactStorage

QT_BEGIN_NAMESPACE

namespace QtPrivate {

#define FOR_EACH_CVREF(op) \
    op(&) \
    op(const &) \
    op(&&) \
    op(const &&) \
    /* end */

namespace _testing {
    struct empty {};
    struct final final {};
    static_assert(std::is_same_v<CompactStorage<empty>,
                                 detail::StorageEmptyBaseClassOptimization<empty>>);
    static_assert(std::is_same_v<CompactStorage<final>,
                                 detail::StorageByValue<final>>);
    static_assert(std::is_same_v<CompactStorage<int>,
                                 detail::StorageByValue<int>>);
#define CHECK1(Obj, cvref) \
    static_assert(std::is_same_v<decltype(std::declval<CompactStorage< Obj > cvref>().object()), \
                                 Obj cvref>);
#define CHECK(cvref) \
    CHECK1(empty, cvref) \
    CHECK1(final, cvref) \
    CHECK1(int, cvref) \
    /* end */

    FOR_EACH_CVREF(CHECK)
#undef CHECK
#undef CHECK1
} // namespace _testing

} // namespace QtPrivate

#undef FOR_EACH_CVREF

QT_END_NAMESPACE
