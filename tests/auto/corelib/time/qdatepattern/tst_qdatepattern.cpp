// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "private/qttemporalpattern_p.h"

#include "QtCore/qlatin1stringview.h"
#include <QTest>

using namespace Qt::StringLiterals;

class tst_QDatePattern : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fromQtFormat_data();
    void fromQtFormat();
    void forLocale_data();
    void forLocale();

private:
    static constexpr QLatin1StringView localeTags[] = {
        "en"_L1, "en-GB"_L1,
        "ff-Adlm-GN"_L1, // non-BMP-digits
    };
};

void tst_QDatePattern::fromQtFormat_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QCalendar::System>("system");
    // Convention: use int for baseYear, but since relevant calendars have no
    // year zero, 0 means unset.
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QDate>("date");

    const auto addRows = [](QByteArrayView stem, QString &&format,
                                QCalendar::System system, int baseYear, QDate date) {
        for (const auto &loc : localeTags) {
            const QString locName(loc);
            QTest::addRow("%s/%s", stem.constData(), loc.constData())
                << format << locName << system << baseYear << date;
        }
    };
    using Sys = QCalendar::System;
    addRows("empy", u""_s, Sys::Gregorian, 0, QDate());
    // Base year doesn't matter for four-digit year format.
    addRows("yyyy-MM-dd/gregor", u"yyyy-MM-dd"_s, Sys::Gregorian, 0, QDate(2025, 12, 4));
    addRows("yyyy-MM-dd/julius", u"yyyy-MM-dd"_s, Sys::Julian, 0, QDate(2025, 12, 4));
    addRows("yyyy-MM-dd/milank", u"yyyy-MM-dd"_s, Sys::Milankovic, 0, QDate(2025, 12, 4));
    // Should revise base year anyway for these next two, as their numbering is different.
#if QT_CONFIG(jalalicalendar)
    addRows("yyyy-MM-dd/persia", u"yyyy-MM-dd"_s, Sys::Jalali, 0, QDate(2025, 12, 4));
#endif
#if QT_CONFIG(islamiccivilcalendar)
    addRows("yyyy-MM-dd/islamic", u"yyyy-MM-dd"_s, Sys::IslamicCivil, 0, QDate(2025, 12, 4));
#endif
}

void tst_QDatePattern::fromQtFormat()
{
    QFETCH(const QString, format);
    QFETCH(const QString, localeName);
    QFETCH(const QCalendar::System, system);
    QFETCH(const int, baseYear);
    // QFETCH(const QDate, date);

    const QLocale locale(localeName);
    const QCalendar calendar(system);
    auto pattern = QDatePattern::fromQtFormat(format);
    pattern.setLocale(locale);
    pattern.setCalendar(calendar);
    if (baseYear)
        pattern.setBaseYear(baseYear);

    QCOMPARE(pattern.locale(), locale);
    QCOMPARE(pattern.calendar().name(), calendar.name());
    if (const auto base = pattern.baseYear())
        QCOMPARE(*base, baseYear);
    else
        QCOMPARE(baseYear, 0);

    // TODO: round-trip tests, once serialization is implemented
}

void tst_QDatePattern::forLocale_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QLocale::FormatType>("type");
    QTest::addColumn<QCalendar::System>("system");
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QDate>("date");

    const auto baseYearFor = [](QCalendar cal, QDate date) {
        // A year late in the previous century for dates in the first half of
        // the next, or early in the present century for dates late in it.
        const int year = date.year(cal);
        const int y2 = year % 100;
        const int century = year - y2;
        return y2 < 50 ? century - 34 : century + 33;
    };
    constexpr QLocale::FormatType types[] = {
        QLocale::LongFormat, QLocale::ShortFormat, QLocale::NarrowFormat
    };
    using Sys = QCalendar::System;
    constexpr struct { Sys system; QLatin1StringView nick; } calendars[] = {
        { Sys::Gregorian, "gregor"_L1 },
        { Sys::Julian, "julius"_L1 }, { Sys::Milankovic, "milank"_L1 },
#if QT_CONFIG(jalalicalendar)
        { Sys::Jalali, "persia"_L1 },
#endif
#if QT_CONFIG(islamiccivilcalendar)
        { Sys::IslamicCivil, "islamic"_L1 },
#endif
    };
    constexpr struct { int yr; int mon; int day; } dateTable[] = {
        { 1900, 1, 1 }, { 1970, 1, 1 }, { 1999, 12, 31 },
        { 2000, 2, 29 }, { 2000, 12, 31 }, { 2025, 12, 5 },
    };
    for (const auto &loc : localeTags) {
        const QString locale(loc);
        for (const auto &cal : calendars) {
            const QCalendar calendar(cal.system);
            for (const auto when : dateTable) {
                const QDate date(when.yr, when.mon, when.day);
                int base = baseYearFor(calendar, date);
                for (const auto type : types) {
                    const char *fmt = [](QLocale::FormatType type) {
                        switch (type) {
                        case QLocale::LongFormat: return "long";
                        case QLocale::ShortFormat: return "shrt";
                        case QLocale::NarrowFormat: return "nrow";
                        }
                        Q_UNREACHABLE_RETURN("<unknown>");
                    }(type);
                    QTest::addRow("%s/%s/%s/%04d-%02d-%02d",
                                  loc.constData(), cal.nick.constData(),
                                  fmt, when.yr, when.mon, when.day)
                        << locale << type << cal.system << base << date;
                }
            }
        }
    }
}

void tst_QDatePattern::forLocale()
{
    QFETCH(const QString, localeName);
    QFETCH(const QLocale::FormatType, type);
    QFETCH(const QCalendar::System, system);
    QFETCH(const int, baseYear);
    // QFETCH(const QDate, date);
    const QLocale locale(localeName);
    const QCalendar calendar(system);

    auto pattern = QDatePattern::forLocale(locale, type);
    pattern.setCalendar(calendar);
    if (baseYear)
        pattern.setBaseYear(baseYear);

    QCOMPARE(pattern.locale(), locale);
    QCOMPARE(pattern.calendar().name(), calendar.name());
    if (const auto base = pattern.baseYear())
        QCOMPARE(*base, baseYear);
    else
        QCOMPARE(baseYear, 0);

    // TODO: round-trip tests, once serialization is implemented
}

QTEST_APPLESS_MAIN(tst_QDatePattern)
#include "tst_qdatepattern.moc"
