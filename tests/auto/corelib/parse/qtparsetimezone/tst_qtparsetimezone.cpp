// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QTest>
#include <private/qtparsetimezone_p.h>

#include <QtCore/private/qdatetime_p.h>
#include <QtCore/qlocale.h>
#include <QtCore/private/qlocale_p.h>
#include <QtCore/private/qlocaltime_p.h>
#include <QtCore/qstring.h>

using namespace Qt::StringLiterals;

// We can't make TimeType a Q_ENUM because QDateTimePrivate is not a QObject, so:
const char *toString(QDateTimePrivate::DaylightStatus arg)
{
    return qstrdup([arg]() {
        switch (arg) {
        case QDateTimePrivate::StandardTime: return "Standard Time";
        case QDateTimePrivate::DaylightTime: return "Daylight-Saving Time";
        case QDateTimePrivate::UnknownDaylightTime: return "Generic Time";
        }
        Q_UNREACHABLE_RETURN("<unknown time type>");
    }());
}

#if QT_CONFIG(timezone)
QDateTimePrivate::DaylightStatus timeTypeToStatus(QTimeZone::TimeType type) {
    using QDTP = QDateTimePrivate;
    switch (type) {
    case QTimeZone::GenericTime: return QDTP::UnknownDaylightTime;
    case QTimeZone::StandardTime: return QDTP::StandardTime;
    case QTimeZone::DaylightTime: return QDTP::DaylightTime;
    }
    Q_UNREACHABLE_RETURN(QDTP::UnknownDaylightTime);
}
#endif

class tst_QtParseTimeZone : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void prefix_data();
    void prefix();
};

void tst_QtParseTimeZone::prefix_data()
{
    using QDTP = QDateTimePrivate;
    using QTZ = QTimeZone;
    using Flag = QtTemporalPattern::TemporalFieldFlag;
    using Flags = QtTemporalPattern::TemporalFieldFlags;
    QTest::addColumn<QString>("text");
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<Flags>("flags");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("until"); // 0 => fail to parse
    QTest::addColumn<QDTP::DaylightStatus>("type");
    QTest::addColumn<QTZ>("zone");

    const QTZ lt(QTZ::LocalTime);
    using namespace QtParseTimeZone;

    // Mainly to check we don't crash or trigger assertions:
    QTest::addRow("null/C/none/0")
        << QString() << QLocale::c() << Flags{} << 0 << 0 << QDTP::UnknownDaylightTime << lt;
    QTest::addRow("null/C/any/0")
        << QString() << QLocale::c() << AnyZoneForm << 0 << 0 << QDTP::UnknownDaylightTime << lt;
    QTest::addRow("null/C/any/7")
        << QString() << QLocale::c() << AnyZoneForm << 7 << 0 << QDTP::UnknownDaylightTime << lt;
    QTest::addRow("empty/C/none/0")
        << u""_s << QLocale::c() << Flags{} << 0 << 0 << QDTP::UnknownDaylightTime << lt;
    QTest::addRow("empty/C/any/0")
        << u""_s << QLocale::c() << AnyZoneForm << 0 << 0 << QDTP::UnknownDaylightTime << lt;
    QTest::addRow("empty/C/any/7")
        << u""_s << QLocale::c() << AnyZoneForm << 7 << 0 << QDTP::UnknownDaylightTime << lt;

    // Local time's name as reported by (maybe QTZ::system() and) the native system:
    {
        const QDateTime now = QDateTime::currentDateTime(QTimeZone::LocalTime);
        // Dates unlikely to be when a zone has a transition and likely to differ as to DST
        const QDateTime jan = QDateTime(QDate(now.date().year(), 1, 12), QTime(12, 0),
                                        QTimeZone::LocalTime);
        const QDateTime jul = QDateTime(QDate(now.date().year(), 7, 12), QTime(12, 0),
                                        QTimeZone::LocalTime);
        QDTP::DaylightStatus janType = QDTP::UnknownDaylightTime;
        QDTP::DaylightStatus julType = QDTP::UnknownDaylightTime;
        if (jan.isDaylightTime()) {
            janType = QDTP::DaylightTime;
            julType = jul.isDaylightTime() ? QDTP::DaylightTime : QDTP::StandardTime;
        } else if (jul.isDaylightTime()) {
            janType = QDTP::StandardTime;
            julType = QDTP::DaylightTime;
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

    // Variations on (locale-independent) ISO offset format:
    QTest::addRow("+0100/C/ISO-pfx+num+wide/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO-pfx+num+wide+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO-pfx+num+shrt/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO-pfx+num+shrt+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO-pfx+num+abbr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO-pfx+num+narr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+num+wide/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+num+wide+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO+num+shrt/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+num+shrt+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO+num+abbr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+num+narr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+pfx+num+abbr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO+pfx+num+narr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+0100/C/ISO-pfx+num+abbr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+num+wide+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+num+wide/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+num+shrt+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+num+shrt/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+num+abbr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+num+narr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+num+wide/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+num+wide+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+num+shrt/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+num+shrt+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+num+abbr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+num+narr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+01:00/C/ISO-pfx+num+wide/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+num+wide+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO-pfx+num+shrt/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+num+shrt+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO-pfx+num+abbr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+num+abbr+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO-pfx+num+narr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+num+wide/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+num+wide+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO+num+shrt/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+num+shrt+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO+num+abbr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+num+abbr+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO+num+narr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+pfx+num+narr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    QTest::addRow("UTC+01:00/C/ISO-pfx+num+narr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+num+wide/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+num+wide+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+num+shrt/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+num+shrt+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+num+abbr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+num+abbr+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+num+narr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Numeric | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+wide/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+wide+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+shrt/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+shrt+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+abbr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+abbr+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+pfx+num+narr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO-pfx+num+narr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    // Seconds field present:
    QTest::addRow("+010000/C/ISO-pfx+num+wide/0")
        << u"+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 7 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+010000/C/ISO-pfx+num+shrt/0")
        << u"+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 7 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+010000/C/ISO-pfx+num+abbr/0")
        << u"+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+010000/C/ISO-pfx+num+narr/0")
        << u"+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+010000/C/ISO+pfx+num+wide/0")
        << u"UTC+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 10 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+010000/C/ISO+pfx+num+shrt/0")
        << u"UTC+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 10 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+010000/C/ISO+pfx+num+abbr/0")
        << u"UTC+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+010000/C/ISO+pfx+num+narr/0")
        << u"UTC+010000"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    // Partial-width minutes should be ignored (when allowed) as cruft:
    QTest::addRow("+017/C/ISO-pfx+num+wide/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+017/C/ISO-pfx+num+wide+0pad/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+017/C/ISO-pfx+num+shrt/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+017/C/ISO-pfx+num+shrt+0pad/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+017/C/ISO-pfx+num+abbr/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+017/C/ISO-pfx+num+abbr+0pad/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+017/C/ISO-pfx+num+narr/0")
        << u"+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+017/C/ISO+pfx+num+wide/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+017/C/ISO+pfx+num+wide+0pad/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+017/C/ISO+pfx+num+short/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+017/C/ISO+pfx+num+shrt+0pad/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+017/C/ISO+pfx+num+abbr/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+017/C/ISO+pfx+num+abbr+0pad/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Numeric | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+017/C/ISO+pfx+num+narr/0")
        << u"UTC+017"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Numeric | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    // And now all the same, but expecting colons (Verbal in place of Numeric):
    QTest::addRow("+0100/C/ISO-pfx+verb+wide/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO-pfx+verb+wide+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO-pfx+verb+shrt/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO-pfx+verb+shrt+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO-pfx+verb+abbr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO-pfx+verb+abbr+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO-pfx+verb+narr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+0100/C/ISO+verb+wide/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+verb+wide+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO+verb+shrt/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+verb+shrt+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO+verb+abbr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+0100/C/ISO+verb+abbr+0pad/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+0100/C/ISO+verb+narr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+0100/C/ISO+pfx+verb+narr/0")
        << u"+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO-pfx+verb+narr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    QTest::addRow("UTC+0100/C/ISO+verb+wide/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+verb+wide+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+verb+shrt/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+verb+shrt+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+verb+abbr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+verb+abbr+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+verb+narr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+0100/C/ISO+pfx+verb+wide/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+verb+wide+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+pfx+verb+shrt/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+verb+shrt+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+pfx+verb+abbr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+0100/C/ISO+pfx+verb+abbr+0pad/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+0100/C/ISO+pfx+verb+narr/0")
        << u"UTC+0100"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+01:00/C/ISO-pfx+verb+wide/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+verb+wide+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO-pfx+verb+shrt/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+verb+shrt+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO-pfx+verb+abbr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+verb+abbr+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+verb+narr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO-pfx+verb+narr+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Narrow | Flag::ZeroPad }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+1:00/C/ISO-pfx+verb+wide/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00/C/ISO-pfx+verb+wide+0pad/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+1:00/C/ISO-pfx+verb+shrt/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00/C/ISO-pfx+verb+shrt+0pad/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+1:00/C/ISO-pfx+verb+abbr/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00/C/ISO-pfx+verb+abbr+0pad/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+1:00/C/ISO-pfx+verb+narr/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 2 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00/C/ISO-pfx+verb+narr+0pad/0")
        << u"+1:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Narrow | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    QTest::addRow("+01:00/C/ISO+verb+wide/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+verb+wide+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO+verb+shrt/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+verb+shrt+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:00/C/ISO+verb+abbr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+verb+abbr+0pad/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00/C/ISO+verb+narr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+01:00/C/ISO+pfx+verb+narr/0")
        << u"+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO-pfx+verb+narr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    QTest::addRow("UTC+01:00/C/ISO+verb+wide/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+verb+wide+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+verb+shrt/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+verb+shrt+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+verb+abbr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+verb+abbr+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+verb+narr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::Verbal | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+wide/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+wide+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+shrt/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+shrt+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+abbr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+abbr+0pad/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00/C/ISO+pfx+verb+narr/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    // Seconds field present:
    QTest::addRow("+01:00:00/C/ISO-pfx+verb+wide/0")
        << u"+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00:00/C/ISO-pfx+verb+short/0")
        << u"+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00:00/C/ISO-pfx+verb+abbr/0")
        << u"+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:00:00/C/ISO-pfx+verb+narr/0")
        << u"+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("+1:00:00/C/ISO-pfx+verb+wide/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+wide+0pad/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+short/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+short+0pad/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
            | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+abbr/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+abbr+0pad/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+narr/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 2 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+1:00:00/C/ISO-pfx+verb+narr+0pad/0")
        << u"+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Narrow | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    QTest::addRow("UTC+01:00:00/C/ISO+pfx+verb+wide/0")
        << u"UTC+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 12 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00:00/C/ISO+pfx+verb+shrt/0")
        << u"UTC+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 12 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00:00/C/ISO+pfx+verb+abbr/0")
        << u"UTC+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:00:00/C/ISO+pfx+verb+narr/0")
        << u"UTC+01:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+wide/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 11 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+wide+0pad/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                 | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+shrt/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 11 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+shrt+0pad/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                 | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+abbr/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 8 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+abbr+0pad/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                 | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+narr/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 5 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+1:00:00/C/ISO+pfx+verb+narr+0pad/0")
        << u"UTC+1:00:00"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Narrow | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();

    // Partial-width minutes should be ignored (when allowed) as cruft:
    QTest::addRow("+01:7/C/ISO-pfx+verb+wide/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Wide }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:7/C/ISO-pfx+verb+wide+0pad/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Wide | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:7/C/ISO-pfx+verb+shrt/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:7/C/ISO-pfx+verb+shrt+0pad/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:7/C/ISO-pfx+verb+abbr/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("+01:7/C/ISO-pfx+verb+abbr+0pad/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("+01:7/C/ISO-pfx+verb+narr/0")
        << u"+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::NeedNoUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 3 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    QTest::addRow("UTC+01:7/C/ISO+pfx+verb+shrt/0")
        << u"UTC+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Short }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:7/C/ISO+pfx+verb+shrt+0pad/0")
        << u"UTC+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Short | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:7/C/ISO+pfx+verb+abbr/0")
        << u"UTC+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Abbreviated }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);
    QTest::addRow("UTC+01:7/C/ISO+pfx+verb+abbr+0pad/0")
        << u"UTC+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix
                | Flag::Verbal | Flag::Abbreviated | Flag::ZeroPad }
        << 0 << 0 << QDTP::UnknownDaylightTime << QTimeZone();
    QTest::addRow("UTC+01:7/C/ISO+pfx+verb+narr/0")
        << u"UTC+01:7"_s << QLocale::c()
        << Flags{ Flag::Iso8601 | Flag::AcceptUtcPrefix | Flag::Verbal | Flag::Narrow }
        << 0 << 6 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    // Some UTC tests, with offsets.
    QTest::addRow("UTC+01:00/C/num+wide/0")
        << u"UTC+01:00"_s << QLocale::c()
        << Flags{ Flag::Numeric | Flag::Wide }
        << 0 << 9 << QDTP::UnknownDaylightTime << QTimeZone(3600);

    // The only locale (in CLDR v48) that doesn't use a separator:
    const QLocale am_ET(QLocale::Amharic, QLocale::Ethiopia);
    {
        // What CLDR says we should be using:
        const QString correct = u"\u1302 \u12a4\u121d \u1272+0300"_s;
#if QT_CONFIG(icu) || defined(Q_OS_DARWIN)
        // We haven't found an ICU C API to localize offset locale names properly :-(
        const QString formatted = QTimeZone(3600 * 3).displayName(QTimeZone::GenericTime,
                                                                  QTimeZone::LongName, am_ET);
        // UTC+03:00; this is wrong.
        if (formatted == correct) // Notify if that ever gets fixed:
            qInfo("We can now remove the special case for ICU on am_ET prefix tests");
#else
        const QString formatted = correct;
#endif
        QTest::addRow("UTC+0300/am_ET/num+wide/0")
            << formatted << am_ET << Flags{ Flag::Numeric | Flag::Wide }
            << 0 << formatted.size() << QDTP::UnknownDaylightTime << QTimeZone(3600 * 3);
        QTest::addRow("UTC+0300/am_ET/num+wide/11") // Date of Haile Selassie's murder:
            << u"1975-08-27 " + formatted << am_ET << Flags{ Flag::Numeric | Flag::Wide }
            << 11 << 11 + formatted.size() << QDTP::UnknownDaylightTime << QTimeZone(3600 * 3);
    }

#if QT_CONFIG(timezone) // Basic well-known zones in relevant locales:
    // First the GMT-based ones, in order of simplicity:
    if (QTZ gmt("GMT"); gmt.isValid()) {
        const QLocale enGB(QLocale::English, QLocale::UnitedKingdom);
        QTest::addRow("GMT/en/any/0")
            << u"GMT"_s << enGB
            << AnyGlobalZoneForm << 0 << 3 << QDTP::UnknownDaylightTime << gmt;
        QTest::addRow("16:47 GMT/en/any/6")
            << u"16:47 GMT in the UK"_s << enGB
            << AnyGlobalZoneForm << 6 << 9 << QDTP::UnknownDaylightTime << gmt;
    } else {
        qDebug("Skipping GMT tests, not recognised by system backend");
    }
    // Skip Etc/GMT-with-offset as they do weird things; treat as unsupported.

    const auto zoneTests = [](const QTimeZone &zone, const QLocale &locale,
                              QTimeZone::TimeType season,
                              QStringView prefix = {}, QStringView suffix = {}) {
        constexpr QTZ::NameType nameTypes[] = {
            QTZ::LongName,
#if 0 // If we ever support abbreviation-parsing ...
            QTZ::ShortName,
#endif
            QTZ::OffsetName,
        };
        const auto nameTypeName = [](QTZ::NameType type) {
            switch (type) {
            case QTZ::DefaultName: return "dfl"; // (not used)
            case QTZ::LongName: return "lng";
            case QTZ::ShortName: return "srt"; // (not currently active)
            case QTZ::OffsetName: return "off";
            }
            Q_UNREACHABLE_RETURN("");
        };
        constexpr QTZ::TimeType timeTypes[] = {
            QTZ::GenericTime, QTZ::StandardTime, QTZ::DaylightTime
        };
        const auto timeTypeName = [](QTZ::TimeType type) {
            switch (type) {
            case QTZ::StandardTime: return "std";
            case QTZ::DaylightTime: return "dst";
            case QTZ::GenericTime: return "gen";
            }
            Q_UNREACHABLE_RETURN("");
        };
        const auto matchesOffset = [zone, locale](QStringView lhs, QTZ::TimeType type) {
            const QString offsetName = zone.displayName(type, QTZ::OffsetName, locale);
            QStringView rhs{offsetName};
            if (lhs == rhs)
                return true;
            const auto crossCheck = [lhs, rhs](QLatin1StringView one, QLatin1StringView two) {
                const qsizetype len = one.size();
                Q_ASSERT(len == two.size());
                if (!lhs.startsWith(one) || !rhs.startsWith(two))
                    return false;
                if (len == 3)
                    return true;
                return lhs.sliced(len) == rhs.sliced(len);
            };
            return crossCheck("UTC"_L1, "GMT"_L1) || crossCheck("GMT"_L1, "UTC"_L1);
        };
        const QByteArray loc = QLocalePrivate::get(locale)->m_data->id().name();
        const QByteArray zoneId = zone.id();
        QByteArrayView zid{zoneId}; // Find last /-component:
        for (qsizetype cut; (cut = zid.indexOf('/')) >= 0 && cut + 1 < zid.size();)
            zid = zid.sliced(cut + 1);

        for (const QTZ::NameType nameType : nameTypes) {
            QString generic;
            for (const QTZ::TimeType timeType : timeTypes) {
                if (timeType == QTZ::DaylightTime && !zone.hasDaylightTime())
                    continue;
                QString name = zone.displayName(timeType, nameType, locale);
                if (name.isEmpty())
                    continue;
                const QTimeZone expect = [timeType, nameType, zone, name, matchesOffset]() {
                    // Use zone unless name is or looks like an offset name:
                    if (nameType != QTZ::OffsetName && !matchesOffset(name, timeType))
                        return zone;
                    // Otherwise, expect the relevant offset-zone instead:
                    const QDateTime noon(QDate::currentDate(), QTime(12, 0), zone);
                    if (noon.date().year() == 2026 && zone.id() == "America/Vancouver") {
                        // IANA DB revision 2026b: British Columbia changes its
                        // standard time offset to its former DST as it exits
                        // DST for the last time. This confuses the GenericTime
                        // test, so use the year-end standard time for it:
                        if (timeType == QTZ::GenericTime)
                            return QTZ(zone.standardTimeOffset(QDateTime(QDate(2026, 12, 1),
                                                                         QTime(12, 0), zone)));
                    }
                    if (timeType != QTZ::DaylightTime)
                        return QTZ(zone.standardTimeOffset(noon));
                    const QDateTime june(QDate(noon.date().year(), 6, 21), QTime(12, 0), zone);
                    if (zone.daylightTimeOffset(june))
                        return QTZ(june.offsetFromUtc());
                    const QDateTime yule(QDate(noon.date().year(), 12, 21), QTime(12, 0), zone);
                    if (zone.daylightTimeOffset(yule))
                        return QTZ(yule.offsetFromUtc());
                    return QTZ(); // Abandon test case.
                }();
                if (!expect.isValid())
                    continue;
                if (timeType == QTZ::GenericTime)
                    generic = name;
                else if (name == generic)
                    continue;
                QDTP::DaylightStatus expectType =
                    zone.id() == expect.id() && nameType != QTZ::OffsetName
                    ? timeTypeToStatus(timeType) : QDTP::UnknownDaylightTime;

                QTest::addRow("%s/%s/%s/%s/any/0", zid.data(), loc.data(),
                              nameTypeName(nameType), timeTypeName(timeType))
                    << name << locale << AnyGlobalZoneForm << 0
                    << name.size() << expectType << expect;
                if (timeType == season && !prefix.isEmpty()) {
                    const int inset = int(prefix.size());
                    QTest::addRow("%s/%s/%s/%s/any/%d", zid.data(), loc.data(),
                                  nameTypeName(nameType), timeTypeName(timeType), inset)
                        << (prefix + name + suffix) << locale << AnyGlobalZoneForm << inset
                        << inset + name.size() << expectType << expect;
                }
            }
        }

        constexpr Flags IanaForm = Flag::Standalone | Flag::Short;
        const QString ianaStr = QLatin1String(zoneId);
        QTest::addRow("%s/%s/any/0", zoneId.data(), loc.data())
            << ianaStr << locale << AnyGlobalZoneForm << 0
            << int(zoneId.size()) << QDTP::UnknownDaylightTime << zone;
        QTest::addRow("%s%s/iana/0", zoneId.data(), loc.data())
            << ianaStr << locale << IanaForm << 0
            << int(zoneId.size()) << QDTP::UnknownDaylightTime << zone;
        if (!prefix.isEmpty()) {
            const int inset = int(prefix.size());
            const int until = inset + int(zoneId.size());
            const QString padded = prefix + ianaStr + suffix;
            QTest::addRow("%s/%s/any/%d", zoneId.data(), loc.data(), inset)
                << padded << locale << AnyGlobalZoneForm << inset
                << until << QDTP::UnknownDaylightTime << zone;
            QTest::addRow("%s%s/iana/%d", zoneId.data(), loc.data(), inset)
                << padded << locale << IanaForm << inset
                << until << QDTP::UnknownDaylightTime << zone;
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
#if QT_CONFIG(icu) || defined(Q_OS_DARWIN)
        // ICU uses Isla de Pascua in displayName()s.
        // Darwin uses ICU.
#else
        // qDebug() << ahu.displayName(QTZ::GenericTime, QTZ::LongName, esCL);
        // Actual ahu.displayName()s use "Easter" where these use "Isla de Pascua"
        // (This probably means QTZLocale is missing (at least) this translation.)
        QTest::addRow("EasterIslandT/es-CL/any/0")
            << u"hora de Isla de Pascua"_s
            << esCL << AnyGlobalZoneForm << 0 << 22 << QDTP::UnknownDaylightTime << ahu;
        QTest::addRow("EasterIslandST/es-CL/any/0")
            << u"hora estándar de Isla de Pascua"_s
            << esCL << AnyGlobalZoneForm << 0 << 31 << QDTP::StandardTime << ahu;
        // Zone has no actual DST, so hits a fall-back.
        QTest::addRow("EasterIslandDST/es-CL/any/0")
            << u"hora de verano de Easter"_s
            << esCL << AnyGlobalZoneForm << 0 << 24 << QDTP::DaylightTime << ahu;
#endif
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
    // Should be matched by the UTC backend.
    if (const QTZ west("UTC-02:00"); west.isValid()) {
        const QLocale nuuk(QLocale::Kalaallisut, QLocale::Greenland);
        QTest::addRow("UTC-02:00/kl-GL/any/0")
            << u"UTC-02:00"_s << nuuk << AnyGlobalZoneForm << 0
            << 9 << QDTP::UnknownDaylightTime << west;
        QTest::addRow("GMT-02:00/kl-GL/any/0")
            << u"GMT-02:00"_s << nuuk << AnyGlobalZoneForm << 0
            << 9 << QDTP::UnknownDaylightTime << west;
    } else {
        qDebug("Skipping UTC-02:00 tests, not recognised by UTC-offset backend :-(");
    }
    if (const QTZ east("UTC+02:00"); east.isValid()) {
        const QLocale sudan(QLocale::Arabic, QLocale::Sudan);
        // ICU uses this offset form also for ar-SD long and offset forms:
        QTest::addRow("[short]/ar-SD/any/all")
            << u"UTC+02:00"_s << sudan
            << AnyGlobalZoneForm << 0 << 9 << QDTP::UnknownDaylightTime << east;
#if !QT_CONFIG(icu) && !defined(Q_OS_DARWIN) // Darwin uses ICU
        QTest::addRow("[long]/ar-SD/any/all")
            << u"\u063a\u0631\u064a\u0646\u062a\u0634+\u0662"_s << sudan
            << AnyGlobalZoneForm << 0 << 8 << QDTP::UnknownDaylightTime << east;
        QTest::addRow("[offset]/ar-SD/any/all")
            << u"\u063a\u0631\u064a\u0646\u062a\u0634+\u0660\u0662:\u0660\u0660"_s << sudan
            << AnyGlobalZoneForm << 0 << 12 << QDTP::UnknownDaylightTime << east;
#endif
    } else {
        qDebug("Skipping UTC+02:00 tests, not recognised by UTC-offset backend :-(");
    }
}

void tst_QtParseTimeZone::prefix()
{
    QFETCH(const QString, text);
    QFETCH(const QLocale, locale);
    QFETCH(const QtTemporalPattern::TemporalFieldFlags, flags);
    QFETCH(const int, from);
    QFETCH(const int, until);
    QFETCH(const QDateTimePrivate::DaylightStatus, type);
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
        if ((parse.timeType == QDateTimePrivate::UnknownDaylightTime
             && type == QDateTimePrivate::StandardTime)
            || (parse.timeType == QDateTimePrivate::StandardTime
                && type == QDateTimePrivate::UnknownDaylightTime)) {
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
