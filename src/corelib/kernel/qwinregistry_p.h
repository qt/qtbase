// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QtCore/qpair.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringview.h>
#include <QtCore/qt_windows.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QWinRegistryKey
{
public:
    Q_DISABLE_COPY(QWinRegistryKey)

    QWinRegistryKey();
    explicit QWinRegistryKey(HKEY parentHandle, QStringView subKey,
                             REGSAM permissions = KEY_READ, REGSAM access = 0);
    ~QWinRegistryKey();

    QWinRegistryKey(QWinRegistryKey &&other) noexcept
        : m_key(qExchange(other.m_key, nullptr)) {}
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QWinRegistryKey)
    void swap(QWinRegistryKey &other) noexcept { qSwap(m_key, other.m_key); }

    bool isValid() const { return m_key != nullptr; }
    operator HKEY() const { return m_key; }
    void close();

    QString stringValue(QStringView subKey) const;
    QPair<DWORD, bool> dwordValue(QStringView subKey) const;

private:
    HKEY m_key;
};

QT_END_NAMESPACE

#endif // QWINREGISTRY_H
