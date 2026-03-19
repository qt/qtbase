// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <private/qtparsetemporal_p.h>

#include <QtCore/qcalendar.h>
#include <QtCore/qlocale.h>
#include <QtCore/qstring.h>

QT_REQUIRE_CONFIG(datetimeparser);

using namespace Qt::StringLiterals;
using namespace QtTemporalPattern;

class tst_QtParseTemporal : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void prefix_data();
    void prefix();
};

void tst_QtParseTemporal::prefix_data()
{
    using Field = QtTemporalPattern::TemporalField;
    using Fields = QList<Field>;
    // Inputs
    QTest::addColumn<QString>("text");
    QTest::addColumn<Fields>("fields");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QCalendar::System>("cal");
    QTest::addColumn<int>("baseYear"); // 0 invalid so map to std::nullopt
    QTest::addColumn<int>("from");
    // Outputs expected (when until > 0):
    QTest::addColumn<int>("until"); // 0 => fail to parse
    QTest::addColumn<QTimeZone>("zone");
    QTest::addColumn<int>("millis");
    QTest::addColumn<int>("second");
    QTest::addColumn<int>("minute");
    QTest::addColumn<int>("hour");
    QTest::addColumn<int>("dayOfWeek");
    QTest::addColumn<int>("dayOfMonth");
    QTest::addColumn<int>("month");
    QTest::addColumn<int>("year");

    // Parsing treats "no zone found" as local time (a.k.a. wall-clock time)
    // since a timestamp with no zone indicator is assumed to mean that.
    const QTimeZone wall(QTimeZone::LocalTime);
    const QString empty;

    // Mainly to check we don't crash or trigger assertions:
    QTest::newRow("null/null/C/greg/0")
        << QString() << Fields() << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    QTest::newRow("empty/empty/C/greg/0")
        << empty << Fields{} << QLocale::c() << QCalendar::System::Gregorian << 0 << 0
        << 0 << wall << -1 << -1 << -1 << -1 << 0 << 0 << 0 << 0;
    // Those are technically successful parses of the no fields asked for.
}

void tst_QtParseTemporal::prefix()
{
    QFETCH(const QString, text);
    QFETCH(const QList<QtTemporalPattern::TemporalField>, fields);
    QFETCH(const QLocale, locale);
    QFETCH(const QCalendar::System, cal);
    QFETCH(const int, baseYear);
    QFETCH(const int, from);
    QFETCH(const int, until);

    const QCalendar calendar(cal);
    std::optional<int> century;
    if (baseYear)
        century = baseYear;
    const auto parsed = QtParseTemporal::prefix(text, fields, locale, calendar, century, from);
    if (until) {
        QVERIFY(!parsed.isEmpty());
        QCOMPARE(parsed.startIndex, from);
        QCOMPARE(parsed.endIndex, until);
        QTEST(parsed.zone, "zone");
        QTEST(parsed.millis, "millis");
        QTEST(parsed.second, "second");
        QTEST(parsed.minute, "minute");
        QTEST(parsed.hour, "hour");
        QTEST(parsed.dayOfWeek, "dayOfWeek");
        QTEST(parsed.dayOfMonth, "dayOfMonth");
        QTEST(parsed.month, "month");
        // Test uses year 0 as unspecified, but parser results can't because
        // some proleptic calendars have a year zero.
        QFETCH(const int, year);
        if (year) {
            QVERIFY2(parsed.year, "Year wasn't set but should have been");
            QCOMPARE(*parsed.year, year);
        } else {
            QVERIFY2(!parsed.year, "Year field should not have been set");
        }
    } else {
        QVERIFY(parsed.isEmpty());
    }
}

QTEST_APPLESS_MAIN(tst_QtParseTemporal)
#include "tst_qtparsetemporal.moc"
