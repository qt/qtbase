/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtConcurrent module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTCONCURRENT_MEDIAN_H
#define QTCONCURRENT_MEDIAN_H

#include <QtConcurrent/qtconcurrent_global.h>

#if !defined(QT_NO_CONCURRENT) ||defined(Q_CLANG_QDOC)

#include <QtCore/qvector.h>

#include <algorithm>

QT_BEGIN_NAMESPACE



namespace QtConcurrent {

template <typename T>
class Median
{
public:
    Median(int _bufferSize)
        : currentMedian(), bufferSize(_bufferSize), currentIndex(0), valid(false), dirty(true)
    {
        values.resize(bufferSize);
    }

    void reset()
    {
        values.fill(0);
        currentIndex = 0;
        valid = false;
        dirty = true;
    }

    void addValue(T value)
    {
        currentIndex = ((currentIndex + 1) % bufferSize);
        if (valid == false && currentIndex % bufferSize == 0)
            valid = true;

        // Only update the cached median value when we have to, that
        // is when the new value is on then other side of the median
        // compared to the current value at the index.
        const T currentIndexValue = values[currentIndex];
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

    T median()
    {
        if (dirty) {
            dirty = false;

// This is a workaround for http://gcc.gnu.org/bugzilla/show_bug.cgi?id=58800
// Avoid using std::nth_element for the affected stdlibc++ releases 4.7.3 and 4.8.2.
// Note that the official __GLIBCXX__ value of the releases is not used since that
// one might be patched on some GNU/Linux distributions.
#if defined(__GLIBCXX__) && __GLIBCXX__ <= 20140107
            QVector<T> sorted = values;
            std::sort(sorted.begin(), sorted.end());
            currentMedian = sorted.at(bufferSize / 2);
#else
            QVector<T> copy = values;
            typename QVector<T>::iterator begin = copy.begin(), mid = copy.begin() + bufferSize/2, end = copy.end();
            std::nth_element(begin, mid, end);
            currentMedian = *mid;
#endif
        }
        return currentMedian;
    }
private:
    QVector<T> values;
    T currentMedian;
    int bufferSize;
    int currentIndex;
    bool valid;
    bool dirty;
};

// ### Qt6: Drop Median<double> in favor of this faster MedianDouble
class MedianDouble
{
public:
    enum { BufferSize = 7 };

    MedianDouble()
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
