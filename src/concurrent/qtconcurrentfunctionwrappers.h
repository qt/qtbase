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

#ifndef QTCONCURRENT_FUNCTIONWRAPPERS_H
#define QTCONCURRENT_FUNCTIONWRAPPERS_H

#include <QtConcurrent/qtconcurrentcompilertest.h>
#include <QtCore/QStringList>

#include <tuple>

#if !defined(QT_NO_CONCURRENT) || defined(Q_CLANG_QDOC)

QT_BEGIN_NAMESPACE

namespace QtPrivate {

struct PushBackWrapper
{
    template <class C, class U>
    inline void operator()(C &c, const U &u) const
    {
        return c.push_back(u);
    }

    template <class C, class U>
    inline void operator()(C &c, U &&u) const
    {
        return c.push_back(u);
    }
};

// -- MapResultType

template <class T, class Enable = void>
struct Argument
{
    using Type = void;
};

template <class Sequence>
struct Argument<Sequence, typename std::enable_if<IsIterableValue<Sequence>>::type>
{
    using Type = std::decay_t<decltype(*std::declval<Sequence>().begin())>;
};

template <class Iterator>
struct Argument<Iterator, typename std::enable_if<IsDereferenceableValue<Iterator>>::type>
{
    using Type = std::decay_t<decltype(*std::declval<Iterator>())>;
};

template <class T>
using ArgumentType = typename Argument<T>::Type;

template <class T, class MapFunctor>
struct MapResult
{
    static_assert(std::is_invocable_v<std::decay_t<MapFunctor>, ArgumentType<T>>,
                  "It's not possible to invoke the function with passed argument.");
    using Type = std::invoke_result_t<std::decay_t<MapFunctor>, ArgumentType<T>>;
};

template <class T, class MapFunctor>
using MapResultType = typename MapResult<T, MapFunctor>::Type;

// -- ReduceResultType

template <class T>
struct ReduceResultType;

template <class U, class V>
struct ReduceResultType<void(*)(U&,V)>
{
    using ResultType = U;
};

template <class T, class C, class U>
struct ReduceResultType<T(C::*)(U)>
{
    using ResultType = C;
};

template <class U, class V>
struct ReduceResultType<std::function<void(U&, V)>>
{
    using ResultType = U;
};

template <typename R, typename ...A>
struct ReduceResultType<R(*)(A...)>
{
    using ResultType = typename std::tuple_element<0, std::tuple<A...>>::type;
};

#if defined(__cpp_noexcept_function_type) && __cpp_noexcept_function_type >= 201510
template <class U, class V>
struct ReduceResultType<void(*)(U&,V) noexcept>
{
    using ResultType = U;
};

template <class T, class C, class U>
struct ReduceResultType<T(C::*)(U) noexcept>
{
    using ResultType = C;
};
#endif

// -- MapSequenceResultType

template <class InputSequence, class MapFunctor>
struct MapSequenceResultType;

template <class MapFunctor>
struct MapSequenceResultType<QStringList, MapFunctor>
{
    typedef QList<QtPrivate::MapResultType<QStringList, MapFunctor>> ResultType;
};

#ifndef QT_NO_TEMPLATE_TEMPLATE_PARAMETERS

template <template <typename...> class InputSequence, typename MapFunctor, typename ...T>
struct MapSequenceResultType<InputSequence<T...>, MapFunctor>
{
    typedef InputSequence<QtPrivate::MapResultType<InputSequence<T...>, MapFunctor>> ResultType;
};

#endif // QT_NO_TEMPLATE_TEMPLATE_PARAMETER

template<typename Sequence>
struct SequenceHolder
{
    SequenceHolder(const Sequence &s) : sequence(s) { }
    SequenceHolder(Sequence &&s) : sequence(std::move(s)) { }
    Sequence sequence;
};

} // namespace QtPrivate.


QT_END_NAMESPACE

#endif // QT_NO_CONCURRENT

#endif
