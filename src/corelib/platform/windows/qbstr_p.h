// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBSTR_P_H
#define QBSTR_P_H

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

#include <QtCore/private/qglobal_p.h>

#if defined(Q_OS_WIN) || defined(Q_QDOC)

#include <QtCore/qt_windows.h>
#include <QtCore/qstring.h>
#include <utility>
#include <oaidl.h>

QT_BEGIN_NAMESPACE

class QBStr
{
public:
    QBStr() = default;

    ~QBStr() noexcept
    {
        free();
    }

    // Does not take ownership
    explicit QBStr(LPCOLESTR str) noexcept
    {
        if (str)
            m_str = ::SysAllocString(str);
        Q_ASSERT(m_str || !str);
    }

    explicit QBStr(const QString &str) noexcept
    {
        if (!str.isNull())
            m_str = ::SysAllocString(reinterpret_cast<const wchar_t *>(str.utf16()));
        Q_ASSERT(m_str || str.isNull());
    }

    QBStr(const QBStr &str) noexcept : m_str{ str.copy() } { }

    QBStr(QBStr &&str) noexcept : m_str{ std::exchange(str.m_str, nullptr) } { }

    // Does not take ownership
    QBStr &operator=(LPCOLESTR str) noexcept
    {
        free();
        if (str)
            m_str = ::SysAllocString(str);
        Q_ASSERT(m_str || !str);
        return *this;
    }

    QBStr &operator=(const QString &str) noexcept
    {
        free();
        if (!str.isNull())
            m_str = ::SysAllocString(reinterpret_cast<const wchar_t*>(str.utf16()));
        Q_ASSERT(m_str || str.isNull());
        return *this;
    }

    QBStr &operator=(const QBStr &rhs) noexcept
    {
        if (this != std::addressof(rhs))
            reset(rhs.copy());

        return *this;
    }

    QBStr &operator=(QBStr &&rhs) noexcept
    {
        if (this != std::addressof(rhs))
            reset(std::exchange(rhs.m_str, nullptr));

        return *this;
    }

    const BSTR &bstr() const noexcept
    {
        return m_str;
    }

    QString str() const
    {
        return QString::fromWCharArray(m_str);
    }

    [[nodiscard]] BSTR release() noexcept
    {
        return std::exchange(m_str, nullptr);
    }

    [[nodiscard]] BSTR *operator&() noexcept // NOLINT(google-runtime-operator)
    {
        Q_ASSERT(!m_str);
        return &m_str;
    }

private:
    void free() noexcept
    {
        if (m_str)
            ::SysFreeString(m_str);
        m_str = nullptr;
    }

    void reset(BSTR str) noexcept
    {
        free();
        m_str = str;
    }

    [[nodiscard]] BSTR copy() const noexcept
    {
        if (!m_str)
            return nullptr;

        return ::SysAllocStringByteLen(reinterpret_cast<const char *>(m_str),
                                       ::SysStringByteLen(m_str));
    }

    BSTR m_str = nullptr;
};

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCOMPTR_P_H
