// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>

#ifdef Q_OS_WIN

#include <QtCore/private/qbstr_p.h>

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;

typedef int (*SETOANOCACHE)(void);

void DisableBSTRCache()
{
    const HINSTANCE hLib = LoadLibraryA("OLEAUT32.DLL");
    if (hLib != nullptr) {
        const SETOANOCACHE SetOaNoCache =
                reinterpret_cast<SETOANOCACHE>(GetProcAddress(hLib, "SetOaNoCache"));
        if (SetOaNoCache != nullptr)
            SetOaNoCache();
        FreeLibrary(hLib);
    }
}

class tst_qbstr : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        DisableBSTRCache();
    }

    void constructor_nullInitializes()
    {
        const QBStr str;
        QVERIFY(!str.bstr());
    }

    void constructor_initializesToNullptr_withNullptr()
    {
        const QBStr str{ nullptr };
        QCOMPARE_EQ(str.bstr(), nullptr);
    }

    void constructor_initializes_withLiteral()
    {
        const QBStr str{ L"hello world" };
        QCOMPARE_EQ(str.bstr(), "hello world"_L1);
    }

    void constructor_initializes_withQString()
    {
        const QString expected{ "hello world"_L1 };
        QBStr str{ expected };
        QCOMPARE_EQ(QStringView{ str.bstr() }, expected);
    }

    void constructor_initializesToNullptr_withNullQString()
    {
        const QString expected{ };
        QBStr str{ expected };
        QCOMPARE_EQ(str.bstr(), nullptr);
    }

    void constructor_initializes_withEmptyQString()
    {
        const QString expected{ ""_L1 };
        QBStr str{ expected };
        QCOMPARE_EQ(QStringView{ str.bstr() }, expected);
        QCOMPARE_NE(str.bstr(), nullptr);
    }

    void copyConstructor_initializesToNullptr_withNullString()
    {
        const QBStr null;
        const QBStr dest = null;
        QCOMPARE_EQ(dest.bstr(), nullptr);
    }

    void copyConstructor_initializes_withValidQBstr()
    {
        const QBStr expected{ L"hello world" };
        const QBStr dest = expected;
        QCOMPARE_EQ(QStringView{ dest.bstr() }, expected.bstr());
    }

    void moveConstructor_initializesToNullptr_withNullQBstr()
    {
        const QBStr str{ QBStr{} };
        QCOMPARE_EQ(str.bstr(), nullptr);
    }

    void moveConstructor_initializes_withValidQBstr()
    {
        QBStr source { L"hello world" };
        const QBStr str{ std::move(source) };
        QCOMPARE_EQ(str.bstr(), "hello world"_L1);
    }

    void assignment_assigns_withNullptr()
    {
        QBStr value{};
        value = nullptr;
        QCOMPARE_EQ(value.bstr(), nullptr);
    }

    void assignment_assigns_WCharPointer()
    {
        QBStr value{};
        const wchar_t utf16[] = L"Hello world";
        const wchar_t *ptr = utf16;
        value = ptr;
        QCOMPARE_EQ(value.bstr(), "Hello world"_L1);
    }

    void assignment_assigns_WCharLiteral()
    {
        QBStr value{};
        value = L"Hello world";
        QCOMPARE_EQ(value.bstr(), "Hello world"_L1);
    }

    void assignment_assignsEmpty_EmptyWCharLiteral()
    {
        QBStr value{};
        value = L"";
        QCOMPARE_EQ(value.bstr(), ""_L1);
        QCOMPARE_NE(value.bstr(), nullptr);
    }

    void assignment_assigns_withQString()
    {
        QBStr value{};
        value = QString{ "Hello world"_L1 };
        QCOMPARE_EQ(value.bstr(), "Hello world"_L1);
    }

    void assignment_assignsNullptr_withNullQString()
    {
        QBStr value{};
        value = QString{};
        QCOMPARE_EQ(value.bstr(), nullptr);
    }

    void assignment_assignsEmpty_withEmptyQString()
    {
        QBStr value{};
        value = QString{""_L1};
        QCOMPARE_EQ(value.bstr(), ""_L1);
        QCOMPARE_NE(value.bstr(), nullptr);
    }

    void assignment_assignsNullptr_withNullQBStr()
    {
        const QBStr source{};
        const QBStr dest = source;
        QCOMPARE_EQ(dest.bstr(), nullptr);
    }

    void assignment_assigns_withValidQBStr()
    {
        const QBStr source{ L"hello world" };
        const QBStr dest = source;
        QCOMPARE_EQ(dest.bstr(), "hello world"_L1);
    }

    void assignment_assigns_withSelfAssignment()
    {
        QBStr value{ L"hello world" };
        value = value;
        QCOMPARE_EQ(value.bstr(), "hello world"_L1);
    }

    void moveAssignment_assigns_withNullQBStr()
    {
        QBStr source{};
        const QBStr dest = std::move(source);
        QCOMPARE_EQ(dest.bstr(), nullptr);
    }

    void moveAssignment_assigns_withValidQBStr()
    {
        QBStr source{ L"hello world" };
        const QBStr dest = std::move(source);
        QCOMPARE_EQ(dest.bstr(), "hello world"_L1);
    }

    void moveAssignment_assigns_withSelfAssignment()
    {
        QBStr value{ L"hello world" };
        value = std::move(value);
        QCOMPARE_EQ(value.bstr(), "hello world"_L1);
    }

    void release_returnsStringAndClearsValue()
    {
        QBStr str{ L"hello world" };
        BSTR val = str.release();
        QCOMPARE_EQ(str.bstr(), nullptr);
        QCOMPARE_EQ(val, "hello world"_L1);
        SysFreeString(val);
    }

    void str_returnsNullQString_withNullQBStr()
    {
        const QBStr bstr{};
        const QString str = bstr.str();
        QVERIFY(str.isNull());
    }

    void str_returnsQString_withValidQBStr()
    {
        const QBStr bstr{L"hello world"};
        const QString str = bstr.str();
        QCOMPARE_EQ(str, "hello world");
    }

};

QTEST_MAIN(tst_qbstr)
#include "tst_qbstr.moc"

#endif // Q_OS_WIN
