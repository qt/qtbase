// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
MyRecord record(int row) const
{
    Q_ASSERT(row >= 0 && row < count());

    while (row > cache.lastIndex())
        cache.append(slowFetchRecord(cache.lastIndex()+1));
    while (row < cache.firstIndex())
        cache.prepend(slowFetchRecord(cache.firstIndex()-1));

    return cache.at(row);
}
//! [0]

//! [1]
QContiguousCache<int> cache(10);
cache.insert(INT_MAX, 1); // cache contains one value and has valid indexes, INT_MAX to INT_MAX
cache.append(2); // cache contains two values but does not have valid indexes.
cache.normalizeIndexes(); // cache has two values, 1 and 2.  New first index will be in the range of 0 to capacity().
//! [1]
