// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>

#include <QtCore/qcompilerdetection.h>
#include <QtCore/qdebug.h>
#include <QtCore/qfloat16format.h>
#include <QtCore/qstring.h>

using namespace Qt::StringLiterals;

class tst_QFloat16Format : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void formatCompileTime();
    void format_data();
    void format();
    void formatMultiArg();
};

void tst_QFloat16Format::initTestCase()
{
#ifndef QT_SUPPORTS_STD_FORMAT
    QSKIP("This test requires std::format support!");
#endif
}

void tst_QFloat16Format::formatCompileTime()
{
#ifdef QT_SUPPORTS_STD_FORMAT
    // Starting from __cpp_lib_format == 202106L,
    // std::format requires the format string to be evaluated at compile-time,
    // so check it here.

    const qfloat16 val{1.234f};
    std::locale loc{"C"};

    // char
    std::string buffer;
    std::format_to(std::back_inserter(buffer), "{}", val);
    std::format_to(std::back_inserter(buffer), "{:*>15.7f}", val);
    std::format_to(std::back_inserter(buffer), "{:*^+#15.7g}", val);
    std::format_to(std::back_inserter(buffer), "{:*<-#15.7A}", val);
    std::format_to(std::back_inserter(buffer), "{:*^ 15.7e}", val);
    std::format_to(std::back_inserter(buffer), loc, "{:*^10.3Lf}", val);
    std::format_to(std::back_inserter(buffer), loc, "{:*< 10.7LE}", val);

    // wchar_t
    std::wstring wbuffer;
    std::format_to(std::back_inserter(wbuffer), L"{}", val);
    std::format_to(std::back_inserter(wbuffer), L"{:*>15.7f}", val);
    std::format_to(std::back_inserter(wbuffer), L"{:*^+#15.7g}", val);
    std::format_to(std::back_inserter(wbuffer), L"{:*<-#15.7A}", val);
    std::format_to(std::back_inserter(wbuffer), L"{:*^ 15.7e}", val);
    std::format_to(std::back_inserter(wbuffer), loc, L"{:*^10.3Lf}", val);
    std::format_to(std::back_inserter(wbuffer), loc, L"{:*< 10.7LE}", val);
#endif // QT_SUPPORTS_STD_FORMAT
}

void tst_QFloat16Format::format_data()
{
#ifdef QT_SUPPORTS_STD_FORMAT
    QTest::addColumn<QString>("format");
    QTest::addColumn<qfloat16>("value");
    QTest::addColumn<std::locale>("locale");
    QTest::addColumn<QString>("expectedString");

    auto row = [](const QString &format, qfloat16 val, const QString &expected,
                  const std::locale &loc = std::locale::classic())
    {
        QTest::addRow("%s:%s", loc.name().c_str(), qPrintable(format))
                << format << val << loc << expected;
    };

    row(u"{}"_s, qfloat16(1.f), u"1"_s);
    row(u"{:#}"_s, qfloat16(1.f), u"1."_s);
    row(u"{:f}"_s, qfloat16(1.f), u"1.000000"_s);
    row(u"{:*>10.2a}"_s, qfloat16(-1.23f), u"**-1.3bp+0"_s);

    try {
        // check if this locale is a) valid and b) works as expected
#if defined(Q_CC_MSVC)
        std::locale loc("de-DE");
#else
        std::locale loc("de_DE");
#endif
        if (std::format(loc, "{:L}", 1.25) == "1,25") {
            row(u"{:+Lf}"_s, qfloat16(1.f), u"+1,000000"_s, loc);
            row(u"{:*^10.3LF}"_s, qfloat16(-0.1234f), u"**-0,123**"_s, loc);
            row(u"{:*^#10.4Lg}"_s, qfloat16(-1.f), u"**-1,000**"_s, loc);
            row(u"{:*<14.3LE}"_s, qfloat16(-0.1234f), u"-1,234E-01****"_s, loc);
        }
    } catch (const std::runtime_error &) {
        // locale doesn't exist (std::locale constructor threw)
    }
#endif // QT_SUPPORTS_STD_FORMAT
}

void tst_QFloat16Format::format()
{
#ifdef QT_SUPPORTS_STD_FORMAT
    QFETCH(const QString, format);
    QFETCH(const qfloat16, value);
    QFETCH(const std::locale, locale);
    QFETCH(const QString, expectedString);

    // char
    {
        std::string buffer;
        const auto formatStr = format.toStdString();
        std::vformat_to(std::back_inserter(buffer), locale, formatStr,
                        std::make_format_args(value));
        const QString actualString = QString::fromStdString(buffer);
        QCOMPARE_EQ(actualString, expectedString);
    }

    // wchar_t
    {
        std::wstring buffer;
        const auto formatStr = format.toStdWString();
        std::vformat_to(std::back_inserter(buffer), locale, formatStr,
                        std::make_wformat_args(value));
        const QString actualString = QString::fromStdWString(buffer);
        QCOMPARE_EQ(actualString, expectedString);
    }
#endif // QT_SUPPORTS_STD_FORMAT
}

void tst_QFloat16Format::formatMultiArg()
{
#ifdef QT_SUPPORTS_STD_FORMAT
    const qfloat16 v1{-0.1234f};
    const qfloat16 v2{5.67f};

    const QString expectedString = u"**+5.67**_*****-1.234E-01"_s;
    // char
    {
        std::string buffer;
        std::format_to(std::back_inserter(buffer), "{1:*^+9.2f}_{0:*>15.3E}", v1, v2);
        QCOMPARE_EQ(QString::fromStdString(buffer), expectedString);
    }

    // wchar_t
    {
        std::wstring buffer;
        std::format_to(std::back_inserter(buffer), L"{1:*^+9.2f}_{0:*>15.3E}", v1, v2);
        QCOMPARE_EQ(QString::fromStdWString(buffer), expectedString);
    }
#endif // QT_SUPPORTS_STD_FORMAT
}

QTEST_MAIN(tst_QFloat16Format)
#include "tst_qfloat16format.moc"
