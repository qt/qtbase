/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qwinregistry_p.h"

#include <QtCore/qvarlengtharray.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

QWinRegistryKey::QWinRegistryKey() :
    m_key(nullptr)
{
}

// Open a key with the specified permissions (KEY_READ/KEY_WRITE).
// "access" is to explicitly use the 32- or 64-bit branch.
QWinRegistryKey::QWinRegistryKey(HKEY parentHandle, QStringView subKey,
                                 REGSAM permissions, REGSAM access)
{
    if (RegOpenKeyEx(parentHandle, reinterpret_cast<const wchar_t *>(subKey.utf16()),
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

QString QWinRegistryKey::stringValue(QStringView subKey) const
{
    QString result;
    if (!isValid())
        return result;
    DWORD type;
    DWORD size;
    auto subKeyC = reinterpret_cast<const wchar_t *>(subKey.utf16());
    if (RegQueryValueEx(m_key, subKeyC, nullptr, &type, nullptr, &size) != ERROR_SUCCESS
        || (type != REG_SZ && type != REG_EXPAND_SZ) || size <= 2) {
        return result;
    }
    // Reserve more for rare cases where trailing '\0' are missing in registry.
    // Rely on 0-termination since strings of size 256 padded with 0 have been
    // observed (QTBUG-84455).
    size += 2;
    QVarLengthArray<unsigned char> buffer(static_cast<int>(size));
    std::fill(buffer.data(), buffer.data() + size, 0u);
    if (RegQueryValueEx(m_key, subKeyC, nullptr, &type, buffer.data(), &size) == ERROR_SUCCESS)
          result = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
    return result;
}

QPair<DWORD, bool> QWinRegistryKey::dwordValue(QStringView subKey) const
{
    if (!isValid())
        return qMakePair(0, false);
    DWORD type;
    auto subKeyC = reinterpret_cast<const wchar_t *>(subKey.utf16());
    if (RegQueryValueEx(m_key, subKeyC, nullptr, &type, nullptr, nullptr) != ERROR_SUCCESS
        || type != REG_DWORD) {
        return qMakePair(0, false);
    }
    DWORD value = 0;
    DWORD size = sizeof(value);
    const bool ok =
        RegQueryValueEx(m_key, subKeyC, nullptr, nullptr,
                        reinterpret_cast<unsigned char *>(&value), &size) == ERROR_SUCCESS;
    return qMakePair(value, ok);
}

QT_END_NAMESPACE
