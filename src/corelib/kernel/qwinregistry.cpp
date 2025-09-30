// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#define UMDF_USING_NTSTATUS // Avoid ntstatus redefinitions

#include "qwinregistry_p.h"
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qdebug.h>
#include <QtCore/qendian.h>
#include <QtCore/qlist.h>
#include <QtCore/qwineventnotifier.h>
#include <QtCore/private/qsystemerror_p.h>

#include <ntstatus.h>

// User mode version of ZwQueryKey, as per:
// https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-zwquerykey
extern "C" NTSTATUS NTSYSCALLAPI NTAPI NtQueryKey(HANDLE KeyHandle, int KeyInformationClass,
    PVOID KeyInformation, ULONG Length, PULONG ResultLength);

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static const wchar_t *nullTerminate(const QString &s)
{
    // ### port to QString::nullTerminate() if this stays around for longer
    return reinterpret_cast<const wchar_t*>(s.utf16());
}

QWinRegistryKey::QWinRegistryKey(QObject *parent)
    : QObject(parent)
{
}

// Open a key with the specified permissions (KEY_READ/KEY_WRITE).
// "access" is to explicitly use the 32- or 64-bit branch.
QWinRegistryKey::QWinRegistryKey(HKEY parentHandle, const wchar_t *subKey,
                                 REGSAM permissions, REGSAM access,
                                 QObject *parent)
    : QObject(parent)
{
    if (RegOpenKeyExW(parentHandle, subKey,
                      0, permissions | access, &m_key) != ERROR_SUCCESS) {
        m_key = nullptr;
    }
}

QWinRegistryKey::QWinRegistryKey(HKEY parentHandle, const QString &subKey,
                                 REGSAM permissions, REGSAM access, QObject *parent)
    : QWinRegistryKey(parentHandle, nullTerminate(subKey), permissions, access, parent)
{
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

QString QWinRegistryKey::name() const
{
    if (!isValid())
        return {};

    // https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/ns-ntddk-_key_name_information
    constexpr int kKeyNameInformation = 3;
    struct KeyNameInformation { ULONG length = 0; WCHAR name[1] = {}; };

    // Resolve name of key iteratively by first computing the needed size,
    // and then querying the name, accounting for the possibility that the
    // name length changes behind our back a few times.
    DWORD keyInformationSize = 0;
    for (int i = 0; i < 5; ++i) {
        QByteArray buffer(keyInformationSize, 0u);
        auto *keyNameInformation = reinterpret_cast<KeyNameInformation *>(buffer.data());
        switch (NtQueryKey(m_key, kKeyNameInformation, keyNameInformation,
            keyInformationSize, &keyInformationSize)) {
        case STATUS_BUFFER_TOO_SMALL:
        case STATUS_BUFFER_OVERFLOW:
            continue;
        case STATUS_SUCCESS: {
            return QString::fromWCharArray(keyNameInformation->name,
                keyNameInformation->length / sizeof(wchar_t));
        }
        default:
            return {};
        }
    }

    return {};
}

QVariant QWinRegistryKey::value(const wchar_t *subKey) const
{
    // NOTE: Empty value name is allowed in Windows registry, it means the default
    // or unnamed value of a key, you can read/write/delete such value normally.

    if (!isValid())
        return {};

    // Get the size and type of the value.
    DWORD dataType = REG_NONE;
    DWORD dataSize = 0;
    LONG ret = RegQueryValueExW(m_key, subKey, nullptr, &dataType, nullptr, &dataSize);
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

    ret = RegQueryValueExW(m_key, subKey, nullptr, nullptr, data.data(), &dataSize);
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

QVariant QWinRegistryKey::value(const QString &subKey) const
{
    return value(nullTerminate(subKey));
}

// Returns the value of the specified subKey as a string, obtained using
// qvariant_cast from the underlying QVariant. If that value is not a string,
// or the subKey has no value, a string whose isNull() is true is returned.
// Otherwise, the resulting string (which may be empty) is returned.
QString QWinRegistryKey::stringValue(const wchar_t *subKey) const
{
    if (auto v = value<QString>(subKey))
        return std::move(*v);
    return QString();
}

QString QWinRegistryKey::stringValue(const QString &subKey) const
{
    return stringValue(nullTerminate(subKey));
}

void QWinRegistryKey::connectNotify(const QMetaMethod &signal)
{
    if (signal != QMetaMethod::fromSignal(&QWinRegistryKey::valueChanged))
        return;

    if (!isValid())
        return;

    if (m_keyChangedEvent)
        return;

    m_keyChangedEvent.reset(CreateEvent(nullptr, false, false, nullptr));
    auto *notifier = new QWinEventNotifier(m_keyChangedEvent.get(), this);

    auto registerForNotification = [this] {
        constexpr auto changeFilter =
              REG_NOTIFY_CHANGE_NAME
            | REG_NOTIFY_CHANGE_ATTRIBUTES
            | REG_NOTIFY_CHANGE_LAST_SET
            | REG_NOTIFY_CHANGE_SECURITY;

        if (auto status = RegNotifyChangeKeyValue(m_key, true, changeFilter,
            m_keyChangedEvent.get(), true); status != ERROR_SUCCESS) {
            qWarning() << "Failed to register notification for registry key"
                       << this << "due to" << QSystemError::windowsString(status);
        }
    };

    QObject::connect(notifier, &QWinEventNotifier::activated, this,
        [this, registerForNotification] {
            emit valueChanged();
            registerForNotification();
        });

    registerForNotification();

    return QObject::connectNotify(signal);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QWinRegistryKey &key)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QWinRegistryKey(";
    if (key.isValid())
        debug << key.name();
    debug << ')';
    return debug;
}
#endif

QT_END_NAMESPACE
