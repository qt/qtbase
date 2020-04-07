/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTCONCURRENT_MEDIAN_H
#define QTCONCURRENT_MEDIAN_H

#include <QtConcurrent/qtconcurrent_global.h>

#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

#include <algorithm>
#include <cstring>

QT_BEGIN_NAMESPACE

namespace QtConcurrent {

class Median
{
public:
    enum { BufferSize = 7 };

    Median()
        : currentMedian(), currentIndex(0), valid(false), dirty(true)
    {
        std::fill_n(values, static_cast<int>(BufferSize), 0.0);
    }

    void reset()
    {
        std::fill_n(values, static_cast<int>(BufferSize), 0.0);
        currentIndex = 0;
        valid = false;
        dirty = true;
    }

    void addValue(double value)
    {
        ++currentIndex;
        if (currentIndex == BufferSize) {
            currentIndex = 0;
            valid = true;
        }

        // Only update the cached median value when we have to, that
        // is when the new value is on then other side of the median
        // compared to the current value at the index.
        const double currentIndexValue = values[currentIndex];
        if ((currentIndexValue > currentMedian && currentMedian > value)
            || (currentMedian > currentIndexValue && value > currentMedian)) {
            dirty = true;
        }

        values[currentIndex] = value;
    }

    bool isMedianValid() const
    {
        return valid;
    }

    double median()
    {
        if (dirty) {
            dirty = false;

            double sorted[BufferSize];
            ::memcpy(&sorted, &values, sizeof(sorted));
            std::sort(sorted, sorted + static_cast<int>(BufferSize));
            currentMedian = sorted[BufferSize / 2];
        }

        return currentMedian;
    }

private:
    double values[BufferSize];
    double currentMedian;
    int currentIndex;
    bool valid;
    bool dirty;
};

} // namespace QtConcurrent

QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
