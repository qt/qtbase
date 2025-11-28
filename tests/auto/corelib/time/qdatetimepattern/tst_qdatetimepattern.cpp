// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "private/qttemporalpattern_p.h"

#include "QtCore/qbytearray.h"
#include "QtCore/qbytearrayview.h"
#include "QtCore/qlatin1stringview.h"
#include <QTest>

using namespace Qt::StringLiterals;

class tst_QDateTimePattern : public QObject
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

void tst_QDateTimePattern::fromQtFormat_data()
{
    QTest::addColumn<QString>("format");
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QCalendar::System>("system");
    // Convention: use int for baseYear, but since relevant calendars have no
    // year zero, 0 means unset.
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QDateTime>("dateTime");
    const auto addRows = [](QByteArrayView stem, QString &&format,
                                QCalendar::System system, int baseYear, QDateTime dateTime) {
        for (const auto &loc : localeTags) {
            const QString locName(loc);
            QTest::addRow("%s/%s", stem.constData(), loc.constData())
                << format << locName << system << baseYear << dateTime;
        }
    };
    using Sys = QCalendar::System;
    addRows("empy", u""_s, Sys::Gregorian, 0, QDateTime());
    const QDateTime basic(QDate(2025, 12, 4), QTime(12, 0), QTimeZone::UTC);
    // Base year doesn't matter for four-digit year format.
    addRows("yyyy-MM-dd HH:mm:ss.zzz tt/gregor",
            u"yyyy-MM-dd HH:mm:ss.zzz tt"_s, Sys::Gregorian, 0, basic);
    addRows("yyyy-MM-dd HH:mm:ss.zzz tt/julius",
            u"yyyy-MM-dd HH:mm:ss.zzz tt"_s, Sys::Julian, 0, basic);
    addRows("yyyy-MM-dd HH:mm:ss.zzz tt/milank",
            u"yyyy-MM-dd HH:mm:ss.zzz tt"_s, Sys::Milankovic, 0, basic);
    // Should revise base year anyway for these next two, as their numbering is different.
#if QT_CONFIG(jalalicalendar)
    addRows("yyyy-MM-dd HH:mm:ss.zzz tt/persia",
            u"yyyy-MM-dd HH:mm:ss.zzz tt"_s, Sys::Jalali, 0, basic);
#endif
#if QT_CONFIG(islamiccivilcalendar)
    addRows("yyyy-MM-dd HH:mm:ss.zzz tt/islamic",
            u"yyyy-MM-dd HH:mm:ss.zzz tt"_s, Sys::IslamicCivil, 0, basic);
#endif
}

void tst_QDateTimePattern::fromQtFormat()
{
    QFETCH(const QString, format);
    QFETCH(const QString, localeName);
    QFETCH(const QCalendar::System, system);
    QFETCH(const int, baseYear);
    // QFETCH(const QDateTime, dateTime);

    const QLocale locale(localeName);
    const QCalendar calendar(system);
    auto pattern = QDateTimePattern::fromQtFormat(format);
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

void tst_QDateTimePattern::forLocale_data()
{
    QTest::addColumn<QString>("localeName");
    QTest::addColumn<QLocale::FormatType>("type");
    QTest::addColumn<QCalendar::System>("system");
    QTest::addColumn<int>("baseYear");
    QTest::addColumn<QDateTime>("dateTime");

    const auto baseYearFor = [](QCalendar cal, QDateTime dateTime) {
        // A year late in the previous century for dates in the first half of
        // the next, or early in the present century for dates late in it.
        const int year = dateTime.date().year(cal);
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
    const struct {
        int yr; int mon; int day;
        int hr; int min; int sec; int ms;
        QByteArray zone;
    } dateTimeTable[] = {
        { 1900, 1, 1, 0, 0, 0, 0, "UTC"_ba },
        { 1970, 1, 1, 12, 30, 30, 500, ""_ba },
        { 2000, 2, 29, 12, 0, 0, 0, "UTC"_ba }, // Yes, there was a leap day
#if QT_CONFIG(timezone) // Need backends for these:
        { 1969, 9, 30, 12, 0, 0, 0, "Pacific/Kwajalein"_ba }, // Repeated day
        { 1993, 8, 21, 12, 0, 0, 0, "Pacific/Kwajalein"_ba }, // Skipped day
        { 1994, 12, 31, 12, 0, 0, 0, "Pacific/Kiritimati"_ba }, // Skipped day
        { 1994, 12, 31, 12, 0, 0, 0, "Pacific/Kanton"_ba }, // Skipped day (an hour later)
        { 1999, 12, 31, 23, 59, 59, 999, "Pacific/Apia"_ba }, // Last moment of 1900s
        { 2000, 1, 1, 0, 0, 0, 0, "Pacific/Kiritimati"_ba }, // First moment of 2000s
        { 2000, 12, 31, 23, 59, 59, 999, "Pacific/Apia"_ba }, // Official end of 2nd millennium CE
        { 2001, 1, 1, 0, 0, 0, 0,  "Pacific/Kiritimati"_ba }, // Official start of 3rd millennium CE
        { 2011, 12, 30, 12, 0, 0, 0, "Pacific/Apia"_ba }, // Skipped day (also Pacific/Fakaofo)
        { 2025, 12, 5, 16, 54, 23, 0, "Europe/Oslo"_ba },
#endif // timezone
    };

    for (const auto &loc : localeTags) {
        const QString locale(loc);
        for (const auto &cal : calendars) {
            const QCalendar calendar(cal.system);
            for (const auto &when : dateTimeTable) {
                const QDateTime dateTime(QDate(when.yr, when.mon, when.day),
                                         QTime(when.hr, when.min, when.sec, when.ms),
                                         when.zone.isEmpty()
                                         ? QTimeZone(QTimeZone::LocalTime)
                                         :
#if QT_CONFIG(timezone)
                                         QTimeZone(when.zone)
#else // The only non-empty cases without backend are "UTC":
                                         QTimeZone(QTimeZone::UTC)
#endif
                    );
                if (!dateTime.isValid()) {
                    qWarning("Invalid %04d-%02d-%02d %02d:%02d:%02d.%03d %s - skipped",
                             when.yr, when.mon, when.day, when.hr, when.min, when.sec, when.ms,
                             when.zone.constData());
                    continue;
                }
                int base = baseYearFor(calendar, dateTime);
                for (const auto type : types) {
                    const char *fmt = [](QLocale::FormatType type) {
                        switch (type) {
                        case QLocale::LongFormat: return "long";
                        case QLocale::ShortFormat: return "shrt";
                        case QLocale::NarrowFormat: return "nrow";
                        }
                        Q_UNREACHABLE_RETURN("<unknown>");
                    }(type);
                    QTest::addRow("%s/%s/%s/%04d-%02d-%02dT%02d:%02d:%02d.%03d/%s",
                                  loc.constData(), cal.nick.constData(),
                                  fmt, when.yr, when.mon, when.day,
                                  when.hr, when.min, when.sec, when.ms,
                                  when.zone.constData())
                        << locale << type << cal.system << base << dateTime;
                }
            }
        }
    }
}

void tst_QDateTimePattern::forLocale()
{
    QFETCH(const QString, localeName);
    QFETCH(const QLocale::FormatType, type);
    QFETCH(const QCalendar::System, system);
    QFETCH(const int, baseYear);
    // QFETCH(const QDateTime, dateTime);
    const QLocale locale(localeName);
    const QCalendar calendar(system);

    auto pattern = QDateTimePattern::forLocale(locale, type);
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

QTEST_APPLESS_MAIN(tst_QDateTimePattern)
#include "tst_qdatetimepattern.moc"
