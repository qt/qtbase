// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:trivial-impl-only

#ifndef QWINREGISTRY_H
#define QWINREGISTRY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qt_windows.h>
#include <QtCore/qvariant.h>
#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/private/quniquehandle_types_p.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QWinRegistryKey : public QObject
{
    Q_DISABLE_COPY(QWinRegistryKey)
    Q_OBJECT

public:
    QWinRegistryKey(QObject *parent = nullptr);
    explicit QWinRegistryKey(HKEY parentHandle, const wchar_t *subKey,
                             REGSAM permissions = KEY_READ, REGSAM access = 0,
                             QObject *parent = nullptr);
    explicit QWinRegistryKey(HKEY parentHandle, const QString &subKey,
                             REGSAM permissions = KEY_READ, REGSAM access = 0,
                             QObject *parent = nullptr);
    ~QWinRegistryKey();

    QWinRegistryKey(QWinRegistryKey &&other) noexcept
#if 1 // QTBUG-140725
        = delete;
    void operator=(QWinRegistryKey &&) = delete;
#else
        : m_key(std::exchange(other.m_key, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QWinRegistryKey)
    void swap(QWinRegistryKey &other) noexcept
    {
        qt_ptr_swap(m_key, other.m_key);
    }
#endif

    [[nodiscard]] bool isValid() const { return m_key != nullptr; }

    [[nodiscard]] HKEY handle() const { return m_key; }

    operator HKEY() const { return handle(); }

    void close();

    QString name() const;

    [[nodiscard]] QVariant value(const wchar_t *subKey) const;
    [[nodiscard]] QVariant value(const QString &subKey) const;

    template<typename T>
    [[nodiscard]] std::optional<T> value(const wchar_t *subKey) const
    {
        return to_optional<T>(value(subKey));
    }

    template<typename T>
    [[nodiscard]] std::optional<T> value(const QString &subKey) const
    {
        return to_optional<T>(value(subKey));
    }

    QString stringValue(const wchar_t *subKey) const;
    QString stringValue(const QString &subKey) const;

#ifndef QT_NO_DEBUG_STREAM
    friend Q_CORE_EXPORT QDebug operator<<(QDebug dbg, const QWinRegistryKey &);
#endif

Q_SIGNALS:
    void valueChanged();

protected:
    void connectNotify(const QMetaMethod &signal) override;

private:
    template<typename T>
    static std::optional<T> to_optional(QVariant v)
    {
        if (v.isValid())
            return qvariant_cast<T>(std::move(v));
        return std::nullopt;
    }

    HKEY m_key = nullptr;
    QUniqueWin32NullHandle m_keyChangedEvent;
};

QT_END_NAMESPACE

#endif // QWINREGISTRY_H
