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

#if QT_CONFIG(timezone) // Basic well-known zones in relevant locales:
    // First the GMT-based ones, in order of simplicity:
    if (QTZ gmt("GMT"); gmt.isValid()) {
        const QLocale enGB(QLocale::English, QLocale::UnitedKingdom);
        QTest::addRow("GMT/en/any/0")
            << u"GMT"_s << enGB
            << AnyGlobalZoneForm << 0 << 3 << QTZ::GenericTime << gmt;
        QTest::addRow("16:47 GMT/en/any/6")
            << u"16:47 GMT"_s << enGB
            << AnyGlobalZoneForm << 6 << 9 << QTZ::GenericTime << gmt;
    } else {
        qDebug("Skipping GMT tests, not recognised by system backend");
    }
    // Skip Etc/GMT-with-offset as they do weird things; treat as unsupported.

    const auto zoneTests = [](const QTimeZone &zone, const QLocale &locale,
                              QTimeZone::TimeType,
                              QStringView prefix = {}, QStringView suffix = {}) {
        using QTZ = QTimeZone;
        const QByteArray loc = QLocalePrivate::get(locale)->m_data->id().name();
        const QByteArray zoneId = zone.id();

        constexpr Flags IanaForm = Flag::Standalone | Flag::Short;
        const QString ianaStr = QLatin1String(zoneId);
        QTest::addRow("%s/%s/any/0", zoneId.data(), loc.data())
            << ianaStr << locale << AnyGlobalZoneForm << 0
            << int(zoneId.size()) << QTZ::GenericTime << zone;
        QTest::addRow("%s%s/iana/0", zoneId.data(), loc.data())
            << ianaStr << locale << IanaForm << 0
            << int(zoneId.size()) << QTZ::GenericTime << zone;
        if (!prefix.isEmpty()) {
            const int inset = int(prefix.size());
            const int until = inset + int(zoneId.size());
            const QString padded = prefix + ianaStr + suffix;
            QTest::addRow("%s/%s/any/%d", zoneId.data(), loc.data(), inset)
                << padded << locale << AnyGlobalZoneForm << inset
                << until << QTZ::GenericTime << zone;
            QTest::addRow("%s%s/iana/%d", zoneId.data(), loc.data(), inset)
                << padded << locale << IanaForm << inset
                << until << QTZ::GenericTime << zone;
        }
    };

    // Everything else used in tst_QDateTime, in alphabetic order
    if (const QTZ alaska("America/Anchorage"); alaska.isValid()) {
        // America/Metlakatla is also used, but essentially synonymous:
        zoneTests(alaska, QLocale(QLocale::English),
                  QTZ::StandardTime, u"1867-10-18 14:31:37 ", u" (Gregorian)");
    } else {
        qDebug("Skipping America/Anchorage tests, not recognised by system backend");
    }

    if (const QTZ ny("America/New_York"); ny.isValid()) {
        zoneTests(ny, QLocale(QLocale::English),
                  QTZ::DaylightTime, u"1664-08-27 ", u" (treachery)");
    } else {
        qDebug("Skipping America/New_York tests, not recognised by system backend");
    }

    if (const QTZ southBrazil("America/Sao_Paulo"); southBrazil.isValid()) {
        zoneTests(southBrazil, QLocale(QLocale::Portuguese, QLocale::Brazil),
                  QTZ::GenericTime, u"2005-10-16 00:30:00 ", u" (in a spring forward gap)");
    } else {
        qDebug("Skipping America/Sao_Paulo tests, not recognised by system backend");
    }

    if (const QTZ eastern("America/Toronto"); eastern.isValid()) {
        zoneTests(eastern, QLocale(QLocale::English, QLocale::Canada),
                  QTZ::GenericTime, u"2015-03-08 02:30 ", u" (in a spring forward gap)");
    } else {
        qDebug("Skipping America/Toronto tests (ET), not recognised by system backend");
    }

    if (const QTZ pst("America/Vancouver"); pst.isValid()) {
        zoneTests(pst, QLocale(QLocale::English, QLocale::Canada),
                  QTZ::GenericTime, u"2015-03-08 02:30 ", u" (in a spring forward gap)");
    } else {
        qDebug("Skipping America/Vancouver tests (PST), not recognised by system backend");
    }

    if (const QTZ phillip("Asia/Manila"); phillip.isValid()) {
        zoneTests(phillip, QLocale(QLocale::Filipino, QLocale::Philippines),
                  QTZ::GenericTime, u"1844-12-31 12:00 ", u" (noon on a skipped day)");
    } else {
        qDebug("Skipping Asia/Manila tests, not recognised by system backend");
    }

    if (const QTZ sgt("Asia/Singapore"); sgt.isValid()) {
        zoneTests(sgt, QLocale(QLocale::English, QLocale::Singapore),
                  QTZ::GenericTime, u"1982-01-01 00:15 ", u" (in a transition gap)");
    } else {
        qDebug("Skipping Asia/Singapore tests, not recognised by system backend");
    }

    if (const QTZ aest("Australia/Brisbane"); aest.isValid()) {
        // Australia/Sydney also appears, but is the same zone in modern times.
        // Contrast Australia/NSW (below) which calls itself Sydney Time.
        zoneTests(aest, QLocale(QLocale::English, QLocale::Australia),
                  QTZ::GenericTime, u"2012-06-01 02:15:30 ");
    } else {
        qDebug("Skipping Australia/Brisbane tests, not recognised by system backend");
    }

    if (const QTZ cet("Europe/Berlin"); cet.isValid()) {
        zoneTests(cet, QLocale(QLocale::German),
                  QTZ::GenericTime, u"1970-01-01 01:00", u" (epoch)");
    } else {
        qDebug("Skipping Europe/Berlin tests (CET), not recognised by system backend");
    }

    if (const QTZ eet("Europe/Helsinki"); eet.isValid()) {
        zoneTests(eet, QLocale(QLocale::Finnish),
                  QTZ::GenericTime, u"210501 001006 ", u" (in gap: end of LMT)");
    } else {
        qDebug("Skipping Europe/Helsinki tests (EET), not recognised by system backend");
    }

    if (const QTZ wet("Europe/Lisbon"); wet.isValid()) {
        zoneTests(wet, QLocale(QLocale::Portuguese, QLocale::Portugal),
                  QTZ::GenericTime, u"2015-03-29 01:30 ", u" (in spring-forward gap)");
    } else {
        qDebug("Skipping Europe/Lisbon tests (WET), not recognised by system backend");
    }

    if (const QTZ cet("Europe/Oslo"); cet.isValid()) {
        zoneTests(cet, QLocale(QLocale::NorwegianBokmal),
                  QTZ::GenericTime, u"1940-04-09 04:21 ", u" (Oscarsborg)");
    } else {
        qDebug("Skipping Europe/Oslo tests (CET), not recognised by system backend");
    }

    if (const QTZ rome("Europe/Rome"); rome.isValid())
        zoneTests(rome, QLocale(QLocale::Italian), QTZ::GenericTime, u"1970-01-01 12:00");
    else
        qDebug("Skipping Europe/Rome tests, not recognised by system backend");

    if (const QTZ nz("Pacific/Auckland"); nz.isValid()) {
        zoneTests(nz, QLocale(QLocale::English, QLocale::NewZealand),
                  QTZ::GenericTime, u"1840-02-06 ", u" (Te Tiriti o Waitangi)");
    } else {
        qDebug("Skipping Pacific/Auckland tests, not recognised by system backend");
    }

    if (const QTZ lint("Pacific/Kiritimati"); lint.isValid()) {
        // No gil-KI in CLDR, so use en-KI.
        zoneTests(lint, QLocale(QLocale::English, QLocale::Kiribati),
                  QTZ::GenericTime, u"1994-12-31 12:00 ", u" (skipped day)");
        // No DST.
    } else {
        qDebug("Skipping Pacific/Kiritimati tests, not recognised by system backend");
    }

    // Zones used by tst_QTimeZoneBackend:
    if (const QTZ wet("Africa/Casablanca"); wet.isValid()) { // lng/gen/any/{0,16} get Accra
        // ar_MA (little zgh_MA in CLDR)
        zoneTests(wet, QLocale(QLocale::Arabic, QLocale::Morocco),
                  QTZ::GenericTime, u"1942-11-08 dawn ", u" (Operation Torch)");
        // (It was actually on permanent DST on that date, but we're not parsing
        // the datetime - it's just there as dangling cruft - so that's beside
        // the point, here.)
    } else {
        qDebug("Skipping Africa/Casablanca tests, not recognised by system backend");
    }

    if (const QTZ lagos("Africa/Lagos"); lagos.isValid()) {
        zoneTests(lagos, QLocale(QLocale::Hausa, QLocale::Nigeria),
                  QTZ::GenericTime, u"1908-07-01 00:00 ", u" (revert from GMT to LMT)");
        // No DST.
    } else {
        qDebug("Skipping Africa/Lagos tests, not recognised by system backend");
    }

    if (const QTZ tunis("Africa/Tunis"); tunis.isValid()) {
        zoneTests(tunis, QLocale(QLocale::Arabic, QLocale::Tunisia),
                  QTZ::GenericTime, u"2008-10-26 03:00 ", u" (final escape from DST)");
    } else {
        qDebug("Skipping Africa/Tunis tests, not recognised by system backend");
    }

    if (const QTZ vet("America/Caracas"); vet.isValid()) {
        zoneTests(vet, QLocale(QLocale::Spanish, QLocale::Venezuela),
                  QTZ::GenericTime, u"2016-05-01 02:45 ", u" (in skipped half-hour)");
        // No DST
    } else {
        qDebug("Skipping America/Caracas tests, not recognised by system backend");
    }

#if 0 // Permanent DST leads to its standard time long name being mistaken for Chile DST
    if (const QTZ chile("America/Coyhaique"); chile.isValid()) {
        zoneTests(chile, QLocale(QLocale::Spanish, QLocale::Chile),
                  QTZ::GenericTime, u"2025-03-20 01:00 ", u" (or maybe 00:00; escaped DST)");
    } else {
        qDebug("Skipping America/Coyhaique tests, not recognised by system backend");
    }
#endif

    if (const QTZ tell("America/Indiana/Tell_City"); tell.isValid()) {
        zoneTests(tell, QLocale(QLocale::English),
                  QTZ::GenericTime, u"2006-04-02 02:00 ", u" (EST \u2192 CDT)");
    } else {
        qDebug("Skipping America/Indiana/Tell_City tests, not recognised by system backend");
    }

    if (const QTZ cst("America/Managua"); cst.isValid()) {
        zoneTests(cst, QLocale(QLocale::Spanish, QLocale::Nicaragua),
                  QTZ::GenericTime, u"2006-10-01 01:00 ", u" (escaped DST)");
    } else {
        qDebug("Skipping America/Managua tests, not recognised by system backend");
    }

    if (const QTZ thai("Asia/Bangkok"); thai.isValid()) {
        zoneTests(thai, QLocale(QLocale::Thai),
                  QTZ::GenericTime, u"1920-04-01 00:00 ", u" (end LMT)");
        // No DST
    } else {
        qDebug("Skipping Asia/Bangkok tests, not recognised by system backend");
    }

    if (const QTZ sri("Asia/Colombo"); sri.isValid()) {
        zoneTests(sri, QLocale(QLocale::Sinhala, QLocale::SriLanka),
                  QTZ::GenericTime, u"2006-04-15 00:30 ", u" (rejoined IST)");
        zoneTests(sri, QLocale(QLocale::Tamil, QLocale::SriLanka),
                  QTZ::GenericTime, u"1996-05-25 00:00 ", u" (split from IST)");
    } else {
        qDebug("Skipping Asia/Colombo tests, not recognised by system backend");
    }

    if (const QTZ nipon("Asia/Tokyo"); nipon.isValid()) {
        zoneTests(nipon, QLocale(QLocale::Japanese),
                  QTZ::GenericTime, u"1951-09-08 02:00 ", u" (escaped DST)");
    } else {
        qDebug("Skipping Asia/Tokyo tests, not recognised by system backend");
    }

    if (const QTZ ast("Atlantic/Bermuda"); ast.isValid()) {
        zoneTests(ast, QLocale(QLocale::English, QLocale::Bermuda),
                  QTZ::GenericTime, u"1974-04-28 02:00 ", u" (revert to DST)");
    } else {
        qDebug("Skipping Atlantic/Bermuda tests, not recognised by system backend");
    }

    if (const QTZ tors("Atlantic/Faroe"); tors.isValid()) {
        zoneTests(tors, QLocale(QLocale::Faroese),
                  QTZ::GenericTime, u"1981-03-29 01:00 ", u" (fell for DST)");
    } else {
        qDebug("Skipping Atlantic/Faroe tests, not recognised by system backend");
    }

    if (const QTZ wet("Atlantic/Madeira"); wet.isValid()) {
        zoneTests(wet, QLocale(QLocale::Portuguese, QLocale::Portugal),
                  QTZ::GenericTime, u"1976-09-26 01:00 ", u" (joined WEST)");
    } else {
        qDebug("Skipping Atlantic/Madeira tests, not recognised by system backend");
    }

    if (const QTZ act("Australia/Broken_Hill"); act.isValid()) {
        zoneTests(act, QLocale(QLocale::English, QLocale::Australia),
                  QTZ::GenericTime, u"1971-10-31 02:00 ", u" (fell for DST)");
    } else {
        qDebug("Skipping Australia/Broken_Hill tests, not recognised by system backend");
    }

    if (const QTZ nsw("Australia/NSW"); nsw.isValid()) {// Alias for Australia/Sydney
        zoneTests(nsw, QLocale(QLocale::English, QLocale::Australia),
                  QTZ::GenericTime, u"1942-01-01 02:00 ", u" (war-time DST)");
    } else {
        qDebug("Skipping Australia/NSW tests, not recognised by system backend");
    }

    if (const QTZ tas("Australia/Tasmania"); tas.isValid()) { // Alias for Australia/Hobart
        zoneTests(tas, QLocale(QLocale::English, QLocale::Australia),
                  QTZ::GenericTime, u"1967-10-01 02:00 ", u" (fell for DST)");
    } else {
        qDebug("Skipping Australia/Tasmania tests, not recognised by system backend");
    }

    if (const QTZ acre("Brazil/Acre"); acre.isValid()) { // Alias for America/Rio_Branco
        zoneTests(acre, QLocale(QLocale::Portuguese, QLocale::Brazil),
                  QTZ::GenericTime, u"2013-11-10 00:00 ", u" (split from Amazon Time)");
    } else {
        qDebug("Skipping Brazil/Acre tests, not recognised by system backend");
    }

    if (const QTZ ast("Canada/Atlantic"); ast.isValid()) { // Alias for America/Halifax
        zoneTests(ast, QLocale(QLocale::French, QLocale::Canada),
                  QTZ::GenericTime, u"1974-04-28 02:00 ", u" (sync into Canada Time)");
    } else {
        qDebug("Skipping Canada/Atlantic tests, not recognised by system backend");
    }

    if (const QTZ ahu("Chile/EasterIsland"); ahu.isValid()) { // Alias for Pacific/Easter
        const QLocale esCL(QLocale::Spanish, QLocale::Chile); // (No CLDR data for Rapa Nui)
        zoneTests(ahu, esCL, QTZ::GenericTime, u"1990-03-11 ");
    } else {
        qDebug("Skipping Chile/EasterIsland tests, not recognised by system backend");
    }

    if (const QTZ cst("CST6CDT"); cst.isValid()) { // Alias for America/Chicago
        zoneTests(cst, QLocale(QLocale::Spanish, QLocale::Mexico),
                  QTZ::GenericTime, u"1942-02-09 02:00 ", u" (wartime no-DST)");
    } else {
        qDebug("Skipping CST6CDT tests, not recognised by system backend");
    }

    if (const QTZ egmt("Etc/Greenwich"); egmt.isValid()) { // Alias for Etc/GMT
        zoneTests(egmt, QLocale(QLocale::English, QLocale::UnitedKingdom), QTZ::GenericTime);
        // No DST.
    } else {
        qDebug("Skipping Etc/Greenwich tests, not recognised by system backend");
    }

    if (const QTZ univ("Etc/Universal"); univ.isValid()) { // Alias for Etc/UTC
        zoneTests(univ, QLocale(QLocale::Fulah, QLocale::AdlamScript), QTZ::GenericTime);
        // No DST.
    } else {
        qDebug("Skipping Etc/Universal tests, not recognised by system backend");
    }

    if (const QTZ sark("Europe/Guernsey"); sark.isValid()) {
        zoneTests(sark, QLocale(QLocale::English, QLocale::UnitedKingdom),
                  QTZ::GenericTime, u"1945-05-09 ", u" (Liberation Day)");
    } else {
        qDebug("Skipping Europe/Guernsey tests, not recognised by system backend");
    }

    if (const QTZ eet("Europe/Kaliningrad"); eet.isValid()) {
        zoneTests(eet, QLocale(QLocale::Russian),
                  QTZ::GenericTime, u"1945-04-09 18:00 ", u" (siege ended)");
    } else {
        qDebug("Skipping Europe/Kaliningrad tests, not recognised by system backend");
    }

    if (const QTZ kyiv("Europe/Kyiv"); kyiv.isValid()) {
        zoneTests(kyiv, QLocale(QLocale::Ukrainian),
                  QTZ::GenericTime, u"1990-07-01 02:00 ", u" (switch to EET)");
    } else {
        qDebug("Skipping Europe/Kyiv tests, not recognised by system backend");
    }

    if (const QTZ czek("Europe/Prague"); czek.isValid()) {
        zoneTests(czek, QLocale(QLocale::Czech),
                  QTZ::GenericTime, u"1942-11-02 03:00 ", u" (back to DST)");
    } else {
        qDebug("Skipping Europe/Prague tests, not recognised by system backend");
    }

    if (const QTZ pope("Europe/Vatican"); pope.isValid()) {
        // Alias for Europe/Rome
        // la_VA lacks zone data
        zoneTests(pope, QLocale(QLocale::Italian, QLocale::VaticanCity),
                  QTZ::GenericTime, u"1929-02-11 ", u" (Lateran Treaty)");
    } else {
        qDebug("Skipping Europe/Vatican tests, not recognised by system backend");
    }

    if (const QTZ eat("Indian/Comoro"); eat.isValid()) {
        zoneTests(eat, QLocale(QLocale::Arabic, QLocale::Comoros),
                  QTZ::GenericTime, u"1911-07-01 00:00", u" (LMT \u2192 EAT)");
        // No DST.
    } else {
        qDebug("Skipping Indian/Comoro tests, not recognised by system backend");
    }

    if (const QTZ sur("Mexico/BajaSur"); sur.isValid()) { // Alias for America/Mazatlan
        zoneTests(sur, QLocale(QLocale::Spanish, QLocale::Mexico),
                  QTZ::GenericTime, u"2022-10-30 02:00 ", u" (quit DST)");
    } else {
        qDebug("Skipping Mexico/BajaSur tests, not recognised by system backend");
    }

    if (const QTZ buka("Pacific/Bougainville"); buka.isValid()) {
        // No ho_PG data, no tpi zone data, so make do with English:
        zoneTests(buka, QLocale(QLocale::English, QLocale::PapuaNewGuinea),
                  QTZ::GenericTime, u"2014-12-28 00:00 ", u" (split from PGT)");
        // No DST.
    } else {
        qDebug("Skipping Pacific/Bougainville tests, not recognised by system backend");
    }

    if (const QTZ sst("Pacific/Midway"); sst.isValid()) {
        // Functionally an alias for Pacitic/Pago_Pago, which has no DST.
        zoneTests(sst, QLocale(QLocale::English, QLocale::UnitedStatesOutlyingIslands),
                  QTZ::GenericTime, u"1942-06-04 04:30 ", u" (a battle)");
        // sst.displayName(DST, Long, enUM) gets "American Samoa DaylightTime",
        // but apparently Pacific/Pago_pago's equivalent is empty - presumably
        // because it does no DST, although Midway also does no DST.
    } else {
        qDebug("Skipping Pacific/Midway tests, not recognised by system backend");
    }

    if (const QTZ wft("Pacific/Wallis"); wft.isValid()) {
        zoneTests(wft, QLocale(QLocale::French, QLocale::WallisAndFutuna),
                  QTZ::GenericTime, u"1942-05-27 ", u" (exit Vichy)");
        // No DST.
    } else {
        qDebug("Skipping Pacific/Wallis tests, not recognised by system backend");
    }

    if (const QTZ adak("US/Aleutian"); adak.isValid()) { // Alias for America/Adak
        zoneTests(adak, QLocale(QLocale::English),
                  QTZ::GenericTime, u"1967-04-01 00:00 ", u" (fell for DST)");
    } else {
        qDebug("Skipping US/Aleutian tests, not recognised by system backend");
    }

    // Some tests use "Vulcan/ShiKahr" as an invalid name; but it is indeed invalid.
#endif
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
#ifdef Q_OS_DARWIN
            QEXPECT_FAIL("Casablanca/ar-Arab-MA/lng/std/any/0",
                         "Best match to same name is Africa/Bissau DST, inconveniently.",
                         Abort);
#endif
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
