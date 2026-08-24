// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <private/qtparseqttemporalformat_p.h>

#include <QtCore/qstring.h>
#include <QtCore/qscopeguard.h>

QT_REQUIRE_CONFIG(datestring);

using namespace Qt::StringLiterals;
using namespace QtTemporalPattern;

static QByteArray fieldString(const QtTemporalPattern::TemporalField &field)
{
    const auto showFlags = [](TemporalFieldFlags flags) {
        QString show;
        static const char16_t blank[] = u"", join[] = u" | ";
        const char16_t *sep = blank;
#define showFlag(name) do { \
            if (flags.testFlag(TemporalFieldFlag::name)) { \
                show += sep + QString::fromUtf8(#name); \
                sep = join; \
                flags.setFlag(TemporalFieldFlag::name, false); \
            } \
        } while (false) // end showFlag()
        showFlag(Numeric); showFlag(Verbal); showFlag(Standalone);
        showFlag(Narrow); showFlag(Abbreviated); showFlag(Short); showFlag(Wide);
        showFlag(ZeroPad); showFlag(SpacePad); showFlag(FlexSpace);
        showFlag(LowerCase); showFlag(UpperCase); showFlag(IgnoreCase);
        showFlag(GenericTime); showFlag(StandardTime); showFlag(DaylightSavingTime);
        showFlag(Iso8601); showFlag(AllowZSuffix);
        showFlag(LocalTimeName);
#undef showFlag
        if (flags)
            show += sep + (u"unrecognised flags: " + QString::number(flags.toInt()));
        return show;
    };
    const auto showCat = [](TemporalFieldCategory cat) {
        switch (cat) {
#define showCase(name) case TemporalFieldCategory::name: return QString::fromUtf8(#name)
            showCase(Literal);
            showCase(TimeZone);
            showCase(SecondFraction);
            showCase(Second);
            // showCase(SecondFraction);
            showCase(Minute);
            // showCase(MinuteFraction);
            showCase(PeriodInDay);
            showCase(HourMod12);
            showCase(Hour);
            // showCase(HourFraction);
            showCase(DayOfWeek);
            showCase(DayOfMonth);
            // showCase(DayOfYear); showCase(JulianDay);
            // showCase(WeekOfMonth);
            // showCase(WeekOfYear);
            showCase(Month);
            // showCase(Quarter);
            // showCase(RelatedGregorianYear); showCase(Century);
            showCase(YearWithinCentury);
            showCase(Year);
            // showCase(Era);
#undef showCase
        case TemporalFieldCategory::EndCategories:
            Q_UNREACHABLE();
        }
        return u"<unknown category>"_s;
    };

    return (u"TemporalField{ \"" + field.literal
            + u"\", " + QString::number(field.width)
            + u", " + showFlags(field.options)
            + u", " + showCat(field.category)
            + u" }").toUtf8();
}

class tst_QtParseQtTemporalFormat : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void prefix_data();
    void prefix();
};

void tst_QtParseQtTemporalFormat::prefix_data()
{
    using namespace QtParseQtTemporalFormat;
    using Part = QtTemporalPattern::DateTimePart;
    using Parts = QtTemporalPattern::DateTimeParts;
    using Field = QtTemporalPattern::TemporalField;
    using Fields = QList<QtTemporalPattern::TemporalField>;
    using Flag = QtTemporalPattern::TemporalFieldFlag;
    using Flags = QtTemporalPattern::TemporalFieldFlags;
    using Cat = QtTemporalPattern::TemporalFieldCategory;
    constexpr Parts AllParts = Part::Date | Part::Time | Part::Zone;
    constexpr Flags FullZoneName
        = Flag::LocalizedZone | Flag::Verbal | Flag::Standalone | Flag::Wide | Flag::Short
        | Flag::LocalTimeName;
    constexpr Flags TextCommon = Flag::IgnoreCase;

    QTest::addColumn<QString>("text");
    QTest::addColumn<Parts>("form");
    QTest::addColumn<int>("until"); // 0 => failed to parse
    QTest::addColumn<Fields>("fields");

    // Mainly to check we don't crash or trigger assertions:
    QTest::addRow("null/none") << QString() << Parts{} << 0 << Fields{};
    QTest::addRow("empty/none") << u""_s << Parts{} << 0 << Fields{};
    QTest::addRow("literal/none")
        << u"literal"_s << Parts{} << 7
        << Fields{ Field{u"literal"_s, 0, Flags{}, Cat::Literal} };
    QTest::addRow("quotey-literal/none")
        // Incidentally check a lone unquoted y is treated as literal, not year-field:
        << u"''li''y't'''eral''"_s << Parts{} << 18
        << Fields{ Field{u"'li'yt'eral'"_s, 0, Flags{}, Cat::Literal} };
    QTest::addRow("hh':'mm' 'aP'unclosed/time")
        << u"hh':'mm' 'aP'unclosed"_s << Parts{Part::Time} << 12
        << Fields{ Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::HourMod12},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 0, TextCommon, Cat::PeriodInDay} };
    QTest::addRow("hh':'mm' 'ss.zz'partially''unclosed/time")
        << u"hh':'mm' 'ss.zz'partially''unclosed"_s << Parts{Part::Time} << 26
        << Fields{ Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            // The absence of a PeriodInDay field has coerced the HourMod12 to an Hour.
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field{u"."_s, 0, Flags{}, Cat::Literal},
            // Although we used zz format, it counts as width 1:
            Field{QString(), 1, Flag::Numeric, Cat::SecondFraction},
            Field{u"partially"_s, 0, Flags{}, Cat::Literal} };

    // Variation in the parts to include:
    QTest::addRow("yyyy-MM-dd HH:mm:ss tttt/none")
        << u"yyyy-MM-dd HH:mm:ss tttt"_s << Parts{} << 24
        << Fields{ Field{u"yyyy-MM-dd HH:mm:ss tttt"_s, 0, Flags{}, Cat::Literal} };
    QTest::addRow("yyyy-MM-dd HH:mm:ss tttt/zone")
        << u"yyyy-MM-dd HH:mm:ss tttt"_s << Parts{Part::Zone} << 24
        << Fields{ Field{u"yyyy-MM-dd HH:mm:ss "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, FullZoneName, Cat::TimeZone} };
    QTest::addRow("yyyy-MM-dd HH:mm:s tttt/time")
        << u"yyyy-MM-dd HH:mm:s tttt"_s << Parts{Part::Time} << 23
        << Fields{ Field{u"yyyy-MM-dd "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Second},
            Field{u" tttt"_s, 0, Flags{}, Cat::Literal} };
    QTest::addRow("yyyy-MM-dd HH:m:s ttt/time+zone")
        << u"yyyy-MM-dd HH:m:s ttt"_s << Parts{Part::Time | Part::Zone} << 21
        << Fields{ Field{u"yyyy-MM-dd "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Second},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 3, Flag::Verbal | Flag::Iso8601 | Flag::ZeroPad, Cat::TimeZone} };
    QTest::addRow("yyyy-MM-dd HH:mm:ss tttt/date")
        << u"yyyy-MM-dd HH:mm:ss tttt"_s << Parts{Part::Date} << 24
        << Fields{ Field{QString(), 4,
                         Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Month},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth},
            Field{u" HH:mm:ss tttt"_s, 0, Flags{}, Cat::Literal} };
    QTest::addRow("yyyyy-M-dd HH:mm:ss tt/date+zone")
        << u"yyyyy-M-dd HH:mm:ss tt"_s << Parts{Part::Date | Part::Zone} << 22
        << Fields{ Field{QString(), 4,
                         Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            // The fifth 'y' is not a format character, so becomes part of a literal:
            Field{u"y-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Month},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth},
            Field{u" HH:mm:ss "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::Iso8601 | Flag::ZeroPad, Cat::TimeZone} };
    QTest::addRow("yyyy-MM-d HH:mm:ss tttt/date+time")
        << u"yyyy-MM-d HH:mm:ss tttt"_s << Parts{Part::Date | Part::Time} << 23
        << Fields{ Field{QString(), 4,
                         Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Month},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::DayOfMonth},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field{u" tttt"_s, 0, Flags{}, Cat::Literal} };
    QTest::addRow("yyyy-M-d HH:m:ss t/date+time+zone")
        << u"yyyy-M-d HH:m:ss t"_s << AllParts << 18
        << Fields{ Field{QString(), 4,
                         Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Month},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::DayOfMonth},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::AllowZSuffix | Flag::LocalTimeName, Cat::TimeZone} };
    // 3 parts => 8 variants.
    // Those have also varied field-length of minute, second, numeric month and day.

    QTest::addRow("RFC-strict/full")
        << u"ddd, dd MMM yyyy HH:mm:ss tt"_s << AllParts << 28
        << Fields{ Field{QString(), 3,
                         Flag::Verbal | Flag::Abbreviated | TextCommon, Cat::DayOfWeek},
            Field{u", "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 3, Flag::Verbal | Flag::Abbreviated | TextCommon, Cat::Month},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::Iso8601 | Flag::ZeroPad, Cat::TimeZone} };
    QTest::addRow("RFC-permissive/full")
        << u"ddd MMM dd HH:mm:ss yyyy tt"_s << AllParts << 27
        << Fields { Field{QString(), 3,
                          Flag::Verbal | Flag::Abbreviated | TextCommon, Cat::DayOfWeek},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 3, Flag::Verbal | Flag::Abbreviated | TextCommon, Cat::Month},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field{u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::Iso8601 | Flag::ZeroPad, Cat::TimeZone} };

    QTest::addRow("ISO-standard/full")
        << u"yyyy-MM-ddTHH:mm:ssttt"_s << AllParts << 22
        << Fields{ Field{QString(), 4,
                         Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Month},
            Field{u"-"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::DayOfMonth},
            Field{u"T"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field{QString(), 3, Flag::Verbal | Flag::Iso8601 | Flag::ZeroPad, Cat::TimeZone} };

    // Our own "C locale" formats, see end of util/locale_database/qlocalexml.py
    QTest::addRow("C-short/full")
        << u"d MM yyyy HH:mm t"_s << AllParts << 17
        << Fields{ Field{QString(), 1, Flag::Numeric, Cat::DayOfMonth},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Month},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::AllowZSuffix | Flag::LocalTimeName, Cat::TimeZone} };
    QTest::addRow("C-long/full")
        // Incidentally also test quirk: absent a/A field, h = H:
        << u"dddd, d MMMM yyyy hh:mm:ss tttt"_s << AllParts << 31
        << Fields{ Field{QString(), 4,
                         Flag::Verbal | Flag::Wide | TextCommon, Cat::DayOfWeek},
            Field {u", "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::DayOfMonth},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, Flag::Verbal | Flag::Wide | TextCommon, Cat::Month},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, Flag::Numeric | Flag::ZeroPad | Flag::YearSignIso8601, Cat::Year},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Hour},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::Second},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 4, FullZoneName, Cat::TimeZone} };

    QTest::addRow("quirky/full")
        << u"ddd, MMM yyy h:m:s.zzz Ap t"_s << AllParts << 27
        << Fields{ Field{QString(), 3,
                         Flag::Verbal | Flag::Abbreviated | TextCommon, Cat::DayOfWeek},
            Field {u", "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 3, Flag::Verbal | Flag::Abbreviated | TextCommon, Cat::Month},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 2, Flag::Numeric | Flag::ZeroPad, Cat::YearWithinCentury},
            // "yyy" is read as "yy" followed by literal "y".
            Field {u"y "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::HourMod12},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Minute},
            Field{u":"_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::Numeric, Cat::Second},
            Field{u"."_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 3, Flag::Numeric | Flag::ZeroPad, Cat::SecondFraction},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 0, TextCommon, Cat::PeriodInDay},
            Field {u" "_s, 0, Flags{}, Cat::Literal},
            Field{QString(), 1, Flag::AllowZSuffix | Flag::LocalTimeName, Cat::TimeZone} };
}

void tst_QtParseQtTemporalFormat::prefix()
{
    QFETCH(const QString, text);
    QFETCH(const QtTemporalPattern::DateTimeParts, form);
    QFETCH(const int, until);
    QFETCH(const QList<QtTemporalPattern::TemporalField>, fields);

    const auto parsed = QtParseQtTemporalFormat::prefix(text, form);
    if (until) {
        QVERIFY(!parsed.isEmpty());
        QCOMPARE(parsed.startIndex, 0);
        QCOMPARE(parsed.endIndex, until);
        auto report = qScopeGuard([parsed, fields]() {
            for (qsizetype i = 0; i < parsed.fields.size(); ++i) {
                const QtTemporalPattern::TemporalField &field = parsed.fields.at(i);
                if (i < fields.size()) {
                    if (const auto &expected = fields.at(i); field != expected) {
                        qDebug() << "Mismatch at index" << i << "actual:";
                        qDebug() << fieldString(field);
                        qDebug("expected:");
                        qDebug() << fieldString(expected);
                    }
                } else {
                    qDebug() << "Extra at index" << i << ':' << fieldString(field);
                }
            }
            for (qsizetype i = parsed.fields.size(); i < fields.size(); ++i)
                qDebug() << "Missing at index" << i << ':' << fieldString(fields.at(i));
        });
        QCOMPARE(parsed.fields, fields);
        report.dismiss();
    } else {
        QVERIFY(parsed.isEmpty());
    }
}

QTEST_APPLESS_MAIN(tst_QtParseQtTemporalFormat)
#include "tst_qtparseqttemporalformat.moc"
