/****************************************************************************
**
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

#ifndef QCONTAINERINFO_H
#define QCONTAINERINFO_H

#include <QtCore/qglobal.h>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace QContainerTraits
{

template<typename C>
using value_type = typename C::value_type;

template<typename C>
using iterator = typename C::iterator;

template<typename C>
using const_iterator = typename C::const_iterator;

// Some versions of Apple clang warn about the constexpr variables below being unused.
QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wunused-const-variable")

template<typename C, typename = void>
constexpr bool has_size_v = false;
template<typename C>
constexpr bool has_size_v<C, std::void_t<decltype(C().size())>> = true;

template<typename C, typename = void>
constexpr bool has_clear_v = false;
template<typename C>
constexpr bool has_clear_v<C, std::void_t<decltype(C().clear())>> = true;

template<typename, typename = void>
constexpr bool has_at_v = false;
template<typename C>
constexpr bool has_at_v<C, std::void_t<decltype(C().at(0))>> = true;

template<typename, typename = void>
constexpr bool can_get_at_index_v = false;
template<typename C>
constexpr bool can_get_at_index_v<C, std::void_t<value_type<C>(decltype(C()[0]))>> = true;

template<typename, typename = void>
constexpr bool can_set_at_index_v = false;
template<typename C>
constexpr bool can_set_at_index_v<C, std::void_t<decltype(C()[0] = value_type<C>())>> = true;

template<typename, typename = void>
constexpr bool has_push_front_v = false;
template<typename C>
constexpr bool has_push_front_v<C, std::void_t<decltype(C().push_front(value_type<C>()))>> = true;

template<typename, typename = void>
constexpr bool has_push_back_v = false;
template<typename C>
constexpr bool has_push_back_v<C, std::void_t<decltype(C().push_back(value_type<C>()))>> = true;

template<typename, typename = void>
constexpr bool has_insert_v = false;
template<typename C>
constexpr bool has_insert_v<C, std::void_t<decltype(C().insert(value_type<C>()))>> = true;

template<typename, typename = void>
constexpr bool has_pop_front_v = false;
template<typename C>
constexpr bool has_pop_front_v<C, std::void_t<decltype(C().pop_front())>> = true;

template<typename, typename = void>
constexpr bool has_pop_back_v = false;
template<typename C>
constexpr bool has_pop_back_v<C, std::void_t<decltype(C().pop_back())>> = true;

template<typename, typename = void>
constexpr bool has_iterator_v = false;
template<typename C>
constexpr bool has_iterator_v<C, std::void_t<iterator<C>>> = true;

template<typename, typename = void>
constexpr bool has_const_iterator_v = false;
template<typename C>
constexpr bool has_const_iterator_v<C, std::void_t<const_iterator<C>>> = true;

template<typename, typename = void>
constexpr bool can_get_at_iterator_v = false;
template<typename C>
constexpr bool can_get_at_iterator_v<C, std::void_t<value_type<C>(decltype(*C().begin()))>> = true;

template<typename, typename = void>
constexpr bool can_set_at_iterator_v = false;
template<typename C>
constexpr bool can_set_at_iterator_v<C, std::void_t<decltype(*C().begin() = value_type<C>())>> = true;

template<typename, typename = void>
constexpr bool can_insert_at_iterator_v = false;
template<typename C>
constexpr bool can_insert_at_iterator_v<C, std::void_t<decltype(C().insert(C().begin(), value_type<C>()))>> = true;

template<typename, typename = void>
constexpr bool can_erase_at_iterator_v = false;
template<typename C>
constexpr bool can_erase_at_iterator_v<C, std::void_t<decltype(C().erase(C().begin()))>> = true;

QT_WARNING_POP

}

QT_END_NAMESPACE

#endif // QCONTAINERINFO_H
