/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
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
#ifndef Q20ITERATOR_H
#define Q20ITERATOR_H

#include <QtCore/qglobal.h>

#include <iterator>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. Types and functions defined
// in this file will behave exactly as their std counterparts. You
// may use these definitions in your own code, but be aware that we
// will remove them once Qt depends on the C++ version that supports
// them in namespace std. There will be NO deprecation warning, the
// definitions will JUST go away.
//
// If you can't agree to these terms, don't use these definitions!
//
// We mean it.
//

QT_BEGIN_NAMESPACE

// like std::ssize
namespace q20 {
#ifdef __cpp_lib_ssize
    using std::ssize;
#else
    template<class C> constexpr auto ssize(const C &c)
      -> std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(c.size())>>
    { return static_cast<std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(c.size())>>>(c.size()); }

    template<class T, std::ptrdiff_t N> constexpr std::ptrdiff_t ssize(const T (&)[N]) noexcept
    { return N; }
#endif
} // namespace q20

QT_END_NAMESPACE

#endif /* Q20ITERATOR_H */
