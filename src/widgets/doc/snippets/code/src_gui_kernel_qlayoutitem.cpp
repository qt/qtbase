// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [0]
int MyLayout::heightForWidth(int w) const
{
    if (cache_dirty || cached_width != w) {
        MyLayout *that = const_cast<MyLayout *>(this);
        int h = calculateHeightForWidth(w);
        that->cached_hfw = h;
        return h;
    }
    return cached_hfw;
}
//! [0]
