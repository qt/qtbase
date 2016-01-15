/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef ARRAY_H
#define ARRAY_H

#include <QtCore/QtGlobal>
#include <algorithm>

QT_BEGIN_NAMESPACE

/* A simple, non-shared array. */

template <class T>
class Array
{
    Q_DISABLE_COPY(Array)
public:
    enum { initialSize = 5 };

    typedef T* const_iterator;

    explicit Array(size_t size= 0) : data(0), m_capacity(0), m_size(0)
        { if (size) resize(size); }
    ~Array() { delete [] data; }

    T *data;
    inline size_t size() const          { return m_size; }
    inline const_iterator begin() const { return data; }
    inline const_iterator end() const   { return data + m_size; }

    inline void append(const T &value)
    {
        const size_t oldSize = m_size;
        resize(m_size + 1);
        data[oldSize] = value;
    }

    inline void resize(size_t size)
    {
        if (size > m_size)
            reserve(size > 1 ? size + size / 2 : size_t(initialSize));
        m_size = size;
    }

    void reserve(size_t capacity)
    {
        if (capacity > m_capacity) {
            const T *oldData = data;
            data = new T[capacity];
            if (oldData) {
                std::copy(oldData, oldData + m_size, data);
                delete [] oldData;
            }
            m_capacity = capacity;
        }
    }

private:
    size_t m_capacity;
    size_t m_size;
};

QT_END_NAMESPACE

#endif // ARRAY_H
