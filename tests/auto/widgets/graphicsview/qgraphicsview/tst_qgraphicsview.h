/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef TST_QGRAPHICSVIEW_H
#define TST_QGRAPHICSVIEW_H

// This file contains structs used in tst_qgraphicsview::scrollBarRanges.
// Whenever these mention scrollbars or spacing it is about the number of
// scrollbars or spacings to use as these are style dependent so that the real
// value to add/remove has to be obtained in test run using the actual style.

struct ExpectedValueDescription {
    Q_DECL_CONSTEXPR ExpectedValueDescription(int v = 0, int sbeta = 0, int sta = 0)
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
    Q_DECL_CONSTEXPR ScrollBarCount(int l = 0, int t = 0, int r = 0, int b = 0 )
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
