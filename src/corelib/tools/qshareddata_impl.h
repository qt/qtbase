/****************************************************************************
**
** Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
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

#ifndef QSHAREDDATA_IMPL_H
#define QSHAREDDATA_IMPL_H

#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

template <typename T>
class QExplicitlySharedDataPointerV2
{
    T *d;

public:
    constexpr QExplicitlySharedDataPointerV2() noexcept : d(nullptr) {}

    explicit QExplicitlySharedDataPointerV2(T *t) noexcept
        : d(t)
    {
        if (d)
            d->ref.ref();
    }

    QExplicitlySharedDataPointerV2(T *t, QAdoptSharedDataTag) noexcept
        : d(t)
    {
    }

    QExplicitlySharedDataPointerV2(const QExplicitlySharedDataPointerV2 &other) noexcept
        : d(other.d)
    {
        if (d)
            d->ref.ref();
    }

    QExplicitlySharedDataPointerV2 &operator=(const QExplicitlySharedDataPointerV2 &other) noexcept
    {
        QExplicitlySharedDataPointerV2 copy(other);
        swap(copy);
        return *this;
    }

    QExplicitlySharedDataPointerV2(QExplicitlySharedDataPointerV2 &&other) noexcept
        : d(qExchange(other.d, nullptr))
    {
    }

    QExplicitlySharedDataPointerV2 &operator=(QExplicitlySharedDataPointerV2 &&other) noexcept
    {
        QExplicitlySharedDataPointerV2 moved(std::move(other));
        swap(moved);
        return *this;
    }

    ~QExplicitlySharedDataPointerV2()
    {
        if (d && !d->ref.deref())
            delete d;
    }

    void detach()
    {
        if (!d) {
            // should this codepath be here on in all user's detach()?
            d = new T;
            d->ref.ref();
        } else if (d->ref.loadRelaxed() != 1) {
            // TODO: qAtomicDetach here...?
            QExplicitlySharedDataPointerV2 copy(new T(*d));
            swap(copy);
        }
    }

    void reset(T *t = nullptr) noexcept
    {
        if (d && !d->ref.deref())
            delete d;
        d = t;
        if (d)
            d->ref.ref();
    }

    constexpr T *take() noexcept
    {
        return qExchange(d, nullptr);
    }

    bool isShared() const noexcept
    {
        return d && d->ref.loadRelaxed() != 1;
    }

    constexpr void swap(QExplicitlySharedDataPointerV2 &other) noexcept
    {
        qSwap(d, other.d);
    }

    // important change from QExplicitlySharedDataPointer: deep const
    constexpr T &operator*() { return *d; }
    constexpr T *operator->() { return d; }
    constexpr const T &operator*() const { return *d; }
    constexpr const T *operator->() const { return d; }

    constexpr T *data() noexcept { return d; }
    constexpr const T *data() const noexcept { return d; }

    constexpr explicit operator bool() const noexcept { return d; }

    constexpr friend bool operator==(const QExplicitlySharedDataPointerV2 &lhs,
                                     const QExplicitlySharedDataPointerV2 &rhs) noexcept
    {
        return lhs.d == rhs.d;
    }

    constexpr friend bool operator!=(const QExplicitlySharedDataPointerV2 &lhs,
                                     const QExplicitlySharedDataPointerV2 &rhs) noexcept
    {
        return lhs.d != rhs.d;
    }
};

template <typename T>
constexpr void swap(QExplicitlySharedDataPointerV2<T> &lhs, QExplicitlySharedDataPointerV2<T> &rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace QtPrivate

QT_END_NAMESPACE

#endif // QSHAREDDATA_IMPL_H
