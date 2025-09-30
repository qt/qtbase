// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <QObject>
#include <QScopeGuard>
#include <private/qwinregistry_p.h>
#include <qt_windows.h>

using namespace Qt::StringLiterals;

static constexpr const wchar_t TEST_KEY[] = LR"(SOFTWARE\tst_qwinregistrykey)";

const std::pair TEST_STRING{L"string", u"string"_s};
const std::pair TEST_STRING_NULL{L"string_null", QString()};
const std::pair TEST_STRINGLIST{L"stringlist", QStringList{u"element1"_s, u"element2"_s, u"element3"_s}};
const std::pair TEST_STRINGLIST_NULL{L"stringlist_null", QStringList()};
const std::pair TEST_DWORD{L"dword", 123};
const std::pair TEST_QWORD{L"qword", 456};
const std::pair TEST_BINARY{L"binary", "binary\0"_ba};
const std::pair TEST_NOT_EXIST{L"not_exist", QVariant()};
const std::pair TEST_DEFAULT{L"", u"default"_s};

[[nodiscard]] static inline bool write(const HKEY key, const wchar_t *name, const QVariant &value)
{
    DWORD type = REG_NONE;
    QByteArray buf = {};

    switch (value.typeId()) {
        case QMetaType::QStringList: {
            // If none of the elements contains '\0', we can use REG_MULTI_SZ, the
            // native registry string list type. Otherwise we use REG_BINARY.
            type = REG_MULTI_SZ;
            const QStringList list = value.toStringList();
            for (auto it = list.constBegin(); it != list.constEnd(); ++it) {
                if ((*it).length() == 0 || it->contains(QChar::Null)) {
                    type = REG_BINARY;
                    break;
                }
            }

            if (type == REG_BINARY) {
                const QString str = value.toString();
                buf = QByteArray(reinterpret_cast<const char *>(str.data()), str.length() * 2);
            } else {
                for (auto it = list.constBegin(); it != list.constEnd(); ++it) {
                    const QString &str = *it;
                    buf += QByteArray(reinterpret_cast<const char *>(str.utf16()), (str.length() + 1) * 2);
                }
                // According to Microsoft Docs, REG_MULTI_SZ requires double '\0'.
                buf.append((char)0);
                buf.append((char)0);
            }
            break;
        }

        case QMetaType::Int:
        case QMetaType::UInt: {
            type = REG_DWORD;
            quint32 num = value.toUInt();
            buf = QByteArray(reinterpret_cast<const char *>(&num), sizeof(quint32));
            break;
        }

        case QMetaType::LongLong:
        case QMetaType::ULongLong: {
            type = REG_QWORD;
            quint64 num = value.toULongLong();
            buf = QByteArray(reinterpret_cast<const char *>(&num), sizeof(quint64));
            break;
        }

        case QMetaType::QByteArray:
        default: {
            // If the string does not contain '\0', we can use REG_SZ, the native registry
            // string type. Otherwise we use REG_BINARY.
            const QString str = value.toString();
            type = str.contains(QChar::Null) ? REG_BINARY : REG_SZ;
            int length = str.length();
            if (type == REG_SZ)
                ++length;
            buf = QByteArray(reinterpret_cast<const char *>(str.utf16()), sizeof(wchar_t) * length);
            break;
        }
    }

    const LONG ret = RegSetValueExW(key, name,
                                    0, type, reinterpret_cast<LPBYTE>(buf.data()), buf.size());
    return ret == ERROR_SUCCESS;
}

class tst_qwinregistrykey : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void qwinregistrykey();
    void name();
    void valueChanged();

private:
    bool m_available = false;
};

void tst_qwinregistrykey::initTestCase()
{
    HKEY key = nullptr;
    const LONG ret = RegCreateKeyExW(HKEY_CURRENT_USER, TEST_KEY, 0, nullptr, 0,
                                     KEY_READ | KEY_WRITE, nullptr, &key, nullptr);
    if (ret != ERROR_SUCCESS)
        return;
    const auto cleanup = qScopeGuard([key](){ RegCloseKey(key); });
    if (!write(key, TEST_STRING.first, TEST_STRING.second))
        return;
    if (!write(key, TEST_STRING_NULL.first, TEST_STRING_NULL.second))
        return;
    if (!write(key, TEST_STRINGLIST.first, TEST_STRINGLIST.second))
        return;
    if (!write(key, TEST_STRINGLIST_NULL.first, TEST_STRINGLIST_NULL.second))
        return;
    if (!write(key, TEST_DWORD.first, TEST_DWORD.second))
        return;
    if (!write(key, TEST_QWORD.first, TEST_QWORD.second))
        return;
    if (!write(key, TEST_BINARY.first, TEST_BINARY.second))
        return;
    if (!write(key, TEST_DEFAULT.first, TEST_DEFAULT.second))
        return;
    m_available = true;
}

void tst_qwinregistrykey::cleanupTestCase()
{
    HKEY key = nullptr;
    const LONG ret = RegOpenKeyExW(HKEY_CURRENT_USER, TEST_KEY, 0, KEY_READ | KEY_WRITE, &key);
    if (ret != ERROR_SUCCESS)
        return;
    RegDeleteTree(key, nullptr);
    RegDeleteKeyW(HKEY_CURRENT_USER, TEST_KEY);
    RegCloseKey(key);
}

void tst_qwinregistrykey::qwinregistrykey()
{
    if (!m_available)
        QSKIP("The test data is not ready.");

    QWinRegistryKey registry(HKEY_CURRENT_USER, TEST_KEY);

    QVERIFY(registry.isValid());

    QVERIFY(registry.handle() != nullptr);

    {
        const auto value = registry.value<QString>(TEST_STRING.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(QString()), TEST_STRING.second);
    }

    {
        const auto value = registry.value<QString>(TEST_STRING_NULL.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(QString()), TEST_STRING_NULL.second);

    }

    {
        const auto value = registry.value<QStringList>(TEST_STRINGLIST.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(QStringList()), TEST_STRINGLIST.second);
    }

    {
        const auto value = registry.value<QStringList>(TEST_STRINGLIST_NULL.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(QStringList()), TEST_STRINGLIST_NULL.second);
    }

    {
        const auto value = registry.value<quint32>(TEST_DWORD.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(0), TEST_DWORD.second);
    }

    {
        const auto value = registry.value<quint64>(TEST_QWORD.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(0), TEST_QWORD.second);
    }

    {
        const auto value = registry.value<QByteArray>(TEST_BINARY.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(QByteArray()), TEST_BINARY.second);
    }

    {
        const auto value = registry.value<QVariant>(TEST_NOT_EXIST.first);
        QVERIFY(!value.has_value());
        QCOMPARE(value.value_or(QVariant()), QVariant());
    }

    {
        const auto value = registry.value<QString>(TEST_DEFAULT.first);
        QVERIFY(value.has_value());
        QCOMPARE(value.value_or(QString()), TEST_DEFAULT.second);
    }

    {
        const QString value = registry.stringValue(TEST_STRING.first);
        QVERIFY(!value.isEmpty());
        QCOMPARE(value, TEST_STRING.second);
    }

    QVERIFY(registry.stringValue(TEST_NOT_EXIST.first).isEmpty());

    {
        const auto value = registry.value<DWORD>(TEST_DWORD.first);
        QVERIFY(value);
        QCOMPARE(*value, TEST_DWORD.second);
    }

    {
        const auto value = registry.value<DWORD>(TEST_NOT_EXIST.first);
        QVERIFY(!value);
    }
}

void tst_qwinregistrykey::name()
{
    if (!m_available)
        QSKIP("The test data is not ready.");

    QWinRegistryKey testKey(HKEY_CURRENT_USER, TEST_KEY);
    QVERIFY(testKey.isValid());

    // In practice: "\\REGISTRY\\USER\\S-1-5-21-4156955479-607706614-2054699034-1000\\Software\\tst_qwinregistrykey",
    //          or: "\\REGISTRY\\USER\\S-1-5-21-4156955479-607706614-2054699034-1000\\SOFTWARE\\tst_qwinregistrykey".
    QVERIFY(testKey.name().toLower().endsWith("software\\tst_qwinregistrykey"));

    // Check that we can report the name of a key with a deep path
    HKEY baseKey = testKey.handle();
    constexpr auto kKeyDepth = 500;
    for (int i = 0; i < kKeyDepth; ++i) {
        constexpr auto kChildKeyName = LR"(childKey)";

        HKEY childKey = nullptr;
        auto ret = RegCreateKeyEx(baseKey, kChildKeyName, 0, nullptr, 0,
            KEY_READ | KEY_WRITE, nullptr, &childKey, nullptr);
        QVERIFY(ret == ERROR_SUCCESS);

        if (i == kKeyDepth - 1) {
            RegCloseKey(childKey);
            QWinRegistryKey leafKey(baseKey, kChildKeyName);
            QVERIFY(leafKey.isValid());
            const QString keyName = leafKey.name();
            QVERIFY(keyName.size() > 1000);
            QVERIFY(keyName.endsWith("childKey\\childKey\\childKey"));
        } else {
            if (baseKey != testKey.handle())
                RegCloseKey(baseKey);
            baseKey = childKey;
        }
    }
}

void tst_qwinregistrykey::valueChanged()
{
    if (!m_available)
        QSKIP("The test data is not ready.");

    QWinRegistryKey testKey(HKEY_CURRENT_USER, TEST_KEY, KEY_READ | KEY_WRITE);
    QVERIFY(testKey.isValid());

    QVERIFY(write(testKey, L"valueThatCanChange", -1));

    bool valueChanged = false;
    QObject::connect(&testKey, &QWinRegistryKey::valueChanged, [&] {
        valueChanged = true;
    });

    for (int i = 0; i < 10; ++i) {
        valueChanged = false;
        QVERIFY(write(testKey, L"valueThatCanChange", i));
        QTRY_VERIFY(valueChanged);
    }
}

QTEST_MAIN(tst_qwinregistrykey)

#include "tst_qwinregistrykey.moc"
