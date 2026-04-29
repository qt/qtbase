// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <private/qtparsetimezone_p.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qlocale.h>
#include <QtCore/private/qlocale_p.h>
#include <QtCore/private/qlocaltime_p.h>
#include <QtCore/qstring.h>

using namespace Qt::StringLiterals;

// We can't make TimeType a Q_ENUM because QTimeZone is not a QObject, so:
const char *toString(QTimeZone::TimeType arg)
{
    return qstrdup([arg]() {
        switch (arg) {
        case QTimeZone::StandardTime: return "Standard Time";
        case QTimeZone::DaylightTime: return "Daylight-Saving Time";
        case QTimeZone::GenericTime: return "Generic Time";
        }
        Q_UNREACHABLE_RETURN("<unknown time type>");
    }());
}

class tst_QtParseTimeZone : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void prefix_data();
    void prefix();
};

void tst_QtParseTimeZone::prefix_data()
{
    using QTZ = QTimeZone;
    using Flag = QtTemporalPattern::TemporalFieldFlag;
    using Flags = QtTemporalPattern::TemporalFieldFlags;
    QTest::addColumn<QString>("text");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<Flags>("flags");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("until"); // 0 => fail to parse
    QTest::addColumn<QTZ::TimeType>("type");
    QTest::addColumn<QTZ>("zone");

    const QTZ lt(QTZ::LocalTime);
    using namespace QtParseTimeZone;

    // Mainly to check we don't crash or trigger assertions:
    QTest::addRow("null/C/none/0")
        << QString() << QLocale::c() << Flags{} << 0 << 0 << QTZ::GenericTime << lt;
    QTest::addRow("null/C/any/0")
        << QString() << QLocale::c() << AnyZoneForm << 0 << 0 << QTZ::GenericTime << lt;
    QTest::addRow("null/C/any/7")
        << QString() << QLocale::c() << AnyZoneForm << 7 << 0 << QTZ::GenericTime << lt;
    QTest::addRow("empty/C/none/0")
        << u""_s << QLocale::c() << Flags{} << 0 << 0 << QTZ::GenericTime << lt;
    QTest::addRow("empty/C/any/0")
        << u""_s << QLocale::c() << AnyZoneForm << 0 << 0 << QTZ::GenericTime << lt;
    QTest::addRow("empty/C/any/7")
        << u""_s << QLocale::c() << AnyZoneForm << 7 << 0 << QTZ::GenericTime << lt;

    // Local time's name as reported by (maybe QTZ::system() and) the native system:
    {
        const QDateTime now = QDateTime::currentDateTime(QTimeZone::LocalTime);
        // Dates unlikely to be when a zone has a transition and likely to differ as to DST
        const QDateTime jan = QDateTime(QDate(now.date().year(), 1, 12), QTime(12, 0),
                                        QTimeZone::LocalTime);
        const QDateTime jul = QDateTime(QDate(now.date().year(), 7, 12), QTime(12, 0),
                                        QTimeZone::LocalTime);
        QTZ::TimeType janType = QTZ::GenericTime, julType = QTZ::GenericTime;
        if (jan.isDaylightTime()) {
            janType = QTZ::DaylightTime;
            julType = jul.isDaylightTime() ? QTZ::DaylightTime : QTZ::StandardTime;
        } else if (jul.isDaylightTime()) {
            janType = QTZ::StandardTime;
            julType = QTZ::DaylightTime;
        } // If neither is DST, both are generic
#ifdef QT_BUILD_INTERNAL
        constexpr QDateTimePrivate::TransitionOptions
            legacy = QDateTimePrivate::GapUseAfter | QDateTimePrivate::FoldUseBefore;
        const QString janLoc = QLocalTime::localTimeAbbreviationAt(jan.toMSecsSinceEpoch(), legacy);
        if (!janLoc.isEmpty()) {
            QTest::addRow("jan-LocalTime/C/local/varies")
                << janLoc << QLocale::c() << Flags{ Flag::LocalTimeName }
                << 0 << janLoc.size() << janType << lt;
        }
        const QString julLoc = QLocalTime::localTimeAbbreviationAt(jul.toMSecsSinceEpoch(), legacy);
        if (!julLoc.isEmpty() && julLoc != janLoc) {
            QTest::addRow("jul-LocalTime/C/local/varies")
                << julLoc << QLocale::c() << Flags{ Flag::LocalTimeName }
                << 0 << julLoc.size() << julType << lt;
        }
#endif
#if QT_CONFIG(timezone)
        const auto sys = QTimeZone::systemTimeZone();
        const QString janSys = sys.abbreviation(jan);
        if (!janSys.isEmpty()) {
            QTest::addRow("jan-SystemZone/C/local/varies")
                << janSys << QLocale::c() << Flags{ Flag::LocalTimeName }
                << 0 << janSys.size() << janType << lt;
        }
        const QString julSys = sys.abbreviation(jul);
        if (!julSys.isEmpty() && julSys != janSys) {
            QTest::addRow("jul-SystemZone/C/local/varies")
                << julSys << QLocale::c() << Flags{ Flag::LocalTimeName }
                << 0 << julSys.size() << julType << lt;
        }
#endif // timezone backends
    }
}

void tst_QtParseTimeZone::prefix()
{
    QFETCH(const QString, text);
    QFETCH(const QLocale, locale);
    QFETCH(const QtTemporalPattern::TemporalFieldFlags, flags);
    QFETCH(const int, from);
    QFETCH(const int, until);
    QFETCH(const QTimeZone::TimeType, type);
    QFETCH(const QTimeZone, zone);

    const auto parsed = QtParseTimeZone::prefix(text, locale, from, flags);
    auto report = qScopeGuard([text, from, until, zone, parsed] {
        QStringView view{text};
        view = view.left(until).mid(from);
        if (parsed.isEmpty()) {
            qDebug() << "Found no zone in" << view << "within" << text;
        } else {
            const QtParseTimeZone::ParsedZone &parse = parsed.front();
            qDebug() << "Given" << view << "within" << text << "found"
                     << parse.startIndex << parse.endIndex << parse.timeType << parse.zone
                     << "expecting" << zone;
        }
    });

    if (until) {
        QVERIFY(!parsed.isEmpty());
        // There may be later alternative parses, but normally the first is used:
        const QtParseTimeZone::ParsedZone &parse = parsed.front();
        QCOMPARE(parse.startIndex, from);
        QCOMPARE(parse.endIndex, until);
        if ((parse.timeType == QTimeZone::GenericTime && type == QTimeZone::StandardTime)
            || (parse.timeType == QTimeZone::StandardTime && type == QTimeZone::GenericTime)) {
            // Generic and standard names commonly coincide, which can lead to
            // either being recognized as the other. Sources also vary, so some
            // may use as generic name what others use as standard name.
        } else {
            QCOMPARE(parse.timeType, type);
        }

        // Some zones may be mapped to the canonical zone for a metazone sharing
        // the name we used for them:
        if (parse.zone != zone && !parse.zone.hasAlternativeName(zone.id())) {
            // For one zone's name to be misparsed as another, they must have a
            // name in common:
            const auto sharedName = [zone, got=parse.zone, locale]() {
                using QTZ = QTimeZone;
                const auto namesMatch = [](QStringView one, QStringView two) {
                    // Accept UTC and GMT as aliases for one another.
                    // e.g. "Universal/ff-Adlm-GN/lng/gen/any/0" gets GMT in place of UTC
                    if ((one.startsWith(u"UTC") && two.startsWith(u"GMT"))
                        || (one.startsWith(u"GMT") && two.startsWith(u"UTC"))) {
                        return one.sliced(3) == two.sliced(3);
                    }
                    return one == two;
                };
                const auto sameName = [zone, got, locale, namesMatch]
                    (QTZ::NameType name, QTZ::TimeType season) {
                    return namesMatch(zone.displayName(season, name, locale),
                                      got.displayName(season, name, locale));
                };
                constexpr QTZ::NameType nameTypes[] = {
                    QTZ::LongName, QTZ::ShortName, QTZ::OffsetName,
                };
                constexpr QTZ::TimeType timeTypes[] = {
                    QTZ::GenericTime, QTZ::StandardTime, QTZ::DaylightTime
                };
                for (auto season : timeTypes) {
                    for (auto name : nameTypes) {
                        if (sameName(name, season))
                            return true;
                    }
                }
                return false;
            };
            if (!sharedName()) // Repeat the zone comparison to fail:
                QCOMPARE(parse.zone, zone);
        }
    } else {
        QVERIFY(parsed.isEmpty());
    }
    report.dismiss();
}

QTEST_APPLESS_MAIN(tst_QtParseTimeZone)

#include "tst_qtparsetimezone.moc"
