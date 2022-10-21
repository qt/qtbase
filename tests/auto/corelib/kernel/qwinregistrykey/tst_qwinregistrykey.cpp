// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QObject>
#include <QPair>
#include <QScopeGuard>
#include <private/qwinregistry_p.h>
#include <qt_windows.h>

using namespace Qt::StringLiterals;

static constexpr const wchar_t TEST_KEY[] = LR"(SOFTWARE\tst_qwinregistrykey)";

static const QPair<QStringView, QString> TEST_STRING = qMakePair(u"string", u"string"_s);
static const QPair<QStringView, QString> TEST_STRING_NULL = qMakePair(u"string_null", QString());
static const QPair<QStringView, QStringList> TEST_STRINGLIST = qMakePair(u"stringlist", QStringList{ u"element1"_s, u"element2"_s, u"element3"_s });
static const QPair<QStringView, QStringList> TEST_STRINGLIST_NULL = qMakePair(u"stringlist_null", QStringList());
static const QPair<QStringView, quint32> TEST_DWORD = qMakePair(u"dword", 123);
static const QPair<QStringView, quint64> TEST_QWORD = qMakePair(u"qword", 456);
static const QPair<QStringView, QByteArray> TEST_BINARY = qMakePair(u"binary", "binary\0"_ba);
static const QPair<QStringView, QVariant> TEST_NOT_EXIST = qMakePair(u"not_exist", QVariant());
static const QPair<QStringView, QVariant> TEST_DEFAULT = qMakePair(u"", u"default"_s);

[[nodiscard]] static inline bool write(const HKEY key, const QStringView name, const QVariant &value)
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

    const LONG ret = RegSetValueExW(key, reinterpret_cast<const wchar_t *>(name.utf16()),
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
    #define C_STR(View) reinterpret_cast<const wchar_t *>(View.utf16())
    RegDeleteValueW(key, C_STR(TEST_STRING.first));
    RegDeleteValueW(key, C_STR(TEST_STRING_NULL.first));
    RegDeleteValueW(key, C_STR(TEST_STRINGLIST.first));
    RegDeleteValueW(key, C_STR(TEST_STRINGLIST_NULL.first));
    RegDeleteValueW(key, C_STR(TEST_DWORD.first));
    RegDeleteValueW(key, C_STR(TEST_QWORD.first));
    RegDeleteValueW(key, C_STR(TEST_BINARY.first));
    RegDeleteValueW(key, C_STR(TEST_DEFAULT.first));
    #undef C_STR
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
        const auto value = registry.dwordValue(TEST_DWORD.first);
        QVERIFY(value.second);
        QCOMPARE(value.first, TEST_DWORD.second);
    }

    {
        const auto value = registry.dwordValue(TEST_NOT_EXIST.first);
        QVERIFY(!value.second);
        QCOMPARE(value.first, DWORD(0));
    }
}

QTEST_MAIN(tst_qwinregistrykey)

#include "tst_qwinregistrykey.moc"
