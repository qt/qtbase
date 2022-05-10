// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef TST_QGRAPHICSVIEW_H
#define TST_QGRAPHICSVIEW_H

// This file contains structs used in tst_qgraphicsview::scrollBarRanges.
// Whenever these mention scrollbars or spacing it is about the number of
// scrollbars or spacings to use as these are style dependent so that the real
// value to add/remove has to be obtained in test run using the actual style.

struct ExpectedValueDescription {
    constexpr ExpectedValueDescription(int v = 0, int sbeta = 0, int sta = 0)
        : value(v)
        , scrollBarExtentsToAdd(sbeta)
        , spacingsToAdd(sta)
    {
    }

    int value;
    // Describes how often the scrollbar widht/height has to be added to or
    // removed from the value.
    int scrollBarExtentsToAdd;

    // Describes how often the scrollbar spacing has to be added to or removed
    // from the value if the used style has SH_ScrollView_FrameOnlyAroundContents
    // set
    int spacingsToAdd;
};

// Describes how often the scroll bar width/height has to be added to/removed
// from the according side of the sceneRect.
struct ScrollBarCount {
    constexpr ScrollBarCount(int l = 0, int t = 0, int r = 0, int b = 0 )
        : left(l)
        , top(t)
        , right(r)
        , bottom(b)
    {
    }

    int left;
    int top;
    int right;
    int bottom;
};

#endif // TST_QGRAPHICSVIEW_H
