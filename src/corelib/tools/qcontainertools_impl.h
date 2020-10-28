/****************************************************************************
**
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#if 0
#pragma qt_sync_skip_header_check
#pragma qt_sync_stop_processing
#endif

#ifndef QCONTAINERTOOLS_IMPL_H
#define QCONTAINERTOOLS_IMPL_H

#include <QtCore/qglobal.h>
#include <QtCore/qtypeinfo.h>

#include <cstring>
#include <iterator>
#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPrivate
{

template <typename T, typename N>
void q_uninitialized_relocate_n(T* first, N n, T* out)
{
    if constexpr (QTypeInfo<T>::isRelocatable) {
        if (n != N(0)) { // even if N == 0, out == nullptr or first == nullptr are UB for memmove()
            std::memmove(static_cast<void*>(out),
                         static_cast<const void*>(first),
                         n * sizeof(T));
        }
    } else {
        std::uninitialized_move_n(first, n, out);
        if constexpr (QTypeInfo<T>::isComplex)
            std::destroy_n(first, n);
    }
}


template <typename Iterator>
using IfIsInputIterator = typename std::enable_if<
    std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::input_iterator_tag>::value,
    bool>::type;

template <typename Iterator>
using IfIsForwardIterator = typename std::enable_if<
    std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>::value,
    bool>::type;

template <typename Iterator>
using IfIsNotForwardIterator = typename std::enable_if<
    !std::is_convertible<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>::value,
    bool>::type;

template <typename Container,
          typename InputIterator,
          IfIsNotForwardIterator<InputIterator> = true>
void reserveIfForwardIterator(Container *, InputIterator, InputIterator)
{
}

template <typename Container,
          typename ForwardIterator,
          IfIsForwardIterator<ForwardIterator> = true>
void reserveIfForwardIterator(Container *c, ForwardIterator f, ForwardIterator l)
{
    c->reserve(static_cast<typename Container::size_type>(std::distance(f, l)));
}

template <typename Iterator, typename = std::void_t<>>
struct AssociativeIteratorHasKeyAndValue : std::false_type
{
};

template <typename Iterator>
struct AssociativeIteratorHasKeyAndValue<
        Iterator,
        std::void_t<decltype(std::declval<Iterator &>().key()),
                    decltype(std::declval<Iterator &>().value())>
    >
    : std::true_type
{
};

template <typename Iterator, typename = std::void_t<>, typename = std::void_t<>>
struct AssociativeIteratorHasFirstAndSecond : std::false_type
{
};

template <typename Iterator>
struct AssociativeIteratorHasFirstAndSecond<
        Iterator,
        std::void_t<decltype(std::declval<Iterator &>()->first),
                    decltype(std::declval<Iterator &>()->second)>
    >
    : std::true_type
{
};

template <typename Iterator>
using IfAssociativeIteratorHasKeyAndValue =
    typename std::enable_if<AssociativeIteratorHasKeyAndValue<Iterator>::value, bool>::type;

template <typename Iterator>
using IfAssociativeIteratorHasFirstAndSecond =
    typename std::enable_if<AssociativeIteratorHasFirstAndSecond<Iterator>::value, bool>::type;

template <typename T, typename U>
using IfIsNotSame =
    typename std::enable_if<!std::is_same<T, U>::value, bool>::type;

template<typename T, typename U>
using IfIsNotConvertible = typename std::enable_if<!std::is_convertible<T, U>::value, bool>::type;
} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QCONTAINERTOOLS_IMPL_H
