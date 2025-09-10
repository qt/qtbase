// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOMVARIANT_P_H
#define QCOMVARIANT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#if defined(Q_OS_WIN) || defined(Q_QDOC)

// clang-format off
#include <QtCore/private/qglobal_p.h>
#include <QtCore/qt_windows.h>
#include <QtCore/private/qbstr_p.h>
#include <QtCore/private/qcomptr_p.h>
// clang-format on

QT_BEGIN_NAMESPACE

struct QComVariant
{
    // clang-format off
    QComVariant() noexcept
    {
        VariantInit(&m_variant);
    }

    ~QComVariant() noexcept
    {
        clear();
    }

    explicit QComVariant(bool value) noexcept
    {
        m_variant.vt = VT_BOOL;
        m_variant.boolVal = value ? VARIANT_TRUE : VARIANT_FALSE;
    }

    explicit QComVariant(int value) noexcept
    {
        m_variant.vt = VT_INT;
        m_variant.intVal = value;
    }

    explicit QComVariant(long value) noexcept
    {
        m_variant.vt = VT_I4;
        m_variant.lVal = value;
    }

    explicit QComVariant(double value) noexcept
    {
        m_variant.vt = VT_R8;
        m_variant.dblVal = value;
    }

    template <typename T>
    QComVariant(const ComPtr<T> &value) noexcept
    {
        static_assert(std::is_base_of_v<IUnknown, T>, "Invalid COM interface");
        ComPtr<IUnknown> unknown = value;
        m_variant.vt = VT_UNKNOWN;
        m_variant.punkVal = unknown.Detach(); // Transfer ownership
    }

    QComVariant(QBStr &&value) noexcept
    {
        m_variant.vt = VT_BSTR;
        m_variant.bstrVal = value.release(); // Transfer ownership
    }

    QComVariant(const QString &str)
    {
        m_variant.vt = VT_BSTR;
        m_variant.bstrVal = QBStr{ str }.release(); // Transfer ownership of copy
    }

    const VARIANT &get() const noexcept
    {
        return m_variant;
    }

    VARIANT &get() noexcept
    {
        return m_variant;
    }
    // clang-format on

    [[nodiscard]] VARIANT *operator&() noexcept // NOLINT(google-runtime-operator)
    {
        clear();
        return &m_variant;
    }

    VARIANT release() noexcept
    {
        const VARIANT detached{ m_variant };
        VariantInit(&m_variant);
        return detached;
    }

    QComVariant(const QComVariant &) = delete;
    QComVariant &operator=(const QComVariant &) = delete;
    QComVariant(QComVariant && other) = delete;
    QComVariant &operator=(QComVariant &&) = delete;

private:

    void clear()
    {
        const HRESULT hr = VariantClear(&m_variant);
        Q_ASSERT(hr == S_OK);
        Q_UNUSED(hr)

        VariantInit(&m_variant); // Clear value field
    }

    VARIANT m_variant{};
};

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCOMVARIANT_P_H
