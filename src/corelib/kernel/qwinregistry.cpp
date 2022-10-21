// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwinregistry_p.h"
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qendian.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

QWinRegistryKey::QWinRegistryKey()
{
}

// Open a key with the specified permissions (KEY_READ/KEY_WRITE).
// "access" is to explicitly use the 32- or 64-bit branch.
QWinRegistryKey::QWinRegistryKey(HKEY parentHandle, QStringView subKey,
                                 REGSAM permissions, REGSAM access)
{
    if (RegOpenKeyExW(parentHandle, reinterpret_cast<const wchar_t *>(subKey.utf16()),
                      0, permissions | access, &m_key) != ERROR_SUCCESS) {
        m_key = nullptr;
    }
}

QWinRegistryKey::~QWinRegistryKey()
{
    close();
}

void QWinRegistryKey::close()
{
    if (isValid()) {
        RegCloseKey(m_key);
        m_key = nullptr;
    }
}

QVariant QWinRegistryKey::value(QStringView subKey) const
{
    // NOTE: Empty value name is allowed in Windows registry, it means the default
    // or unnamed value of a key, you can read/write/delete such value normally.

    if (!isValid())
        return {};

    // Use nullptr when we need to access the default value.
    const auto subKeyC = subKey.isEmpty() ? nullptr : reinterpret_cast<const wchar_t *>(subKey.utf16());

    // Get the size and type of the value.
    DWORD dataType = REG_NONE;
    DWORD dataSize = 0;
    LONG ret = RegQueryValueExW(m_key, subKeyC, nullptr, &dataType, nullptr, &dataSize);
    if (ret != ERROR_SUCCESS)
        return {};

    // Workaround for rare cases where the trailing '\0' is missing.
    if (dataType == REG_SZ || dataType == REG_EXPAND_SZ)
        dataSize += 2;
    else if (dataType == REG_MULTI_SZ)
        dataSize += 4;

    // Get the value.
    QVarLengthArray<unsigned char> data(dataSize);
    std::fill(data.data(), data.data() + dataSize, 0u);

    ret = RegQueryValueExW(m_key, subKeyC, nullptr, nullptr, data.data(), &dataSize);
    if (ret != ERROR_SUCCESS)
        return {};

    switch (dataType) {
        case REG_SZ:
        case REG_EXPAND_SZ: {
            if (dataSize > 0) {
                return QString::fromWCharArray(
                    reinterpret_cast<const wchar_t *>(data.constData()));
            }
            return QString();
        }

        case REG_MULTI_SZ: {
            if (dataSize > 0) {
                QStringList list = {};
                int i = 0;
                while (true) {
                    const QString str = QString::fromWCharArray(
                        reinterpret_cast<const wchar_t *>(data.constData()) + i);
                    i += str.length() + 1;
                    if (str.isEmpty())
                        break;
                    list.append(str);
                }
                return list;
            }
            return QStringList();
        }

        case REG_NONE: // No specific type, treat as binary data.
        case REG_BINARY: {
            if (dataSize > 0) {
                return QString::fromWCharArray(
                    reinterpret_cast<const wchar_t *>(data.constData()), data.size() / 2);
            }
            return QString();
        }

        case REG_DWORD: // Same as REG_DWORD_LITTLE_ENDIAN
            return qFromLittleEndian<quint32>(data.constData());

        case REG_DWORD_BIG_ENDIAN:
            return qFromBigEndian<quint32>(data.constData());

        case REG_QWORD: // Same as REG_QWORD_LITTLE_ENDIAN
            return qFromLittleEndian<quint64>(data.constData());

        default:
            break;
    }

    return {};
}

QString QWinRegistryKey::stringValue(QStringView subKey) const
{
    return value<QString>(subKey).value_or(QString());
}

QPair<DWORD, bool> QWinRegistryKey::dwordValue(QStringView subKey) const
{
    const std::optional<DWORD> val = value<DWORD>(subKey);
    return qMakePair(val.value_or(0), val.has_value());
}

QT_END_NAMESPACE
