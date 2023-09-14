// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2019 Crimson AS <info@crimson.no>
// Copyright (C) 2013 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtimezone.h"
#include "qtimezoneprivate_p.h"
#include "private/qlocale_tools_p.h"
#include "private/qlocking_p.h"

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QCache>
#include <QtCore/QMap>
#include <QtCore/QMutex>

#include <qdebug.h>
#include <qplatformdefs.h>

#include <algorithm>
#include <errno.h>
#include <limits.h>
#ifndef Q_OS_INTEGRITY
#include <sys/param.h> // to use MAXSYMLINKS constant
#endif
#include <unistd.h>    // to use _SC_SYMLOOP_MAX constant

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(icu)
Q_CONSTINIT static QBasicMutex s_icu_mutex;
#endif

/*
    Private

    tz file implementation
*/

struct QTzTimeZone {
    QLocale::Territory territory = QLocale::AnyTerritory;
    QByteArray comment;
};

// Define as a type as Q_GLOBAL_STATIC doesn't like it
typedef QHash<QByteArray, QTzTimeZone> QTzTimeZoneHash;

static bool isTzFile(const QString &name);

// Open a named file under the zone info directory:
static bool openZoneInfo(QString name, QFile *file)
{
    // At least on Linux / glibc (see man 3 tzset), $TZDIR overrides the system
    // default location for zone info:
    const QString tzdir = qEnvironmentVariable("TZDIR");
    if (!tzdir.isEmpty()) {
        file->setFileName(QDir(tzdir).filePath(name));
        if (file->open(QIODevice::ReadOnly))
            return true;
    }
    // Try modern system path first:
    constexpr auto zoneShare = "/usr/share/zoneinfo/"_L1;
    if (tzdir != zoneShare && tzdir != zoneShare.chopped(1)) {
        file->setFileName(zoneShare + name);
        if (file->open(QIODevice::ReadOnly))
            return true;
    }
    // Fall back to legacy system path:
    constexpr auto zoneLib = "/usr/lib/zoneinfo/"_L1;
    if (tzdir != zoneLib && tzdir != zoneLib.chopped(1)) {
        file->setFileName(zoneShare + name);
        if (file->open(QIODevice::ReadOnly))
            return true;
    }
    return false;
}

// Parse zone.tab table for territory information, read directories to ensure we
// find all installed zones (many are omitted from zone.tab; even more from
// zone1970.tab).
static QTzTimeZoneHash loadTzTimeZones()
{
    QFile tzif;
    if (!openZoneInfo("zone.tab"_L1, &tzif))
        return QTzTimeZoneHash();

    QTzTimeZoneHash zonesHash;
    while (!tzif.atEnd()) {
        const QByteArray line = tzif.readLine().trimmed();
        if (line.isEmpty() || line.at(0) == '#') // Ignore empty or comment
            continue;
        // Data rows are tab-separated columns Region, Coordinates, ID, Optional Comments
        QByteArrayView text(line);
        int cut = text.indexOf('\t');
        if (Q_LIKELY(cut > 0)) {
            QTzTimeZone zone;
            // TODO: QLocale & friends could do this look-up without UTF8-conversion:
            zone.territory = QLocalePrivate::codeToTerritory(QString::fromUtf8(text.first(cut)));
            text = text.sliced(cut + 1);
            cut = text.indexOf('\t');
            if (Q_LIKELY(cut >= 0)) { // Skip over Coordinates, read ID and comment
                text = text.sliced(cut + 1);
                cut = text.indexOf('\t'); // < 0 if line has no comment
                if (Q_LIKELY(cut)) {
                    const QByteArray id = (cut > 0 ? text.first(cut) : text).toByteArray();
                    if (cut > 0)
                        zone.comment = text.sliced(cut + 1).toByteArray();
                    zonesHash.insert(id, zone);
                }
            }
        }
    }

    const QString path = tzif.fileName();
    const qsizetype cut = path.lastIndexOf(u'/');
    Q_ASSERT(cut > 0);
    const QDir zoneDir = QDir(path.first(cut));
    QDirIterator zoneFiles(zoneDir, QDirIterator::Subdirectories);
    while (zoneFiles.hasNext()) {
        const QFileInfo info = zoneFiles.nextFileInfo();
        if (!(info.isFile() || info.isSymLink()))
            continue;
        const QString name = zoneDir.relativeFilePath(info.filePath());
        // Two sub-directories containing (more or less) copies of the zoneinfo tree.
        if (info.isDir() ? name == "posix"_L1 || name == "right"_L1
            : name.startsWith("posix/"_L1) || name.startsWith("right/"_L1)) {
            continue;
        }
        // We could filter out *.* and leapseconds instead of doing the
        // isTzFile() check; in practice current (2023) zoneinfo/ contains only
        // actual zone files and matches to that filter.
        const QByteArray id = QFile::encodeName(name);
        if (!zonesHash.contains(id) && isTzFile(zoneDir.absoluteFilePath(name)))
            zonesHash.insert(id, QTzTimeZone());
    }
    return zonesHash;
}

// Hash of available system tz files as loaded by loadTzTimeZones()
Q_GLOBAL_STATIC(const QTzTimeZoneHash, tzZones, loadTzTimeZones());

/*
    The following is copied and modified from tzfile.h which is in the public domain.
    Copied as no compatibility guarantee and is never system installed.
    See https://github.com/eggert/tz/blob/master/tzfile.h
*/

#define TZ_MAGIC      "TZif"
#define TZ_MAX_TIMES  1200
#define TZ_MAX_TYPES   256  // Limited by what (unsigned char)'s can hold
#define TZ_MAX_CHARS    50  // Maximum number of abbreviation characters
#define TZ_MAX_LEAPS    50  // Maximum number of leap second corrections

struct QTzHeader {
    char       tzh_magic[4];        // TZ_MAGIC
    char       tzh_version;         // '\0' or '2' as of 2005
    char       tzh_reserved[15];    // reserved--must be zero
    quint32    tzh_ttisgmtcnt;      // number of trans. time flags
    quint32    tzh_ttisstdcnt;      // number of trans. time flags
    quint32    tzh_leapcnt;         // number of leap seconds
    quint32    tzh_timecnt;         // number of transition times
    quint32    tzh_typecnt;         // number of local time types
    quint32    tzh_charcnt;         // number of abbr. chars
};

struct QTzTransition {
    qint64 tz_time;     // Transition time
    quint8 tz_typeind;  // Type Index
};
Q_DECLARE_TYPEINFO(QTzTransition, Q_PRIMITIVE_TYPE);

struct QTzType {
    int tz_gmtoff;  // UTC offset in seconds
    bool   tz_isdst;   // Is DST
    quint8 tz_abbrind; // abbreviation list index
};
Q_DECLARE_TYPEINFO(QTzType, Q_PRIMITIVE_TYPE);

static bool isTzFile(const QString &name)
{
    QFile file(name);
    return file.open(QFile::ReadOnly) && file.read(strlen(TZ_MAGIC)) == TZ_MAGIC;
}

// TZ File parsing

static QTzHeader parseTzHeader(QDataStream &ds, bool *ok)
{
    QTzHeader hdr;
    quint8 ch;
    *ok = false;

    // Parse Magic, 4 bytes
    ds.readRawData(hdr.tzh_magic, 4);

    if (memcmp(hdr.tzh_magic, TZ_MAGIC, 4) != 0 || ds.status() != QDataStream::Ok)
        return hdr;

    // Parse Version, 1 byte, before 2005 was '\0', since 2005 a '2', since 2013 a '3'
    ds >> ch;
    hdr.tzh_version = ch;
    if (ds.status() != QDataStream::Ok
        || (hdr.tzh_version != '2' && hdr.tzh_version != '\0' && hdr.tzh_version != '3')) {
        return hdr;
    }

    // Parse reserved space, 15 bytes
    ds.readRawData(hdr.tzh_reserved, 15);
    if (ds.status() != QDataStream::Ok)
        return hdr;

    // Parse rest of header, 6 x 4-byte transition counts
    ds >> hdr.tzh_ttisgmtcnt >> hdr.tzh_ttisstdcnt >> hdr.tzh_leapcnt >> hdr.tzh_timecnt
       >> hdr.tzh_typecnt >> hdr.tzh_charcnt;

    // Check defined maximums
    if (ds.status() != QDataStream::Ok
        || hdr.tzh_timecnt > TZ_MAX_TIMES
        || hdr.tzh_typecnt > TZ_MAX_TYPES
        || hdr.tzh_charcnt > TZ_MAX_CHARS
        || hdr.tzh_leapcnt > TZ_MAX_LEAPS
        || hdr.tzh_ttisgmtcnt > hdr.tzh_typecnt
        || hdr.tzh_ttisstdcnt > hdr.tzh_typecnt) {
        return hdr;
    }

    *ok = true;
    return hdr;
}

static QList<QTzTransition> parseTzTransitions(QDataStream &ds, int tzh_timecnt, bool longTran)
{
    QList<QTzTransition> transitions(tzh_timecnt);

    if (longTran) {
        // Parse tzh_timecnt x 8-byte transition times
        for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
            ds >> transitions[i].tz_time;
            if (ds.status() != QDataStream::Ok)
                transitions.resize(i);
        }
    } else {
        // Parse tzh_timecnt x 4-byte transition times
        qint32 val;
        for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
            ds >> val;
            transitions[i].tz_time = val;
            if (ds.status() != QDataStream::Ok)
                transitions.resize(i);
        }
    }

    // Parse tzh_timecnt x 1-byte transition type index
    for (int i = 0; i < tzh_timecnt && ds.status() == QDataStream::Ok; ++i) {
        quint8 typeind;
        ds >> typeind;
        if (ds.status() == QDataStream::Ok)
            transitions[i].tz_typeind = typeind;
    }

    return transitions;
}

static QList<QTzType> parseTzTypes(QDataStream &ds, int tzh_typecnt)
{
    QList<QTzType> types(tzh_typecnt);

    // Parse tzh_typecnt x transition types
    for (int i = 0; i < tzh_typecnt && ds.status() == QDataStream::Ok; ++i) {
        QTzType &type = types[i];
        // Parse UTC Offset, 4 bytes
        ds >> type.tz_gmtoff;
        // Parse Is DST flag, 1 byte
        if (ds.status() == QDataStream::Ok)
            ds >> type.tz_isdst;
        // Parse Abbreviation Array Index, 1 byte
        if (ds.status() == QDataStream::Ok)
            ds >> type.tz_abbrind;
        if (ds.status() != QDataStream::Ok)
            types.resize(i);
    }

    return types;
}

static QMap<int, QByteArray> parseTzAbbreviations(QDataStream &ds, int tzh_charcnt, const QList<QTzType> &types)
{
    // Parse the abbreviation list which is tzh_charcnt long with '\0' separated strings. The
    // QTzType.tz_abbrind index points to the first char of the abbreviation in the array, not the
    // occurrence in the list. It can also point to a partial string so we need to use the actual typeList
    // index values when parsing.  By using a map with tz_abbrind as ordered key we get both index
    // methods in one data structure and can convert the types afterwards.
    QMap<int, QByteArray> map;
    quint8 ch;
    QByteArray input;
    // First parse the full abbrev string
    for (int i = 0; i < tzh_charcnt && ds.status() == QDataStream::Ok; ++i) {
        ds >> ch;
        if (ds.status() == QDataStream::Ok)
            input.append(char(ch));
        else
            return map;
    }
    // Then extract all the substrings pointed to by types
    for (const QTzType &type : types) {
        QByteArray abbrev;
        for (int i = type.tz_abbrind; input.at(i) != '\0'; ++i)
            abbrev.append(input.at(i));
        // Have reached end of an abbreviation, so add to map
        map[type.tz_abbrind] = abbrev;
    }
    return map;
}

static void parseTzLeapSeconds(QDataStream &ds, int tzh_leapcnt, bool longTran)
{
    // Parse tzh_leapcnt x pairs of leap seconds
    // We don't use leap seconds, so only read and don't store
    qint32 val;
    if (longTran) {
        // v2 file format, each entry is 12 bytes long
        qint64 time;
        for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
            // Parse Leap Occurrence Time, 8 bytes
            ds >> time;
            // Parse Leap Seconds To Apply, 4 bytes
            if (ds.status() == QDataStream::Ok)
                ds >> val;
        }
    } else {
        // v0 file format, each entry is 8 bytes long
        for (int i = 0; i < tzh_leapcnt && ds.status() == QDataStream::Ok; ++i) {
            // Parse Leap Occurrence Time, 4 bytes
            ds >> val;
            // Parse Leap Seconds To Apply, 4 bytes
            if (ds.status() == QDataStream::Ok)
                ds >> val;
        }
    }
}

static QList<QTzType> parseTzIndicators(QDataStream &ds, const QList<QTzType> &types, int tzh_ttisstdcnt,
                                        int tzh_ttisgmtcnt)
{
    QList<QTzType> result = types;
    bool temp;
    /*
      Scan and discard indicators.

      These indicators are only of use (by the date program) when "handling
      POSIX-style time zone environment variables".  The flags here say whether
      the *specification* of the zone gave the time in UTC, local standard time
      or local wall time; but whatever was specified has been digested for us,
      already, by the zone-info compiler (zic), so that the tz_time values read
      from the file (by parseTzTransitions) are all in UTC.
     */

    // Scan tzh_ttisstdcnt x 1-byte standard/wall indicators
    for (int i = 0; i < tzh_ttisstdcnt && ds.status() == QDataStream::Ok; ++i)
        ds >> temp;

    // Scan tzh_ttisgmtcnt x 1-byte UTC/local indicators
    for (int i = 0; i < tzh_ttisgmtcnt && ds.status() == QDataStream::Ok; ++i)
        ds >> temp;

    return result;
}

static QByteArray parseTzPosixRule(QDataStream &ds)
{
    // Parse POSIX rule, variable length '\n' enclosed
    QByteArray rule;

    quint8 ch;
    ds >> ch;
    if (ch != '\n' || ds.status() != QDataStream::Ok)
        return rule;
    ds >> ch;
    while (ch != '\n' && ds.status() == QDataStream::Ok) {
        rule.append((char)ch);
        ds >> ch;
    }

    return rule;
}

static QDate calculateDowDate(int year, int month, int dayOfWeek, int week)
{
    if (dayOfWeek == 0) // Sunday; we represent it as 7, POSIX uses 0
        dayOfWeek = 7;
    else if (dayOfWeek & ~7 || month < 1 || month > 12 || week < 1 || week > 5)
        return QDate();

    QDate date(year, month, 1);
    int startDow = date.dayOfWeek();
    if (startDow <= dayOfWeek)
        date = date.addDays(dayOfWeek - startDow - 7);
    else
        date = date.addDays(dayOfWeek - startDow);
    date = date.addDays(week * 7);
    while (date.month() != month)
        date = date.addDays(-7);
    return date;
}

static QDate calculatePosixDate(const QByteArray &dateRule, int year)
{
    Q_ASSERT(!dateRule.isEmpty());
    bool ok;
    // Can start with M, J, or a digit
    if (dateRule.at(0) == 'M') {
        // nth week in month format "Mmonth.week.dow"
        QList<QByteArray> dateParts = dateRule.split('.');
        if (dateParts.size() > 2) {
            Q_ASSERT(!dateParts.at(0).isEmpty()); // the 'M' is its [0].
            int month = QByteArrayView{ dateParts.at(0) }.sliced(1).toInt(&ok);
            int week = ok ? dateParts.at(1).toInt(&ok) : 0;
            int dow = ok ? dateParts.at(2).toInt(&ok) : 0;
            if (ok)
                return calculateDowDate(year, month, dow, week);
        }
    } else if (dateRule.at(0) == 'J') {
        // Day of Year 1...365, ignores Feb 29.
        // So March always starts on day 60.
        int doy = QByteArrayView{ dateRule }.sliced(1).toInt(&ok);
        if (ok && doy > 0 && doy < 366) {
            // Subtract 1 because we're adding days *after* the first of
            // January, unless it's after February in a leap year, when the leap
            // day cancels that out:
            if (!QDate::isLeapYear(year) || doy < 60)
                --doy;
            return QDate(year, 1, 1).addDays(doy);
        }
    } else {
        // Day of Year 0...365, includes Feb 29
        int doy = dateRule.toInt(&ok);
        if (ok && doy >= 0 && doy < 366)
            return QDate(year, 1, 1).addDays(doy);
    }
    return QDate();
}

// returns the time in seconds, INT_MIN if we failed to parse
static int parsePosixTime(const char *begin, const char *end)
{
    // Format "hh[:mm[:ss]]"
    int hour, min = 0, sec = 0;

    const int maxHour = 137; // POSIX's extended range.
    auto r = qstrntoll(begin, end - begin, 10);
    hour = r.result;
    if (!r.ok() || hour < -maxHour || hour > maxHour || r.used > 2)
        return INT_MIN;
    begin += r.used;
    if (begin < end && *begin == ':') {
        // minutes
        ++begin;
        r = qstrntoll(begin, end - begin, 10);
        min = r.result;
        if (!r.ok() || min < 0 || min > 59 || r.used > 2)
            return INT_MIN;

        begin += r.used;
        if (begin < end && *begin == ':') {
            // seconds
            ++begin;
            r = qstrntoll(begin, end - begin, 10);
            sec = r.result;
            if (!r.ok() || sec < 0 || sec > 59 || r.used > 2)
                return INT_MIN;
            begin += r.used;
        }
    }

    // we must have consumed everything
    if (begin != end)
        return INT_MIN;

    return (hour * 60 + min) * 60 + sec;
}

static int parsePosixTransitionTime(const QByteArray &timeRule)
{
    return parsePosixTime(timeRule.constBegin(), timeRule.constEnd());
}

static int parsePosixOffset(const char *begin, const char *end)
{
    // Format "[+|-]hh[:mm[:ss]]"
    // note that the sign is inverted because POSIX counts in hours West of GMT
    bool negate = true;
    if (*begin == '+') {
        ++begin;
    } else if (*begin == '-') {
        negate = false;
        ++begin;
    }

    int value = parsePosixTime(begin, end);
    if (value == INT_MIN)
        return value;
    return negate ? -value : value;
}

static inline bool asciiIsLetter(char ch)
{
    ch |= 0x20; // lowercases if it is a letter, otherwise just corrupts ch
    return ch >= 'a' && ch <= 'z';
}

namespace {

struct PosixZone
{
    enum {
        InvalidOffset = INT_MIN,
    };

    QString name;
    int offset;

    static PosixZone invalid() { return {QString(), InvalidOffset}; }
    static PosixZone parse(const char *&pos, const char *end);

    bool hasValidOffset() const noexcept { return offset != InvalidOffset; }
};

} // unnamed namespace

// Returns the zone name, the offset (in seconds) and advances \a begin to
// where the parsing ended. Returns a zone of INT_MIN in case an offset
// couldn't be read.
PosixZone PosixZone::parse(const char *&pos, const char *end)
{
    static const char offsetChars[] = "0123456789:";

    const char *nameBegin = pos;
    const char *nameEnd;
    Q_ASSERT(pos < end);

    if (*pos == '<') {
        ++nameBegin;    // skip the '<'
        nameEnd = nameBegin;
        while (nameEnd < end && *nameEnd != '>') {
            // POSIX says only alphanumeric, but we allow anything
            ++nameEnd;
        }
        pos = nameEnd + 1;      // skip the '>'
    } else {
        nameEnd = nameBegin;
        while (nameEnd < end && asciiIsLetter(*nameEnd))
            ++nameEnd;
        pos = nameEnd;
    }
    if (nameEnd - nameBegin < 3)
        return invalid();  // name must be at least 3 characters long

    // zone offset, form [+-]hh:mm:ss
    const char *zoneBegin = pos;
    const char *zoneEnd = pos;
    if (zoneEnd < end && (zoneEnd[0] == '+' || zoneEnd[0] == '-'))
        ++zoneEnd;
    while (zoneEnd < end) {
        if (strchr(offsetChars, char(*zoneEnd)) == nullptr)
            break;
        ++zoneEnd;
    }

    QString name = QString::fromUtf8(nameBegin, nameEnd - nameBegin);
    const int offset = zoneEnd > zoneBegin ? parsePosixOffset(zoneBegin, zoneEnd) : InvalidOffset;
    pos = zoneEnd;
    // UTC+hh:mm:ss or GMT+hh:mm:ss should be read as offsets from UTC, not as a
    // POSIX rule naming a zone as UTC or GMT and specifying a non-zero offset.
    if (offset != 0 && (name =="UTC"_L1 || name == "GMT"_L1))
        return invalid();
    return {std::move(name), offset};
}

/* Parse and check a POSIX rule.

   By default a simple zone abbreviation with no offset information is accepted.
   Set \a requireOffset to \c true to require that there be offset data present.
*/
static auto validatePosixRule(const QByteArray &posixRule, bool requireOffset = false)
{
    // Format is described here:
    // http://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html
    // See also calculatePosixTransition()'s reference.
    const auto parts = posixRule.split(',');
    const struct { bool isValid, hasDst; } fail{false, false}, good{true, parts.size() > 1};
    const QByteArray &zoneinfo = parts.at(0);
    if (zoneinfo.isEmpty())
        return fail;

    const char *begin = zoneinfo.begin();
    {
        // Updates begin to point after the name and offset it parses:
        const auto posix = PosixZone::parse(begin, zoneinfo.end());
        if (posix.name.isEmpty())
            return fail;
        if (requireOffset && !posix.hasValidOffset())
            return fail;
    }

    if (good.hasDst) {
        if (begin >= zoneinfo.end())
            return fail;
        // Expect a second name (and optional offset) after the first:
        if (PosixZone::parse(begin, zoneinfo.end()).name.isEmpty())
            return fail;
    }
    if (begin < zoneinfo.end())
        return fail;

    if (good.hasDst) {
        if (parts.size() != 3 || parts.at(1).isEmpty() || parts.at(2).isEmpty())
            return fail;
        for (int i = 1; i < 3; ++i) {
            const auto tran = parts.at(i).split('/');
            if (!calculatePosixDate(tran.at(0), 1972).isValid())
                return fail;
            if (tran.size() > 1) {
                const auto time = tran.at(1);
                if (parsePosixTime(time.begin(), time.end()) == INT_MIN)
                    return fail;
            }
        }
    }
    return good;
}

static QList<QTimeZonePrivate::Data> calculatePosixTransitions(const QByteArray &posixRule,
                                                               int startYear, int endYear,
                                                               qint64 lastTranMSecs)
{
    QList<QTimeZonePrivate::Data> result;

    // POSIX Format is like "TZ=CST6CDT,M3.2.0/2:00:00,M11.1.0/2:00:00"
    // i.e. "std offset dst [offset],start[/time],end[/time]"
    // See the section about TZ at
    // http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html
    // and the link in validatePosixRule(), above.
    QList<QByteArray> parts = posixRule.split(',');

    PosixZone stdZone, dstZone = PosixZone::invalid();
    {
        const QByteArray &zoneinfo = parts.at(0);
        const char *begin = zoneinfo.constBegin();

        stdZone = PosixZone::parse(begin, zoneinfo.constEnd());
        if (!stdZone.hasValidOffset()) {
            stdZone.offset = 0;     // reset to UTC if we failed to parse
        } else if (begin < zoneinfo.constEnd()) {
            dstZone = PosixZone::parse(begin, zoneinfo.constEnd());
            if (!dstZone.hasValidOffset()) {
                // if the dst offset isn't provided, it is 1 hour ahead of the standard offset
                dstZone.offset = stdZone.offset + (60 * 60);
            }
        }
    }

    // If only the name part, or no DST specified, then no transitions
    if (parts.size() == 1 || !dstZone.hasValidOffset()) {
        QTimeZonePrivate::Data data;
        data.atMSecsSinceEpoch = lastTranMSecs;
        data.offsetFromUtc = stdZone.offset;
        data.standardTimeOffset = stdZone.offset;
        data.daylightTimeOffset = 0;
        data.abbreviation = stdZone.name.isEmpty() ? QString::fromUtf8(parts.at(0)) : stdZone.name;
        result << data;
        return result;
    }
    if (parts.size() < 3 || parts.at(1).isEmpty() || parts.at(2).isEmpty())
        return result; // Malformed.

    // Get the std to dst transition details
    const int twoOClock = 7200; // Default transition time, when none specified
    const auto dstParts = parts.at(1).split('/');
    const QByteArray dstDateRule = dstParts.at(0);
    const int dstTime = dstParts.size() < 2 ? twoOClock : parsePosixTransitionTime(dstParts.at(1));

    // Get the dst to std transition details
    const auto stdParts = parts.at(2).split('/');
    const QByteArray stdDateRule = stdParts.at(0);
    const int stdTime = stdParts.size() < 2 ? twoOClock : parsePosixTransitionTime(stdParts.at(1));

    if (dstDateRule.isEmpty() || stdDateRule.isEmpty() || dstTime == INT_MIN || stdTime == INT_MIN)
        return result; // Malformed.

    // Limit year to the range QDateTime can represent:
    const int minYear = int(QDateTime::YearRange::First);
    const int maxYear = int(QDateTime::YearRange::Last);
    startYear = qBound(minYear, startYear, maxYear);
    endYear = qBound(minYear, endYear, maxYear);
    Q_ASSERT(startYear <= endYear);

    for (int year = startYear; year <= endYear; ++year) {
        // Note: std and dst, despite being QDateTime(,, UTC), have the
        // date() and time() of the *zone*'s description of the transition
        // moments; the atMSecsSinceEpoch values computed from them are
        // correctly offse to be UTC-based.

        QTimeZonePrivate::Data dstData; // Transition to DST
        QDateTime dst(calculatePosixDate(dstDateRule, year)
                      .startOfDay(QTimeZone::UTC).addSecs(dstTime));
        dstData.atMSecsSinceEpoch = dst.toMSecsSinceEpoch() - stdZone.offset * 1000;
        dstData.offsetFromUtc = dstZone.offset;
        dstData.standardTimeOffset = stdZone.offset;
        dstData.daylightTimeOffset = dstZone.offset - stdZone.offset;
        dstData.abbreviation = dstZone.name;
        QTimeZonePrivate::Data stdData; // Transition to standard time
        QDateTime std(calculatePosixDate(stdDateRule, year)
                      .startOfDay(QTimeZone::UTC).addSecs(stdTime));
        stdData.atMSecsSinceEpoch = std.toMSecsSinceEpoch() - dstZone.offset * 1000;
        stdData.offsetFromUtc = stdZone.offset;
        stdData.standardTimeOffset = stdZone.offset;
        stdData.daylightTimeOffset = 0;
        stdData.abbreviation = stdZone.name;

        if (year == startYear) {
            // Handle the special case of fixed state, which may be represented
            // by fake transitions at start and end of each year:
            if (dstData.atMSecsSinceEpoch < stdData.atMSecsSinceEpoch) {
                if (dst <= QDate(year, 1, 1).startOfDay(QTimeZone::UTC)
                    && std >= QDate(year, 12, 31).endOfDay(QTimeZone::UTC)) {
                    // Permanent DST:
                    dstData.atMSecsSinceEpoch = lastTranMSecs;
                    result << dstData;
                    return result;
                }
            } else {
                if (std <= QDate(year, 1, 1).startOfDay(QTimeZone::UTC)
                    && dst >= QDate(year, 12, 31).endOfDay(QTimeZone::UTC)) {
                    // Permanent Standard time, perversely described:
                    stdData.atMSecsSinceEpoch = lastTranMSecs;
                    result << stdData;
                    return result;
                }
            }
        }

        const bool useStd = std.isValid() && std.date().year() == year && !stdZone.name.isEmpty();
        const bool useDst = dst.isValid() && dst.date().year() == year && !dstZone.name.isEmpty();
        if (useStd && useDst) {
            if (dst < std)
                result << dstData << stdData;
            else
                result << stdData << dstData;
        } else if (useStd) {
            result << stdData;
        } else if (useDst) {
            result << dstData;
        }
    }
    return result;
}

// Create the system default time zone
QTzTimeZonePrivate::QTzTimeZonePrivate()
    : QTzTimeZonePrivate(staticSystemTimeZoneId())
{
}

QTzTimeZonePrivate::~QTzTimeZonePrivate()
{
}

QTzTimeZonePrivate *QTzTimeZonePrivate::clone() const
{
#if QT_CONFIG(icu)
    const auto lock = qt_scoped_lock(s_icu_mutex);
#endif
    return new QTzTimeZonePrivate(*this);
}

class QTzTimeZoneCache
{
public:
    QTzTimeZoneCacheEntry fetchEntry(const QByteArray &ianaId);

private:
    QTzTimeZoneCacheEntry findEntry(const QByteArray &ianaId);
    QCache<QByteArray, QTzTimeZoneCacheEntry> m_cache;
    QMutex m_mutex;
};

QTzTimeZoneCacheEntry QTzTimeZoneCache::findEntry(const QByteArray &ianaId)
{
    QTzTimeZoneCacheEntry ret;
    QFile tzif;
    if (ianaId.isEmpty()) {
        // Open system tz
        tzif.setFileName(QStringLiteral("/etc/localtime"));
        if (!tzif.open(QIODevice::ReadOnly))
            return ret;
    } else if (!openZoneInfo(QString::fromLocal8Bit(ianaId), &tzif)) {
        // ianaId may be a POSIX rule, taken from $TZ or /etc/TZ
        auto check = validatePosixRule(ianaId);
        if (check.isValid) {
            ret.m_hasDst = check.hasDst;
            ret.m_posixRule = ianaId;
        }
        return ret;
    }

    QDataStream ds(&tzif);

    // Parse the old version block of data
    bool ok = false;
    QByteArray posixRule;
    QTzHeader hdr = parseTzHeader(ds, &ok);
    if (!ok || ds.status() != QDataStream::Ok)
        return ret;
    QList<QTzTransition> tranList = parseTzTransitions(ds, hdr.tzh_timecnt, false);
    if (ds.status() != QDataStream::Ok)
        return ret;
    QList<QTzType> typeList = parseTzTypes(ds, hdr.tzh_typecnt);
    if (ds.status() != QDataStream::Ok)
        return ret;
    QMap<int, QByteArray> abbrevMap = parseTzAbbreviations(ds, hdr.tzh_charcnt, typeList);
    if (ds.status() != QDataStream::Ok)
        return ret;
    parseTzLeapSeconds(ds, hdr.tzh_leapcnt, false);
    if (ds.status() != QDataStream::Ok)
        return ret;
    typeList = parseTzIndicators(ds, typeList, hdr.tzh_ttisstdcnt, hdr.tzh_ttisgmtcnt);
    if (ds.status() != QDataStream::Ok)
        return ret;

    // If version 2 then parse the second block of data
    if (hdr.tzh_version == '2' || hdr.tzh_version == '3') {
        ok = false;
        QTzHeader hdr2 = parseTzHeader(ds, &ok);
        if (!ok || ds.status() != QDataStream::Ok)
            return ret;
        tranList = parseTzTransitions(ds, hdr2.tzh_timecnt, true);
        if (ds.status() != QDataStream::Ok)
            return ret;
        typeList = parseTzTypes(ds, hdr2.tzh_typecnt);
        if (ds.status() != QDataStream::Ok)
            return ret;
        abbrevMap = parseTzAbbreviations(ds, hdr2.tzh_charcnt, typeList);
        if (ds.status() != QDataStream::Ok)
            return ret;
        parseTzLeapSeconds(ds, hdr2.tzh_leapcnt, true);
        if (ds.status() != QDataStream::Ok)
            return ret;
        typeList = parseTzIndicators(ds, typeList, hdr2.tzh_ttisstdcnt, hdr2.tzh_ttisgmtcnt);
        if (ds.status() != QDataStream::Ok)
            return ret;
        posixRule = parseTzPosixRule(ds);
        if (ds.status() != QDataStream::Ok)
            return ret;
    }
    // Translate the TZ file's raw data into our internal form:

    if (!posixRule.isEmpty()) {
        auto check = validatePosixRule(posixRule);
        if (!check.isValid) // We got a POSIX rule, but it was malformed:
            return ret;
        ret.m_posixRule = posixRule;
        ret.m_hasDst = check.hasDst;
    }

    // Translate the array-index-based tz_abbrind into list index
    const int size = abbrevMap.size();
    ret.m_abbreviations.clear();
    ret.m_abbreviations.reserve(size);
    QList<int> abbrindList;
    abbrindList.reserve(size);
    for (auto it = abbrevMap.cbegin(), end = abbrevMap.cend(); it != end; ++it) {
        ret.m_abbreviations.append(it.value());
        abbrindList.append(it.key());
    }
    // Map tz_abbrind from map's keys (as initially read) to abbrindList's
    // indices (used hereafter):
    for (int i = 0; i < typeList.size(); ++i)
        typeList[i].tz_abbrind = abbrindList.indexOf(typeList.at(i).tz_abbrind);

    // TODO: is typeList[0] always the "before zones" data ? It seems to be ...
    if (typeList.size())
        ret.m_preZoneRule = { typeList.at(0).tz_gmtoff, 0, typeList.at(0).tz_abbrind };
    else
        ret.m_preZoneRule = { 0, 0, 0 };

    // Offsets are stored as total offset, want to know separate UTC and DST offsets
    // so find the first non-dst transition to use as base UTC Offset
    int utcOffset = ret.m_preZoneRule.stdOffset;
    for (const QTzTransition &tran : std::as_const(tranList)) {
        if (!typeList.at(tran.tz_typeind).tz_isdst) {
            utcOffset = typeList.at(tran.tz_typeind).tz_gmtoff;
            break;
        }
    }

    // Now for each transition time calculate and store our rule:
    const int tranCount = tranList.size();;
    ret.m_tranTimes.reserve(tranCount);
    // The DST offset when in effect: usually stable, usually an hour:
    int lastDstOff = 3600;
    for (int i = 0; i < tranCount; i++) {
        const QTzTransition &tz_tran = tranList.at(i);
        QTzTransitionTime tran;
        QTzTransitionRule rule;
        const QTzType tz_type = typeList.at(tz_tran.tz_typeind);

        // Calculate the associated Rule
        if (!tz_type.tz_isdst) {
            utcOffset = tz_type.tz_gmtoff;
        } else if (Q_UNLIKELY(tz_type.tz_gmtoff != utcOffset + lastDstOff)) {
            /*
              This might be a genuine change in DST offset, but could also be
              DST starting at the same time as the standard offset changed.  See
              if DST's end gives a more plausible utcOffset (i.e. one closer to
              the last we saw, or a simple whole hour):
            */
            // Standard offset inferred from net offset and expected DST offset:
            const int inferStd = tz_type.tz_gmtoff - lastDstOff; // != utcOffset
            for (int j = i + 1; j < tranCount; j++) {
                const QTzType new_type = typeList.at(tranList.at(j).tz_typeind);
                if (!new_type.tz_isdst) {
                    const int newUtc = new_type.tz_gmtoff;
                    if (newUtc == utcOffset) {
                        // DST-end can't help us, avoid lots of messy checks.
                    // else: See if the end matches the familiar DST offset:
                    } else if (newUtc == inferStd) {
                        utcOffset = newUtc;
                    // else: let either end shift us to one hour as DST offset:
                    } else if (tz_type.tz_gmtoff - 3600 == utcOffset) {
                        // Start does it
                    } else if (tz_type.tz_gmtoff - 3600 == newUtc) {
                        utcOffset = newUtc; // End does it
                    // else: prefer whichever end gives DST offset closer to
                    // last, but consider any offset > 0 "closer" than any <= 0:
                    } else if (newUtc < tz_type.tz_gmtoff
                               ? (utcOffset >= tz_type.tz_gmtoff
                                  || qAbs(newUtc - inferStd) < qAbs(utcOffset - inferStd))
                               : (utcOffset >= tz_type.tz_gmtoff
                                  && qAbs(newUtc - inferStd) < qAbs(utcOffset - inferStd))) {
                        utcOffset = newUtc;
                    }
                    break;
                }
            }
            lastDstOff = tz_type.tz_gmtoff - utcOffset;
        }
        rule.stdOffset = utcOffset;
        rule.dstOffset = tz_type.tz_gmtoff - utcOffset;
        rule.abbreviationIndex = tz_type.tz_abbrind;

        // If the rule already exist then use that, otherwise add it
        int ruleIndex = ret.m_tranRules.indexOf(rule);
        if (ruleIndex == -1) {
            if (rule.dstOffset != 0)
                ret.m_hasDst = true;
            tran.ruleIndex = ret.m_tranRules.size();
            ret.m_tranRules.append(rule);
        } else {
            tran.ruleIndex = ruleIndex;
        }

        tran.atMSecsSinceEpoch = tz_tran.tz_time * 1000;
        ret.m_tranTimes.append(tran);
    }

    return ret;
}

QTzTimeZoneCacheEntry QTzTimeZoneCache::fetchEntry(const QByteArray &ianaId)
{
    QMutexLocker locker(&m_mutex);

    // search the cache...
    QTzTimeZoneCacheEntry *obj = m_cache.object(ianaId);
    if (obj)
        return *obj;

    // ... or build a new entry from scratch
    QTzTimeZoneCacheEntry ret = findEntry(ianaId);
    m_cache.insert(ianaId, new QTzTimeZoneCacheEntry(ret));
    return ret;
}

// Create a named time zone
QTzTimeZonePrivate::QTzTimeZonePrivate(const QByteArray &ianaId)
{
    static QTzTimeZoneCache tzCache;
    auto entry = tzCache.fetchEntry(ianaId);
    if (entry.m_tranTimes.isEmpty() && entry.m_posixRule.isEmpty())
        return; // Invalid after all !

    cached_data = std::move(entry);
    m_id = ianaId;
    // Avoid empty ID, if we have an abbreviation to use instead
    if (m_id.isEmpty()) {
        // This can only happen for the system zone, when we've read the
        // contents of /etc/localtime because it wasn't a symlink.
#if QT_CONFIG(icu)
        // Use ICU's system zone, if only to avoid using the abbreviation as ID
        // (ICU might mis-recognize it) in displayName().
        m_icu = new QIcuTimeZonePrivate();
        // Use its ID, as an alternate source of data:
        m_id = m_icu->id();
        if (!m_id.isEmpty())
            return;
#endif
        m_id = abbreviation(QDateTime::currentMSecsSinceEpoch()).toUtf8();
    }
}

QLocale::Territory QTzTimeZonePrivate::territory() const
{
    return tzZones->value(m_id).territory;
}

QString QTzTimeZonePrivate::comment() const
{
    return QString::fromUtf8(tzZones->value(m_id).comment);
}

QString QTzTimeZonePrivate::displayName(qint64 atMSecsSinceEpoch,
                                        QTimeZone::NameType nameType,
                                        const QLocale &locale) const
{
#if QT_CONFIG(icu)
    auto lock = qt_unique_lock(s_icu_mutex);
    if (!m_icu)
        m_icu = new QIcuTimeZonePrivate(m_id);
    // TODO small risk may not match if tran times differ due to outdated files
    // TODO Some valid TZ names are not valid ICU names, use translation table?
    if (m_icu->isValid())
        return m_icu->displayName(atMSecsSinceEpoch, nameType, locale);
    lock.unlock();
#else
    Q_UNUSED(nameType);
    Q_UNUSED(locale);
#endif
    // Fall back to base-class:
    return QTimeZonePrivate::displayName(atMSecsSinceEpoch, nameType, locale);
}

QString QTzTimeZonePrivate::displayName(QTimeZone::TimeType timeType,
                                        QTimeZone::NameType nameType,
                                        const QLocale &locale) const
{
#if QT_CONFIG(icu)
    auto lock = qt_unique_lock(s_icu_mutex);
    if (!m_icu)
        m_icu = new QIcuTimeZonePrivate(m_id);
    // TODO small risk may not match if tran times differ due to outdated files
    // TODO Some valid TZ names are not valid ICU names, use translation table?
    if (m_icu->isValid())
        return m_icu->displayName(timeType, nameType, locale);
    lock.unlock();
#else
    Q_UNUSED(timeType);
    Q_UNUSED(nameType);
    Q_UNUSED(locale);
#endif
    // If no ICU available then have to use abbreviations instead
    // Abbreviations don't have GenericTime
    if (timeType == QTimeZone::GenericTime)
        timeType = QTimeZone::StandardTime;

    // Get current tran, if valid and is what we want, then use it
    const qint64 currentMSecs = QDateTime::currentMSecsSinceEpoch();
    QTimeZonePrivate::Data tran = data(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise get next tran and if valid and is what we want, then use it
    tran = nextTransition(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise get prev tran and if valid and is what we want, then use it
    tran = previousTransition(currentMSecs);
    if (tran.atMSecsSinceEpoch != invalidMSecs())
        tran = previousTransition(tran.atMSecsSinceEpoch);
    if (tran.atMSecsSinceEpoch != invalidMSecs()
        && ((timeType == QTimeZone::DaylightTime && tran.daylightTimeOffset != 0)
        || (timeType == QTimeZone::StandardTime && tran.daylightTimeOffset == 0))) {
        return tran.abbreviation;
    }

    // Otherwise is strange sequence, so work backwards through trans looking for first match, if any
    auto it = std::partition_point(tranCache().cbegin(), tranCache().cend(),
                                   [currentMSecs](const QTzTransitionTime &at) {
                                       return at.atMSecsSinceEpoch <= currentMSecs;
                                   });

    while (it != tranCache().cbegin()) {
        --it;
        tran = dataForTzTransition(*it);
        int offset = tran.daylightTimeOffset;
        if ((timeType == QTimeZone::DaylightTime) != (offset == 0))
            return tran.abbreviation;
    }

    // Otherwise if no match use current data
    return data(currentMSecs).abbreviation;
}

QString QTzTimeZonePrivate::abbreviation(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).abbreviation;
}

int QTzTimeZonePrivate::offsetFromUtc(qint64 atMSecsSinceEpoch) const
{
    const QTimeZonePrivate::Data tran = data(atMSecsSinceEpoch);
    return tran.offsetFromUtc; // == tran.standardTimeOffset + tran.daylightTimeOffset
}

int QTzTimeZonePrivate::standardTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).standardTimeOffset;
}

int QTzTimeZonePrivate::daylightTimeOffset(qint64 atMSecsSinceEpoch) const
{
    return data(atMSecsSinceEpoch).daylightTimeOffset;
}

bool QTzTimeZonePrivate::hasDaylightTime() const
{
    return cached_data.m_hasDst;
}

bool QTzTimeZonePrivate::isDaylightTime(qint64 atMSecsSinceEpoch) const
{
    return (daylightTimeOffset(atMSecsSinceEpoch) != 0);
}

QTimeZonePrivate::Data QTzTimeZonePrivate::dataForTzTransition(QTzTransitionTime tran) const
{
    return dataFromRule(cached_data.m_tranRules.at(tran.ruleIndex), tran.atMSecsSinceEpoch);
}

QTimeZonePrivate::Data QTzTimeZonePrivate::dataFromRule(QTzTransitionRule rule,
                                                        qint64 msecsSinceEpoch) const
{
    return { QString::fromUtf8(cached_data.m_abbreviations.at(rule.abbreviationIndex)),
             msecsSinceEpoch, rule.stdOffset + rule.dstOffset, rule.stdOffset, rule.dstOffset };
}

QList<QTimeZonePrivate::Data> QTzTimeZonePrivate::getPosixTransitions(qint64 msNear) const
{
    const int year = QDateTime::fromMSecsSinceEpoch(msNear, QTimeZone::UTC).date().year();
    // The Data::atMSecsSinceEpoch of the single entry if zone is constant:
    qint64 atTime = tranCache().isEmpty() ? msNear : tranCache().last().atMSecsSinceEpoch;
    return calculatePosixTransitions(cached_data.m_posixRule, year - 1, year + 1, atTime);
}

QTimeZonePrivate::Data QTzTimeZonePrivate::data(qint64 forMSecsSinceEpoch) const
{
    // If the required time is after the last transition (or there were none)
    // and we have a POSIX rule, then use it:
    if (!cached_data.m_posixRule.isEmpty()
        && (tranCache().isEmpty() || tranCache().last().atMSecsSinceEpoch < forMSecsSinceEpoch)) {
        QList<QTimeZonePrivate::Data> posixTrans = getPosixTransitions(forMSecsSinceEpoch);
        auto it = std::partition_point(posixTrans.cbegin(), posixTrans.cend(),
                                       [forMSecsSinceEpoch] (const QTimeZonePrivate::Data &at) {
                                           return at.atMSecsSinceEpoch <= forMSecsSinceEpoch;
                                       });
        // Use most recent, if any in the past; or the first if we have no other rules:
        if (it > posixTrans.cbegin() || (tranCache().isEmpty() && it < posixTrans.cend())) {
            QTimeZonePrivate::Data data = *(it > posixTrans.cbegin() ? it - 1 : it);
            data.atMSecsSinceEpoch = forMSecsSinceEpoch;
            return data;
        }
    }
    if (tranCache().isEmpty()) // Only possible if !isValid()
        return invalidData();

    // Otherwise, use the rule for the most recent or first transition:
    auto last = std::partition_point(tranCache().cbegin(), tranCache().cend(),
                                     [forMSecsSinceEpoch] (const QTzTransitionTime &at) {
                                         return at.atMSecsSinceEpoch <= forMSecsSinceEpoch;
                                     });
    if (last == tranCache().cbegin())
        return dataFromRule(cached_data.m_preZoneRule, forMSecsSinceEpoch);

    --last;
    return dataFromRule(cached_data.m_tranRules.at(last->ruleIndex), forMSecsSinceEpoch);
}

bool QTzTimeZonePrivate::hasTransitions() const
{
    return true;
}

QTimeZonePrivate::Data QTzTimeZonePrivate::nextTransition(qint64 afterMSecsSinceEpoch) const
{
    // If the required time is after the last transition (or there were none)
    // and we have a POSIX rule, then use it:
    if (!cached_data.m_posixRule.isEmpty()
        && (tranCache().isEmpty() || tranCache().last().atMSecsSinceEpoch < afterMSecsSinceEpoch)) {
        QList<QTimeZonePrivate::Data> posixTrans = getPosixTransitions(afterMSecsSinceEpoch);
        auto it = std::partition_point(posixTrans.cbegin(), posixTrans.cend(),
                                       [afterMSecsSinceEpoch] (const QTimeZonePrivate::Data &at) {
                                           return at.atMSecsSinceEpoch <= afterMSecsSinceEpoch;
                                       });

        return it == posixTrans.cend() ? invalidData() : *it;
    }

    // Otherwise, if we can find a valid tran, use its rule:
    auto last = std::partition_point(tranCache().cbegin(), tranCache().cend(),
                                     [afterMSecsSinceEpoch] (const QTzTransitionTime &at) {
                                         return at.atMSecsSinceEpoch <= afterMSecsSinceEpoch;
                                     });
    return last != tranCache().cend() ? dataForTzTransition(*last) : invalidData();
}

QTimeZonePrivate::Data QTzTimeZonePrivate::previousTransition(qint64 beforeMSecsSinceEpoch) const
{
    // If the required time is after the last transition (or there were none)
    // and we have a POSIX rule, then use it:
    if (!cached_data.m_posixRule.isEmpty()
        && (tranCache().isEmpty() || tranCache().last().atMSecsSinceEpoch < beforeMSecsSinceEpoch)) {
        QList<QTimeZonePrivate::Data> posixTrans = getPosixTransitions(beforeMSecsSinceEpoch);
        auto it = std::partition_point(posixTrans.cbegin(), posixTrans.cend(),
                                       [beforeMSecsSinceEpoch] (const QTimeZonePrivate::Data &at) {
                                           return at.atMSecsSinceEpoch < beforeMSecsSinceEpoch;
                                       });
        if (it > posixTrans.cbegin())
            return *--it;
        // It fell between the last transition (if any) and the first of the POSIX rule:
        return tranCache().isEmpty() ? invalidData() : dataForTzTransition(tranCache().last());
    }

    // Otherwise if we can find a valid tran then use its rule
    auto last = std::partition_point(tranCache().cbegin(), tranCache().cend(),
                                     [beforeMSecsSinceEpoch] (const QTzTransitionTime &at) {
                                         return at.atMSecsSinceEpoch < beforeMSecsSinceEpoch;
                                     });
    return last > tranCache().cbegin() ? dataForTzTransition(*--last) : invalidData();
}

bool QTzTimeZonePrivate::isTimeZoneIdAvailable(const QByteArray &ianaId) const
{
    // Allow a POSIX rule as long as it has offset data. (This needs to reject a
    // plain abbreviation, without offset, since claiming to support such zones
    // would prevent the custom QTimeZone constructor from accepting such a
    // name, as it doesn't want a custom zone to over-ride a "real" one.)
    return tzZones->contains(ianaId) || validatePosixRule(ianaId, true).isValid;
}

QList<QByteArray> QTzTimeZonePrivate::availableTimeZoneIds() const
{
    QList<QByteArray> result = tzZones->keys();
    std::sort(result.begin(), result.end());
    return result;
}

QList<QByteArray> QTzTimeZonePrivate::availableTimeZoneIds(QLocale::Territory territory) const
{
    // TODO AnyTerritory
    QList<QByteArray> result;
    for (auto it = tzZones->cbegin(), end = tzZones->cend(); it != end; ++it) {
        if (it.value().territory == territory)
            result << it.key();
    }
    std::sort(result.begin(), result.end());
    return result;
}

// Getting the system zone's ID:

namespace {
class ZoneNameReader
{
public:
    QByteArray name()
    {
        /* Assumptions:
           a) Systems don't change which of localtime and TZ they use without a
              reboot.
           b) When they change, they use atomic renames, hence a new device and
              inode for the new file.
           c) If we change which *name* is used for a zone, while referencing
              the same final zoneinfo file, we don't care about the change of
              name (e.g. if Europe/Oslo and Europe/Berlin are both symlinks to
              the same CET file, continuing to use the old name, after
              /etc/localtime changes which of the two it points to, is
              harmless).

           The alternative would be to use a file-system watcher, but they are a
           scarce resource.
         */
        const StatIdent local = identify("/etc/localtime");
        const StatIdent tz = identify("/etc/TZ");
        const StatIdent timezone = identify("/etc/timezone");
        if (!m_name.isEmpty() && m_last.isValid()
            && (m_last == local || m_last == tz || m_last == timezone)) {
            return m_name;
        }

        m_name = etcLocalTime();
        if (!m_name.isEmpty()) {
            m_last = local;
            return m_name;
        }

        // Some systems (e.g. uClibc) have a default value for $TZ in /etc/TZ:
        m_name = etcContent(QStringLiteral("/etc/TZ"));
        if (!m_name.isEmpty()) {
            m_last = tz;
            return m_name;
        }

        // Gentoo still (2020, QTBUG-87326) uses this:
        m_name = etcContent(QStringLiteral("/etc/timezone"));
        m_last = m_name.isEmpty() ? StatIdent() : timezone;
        return m_name;
    }

private:
    QByteArray m_name;
    struct StatIdent
    {
        static constexpr unsigned long bad = ~0ul;
        unsigned long m_dev, m_ino;
        constexpr StatIdent() : m_dev(bad), m_ino(bad) {}
        StatIdent(const QT_STATBUF &data) : m_dev(data.st_dev), m_ino(data.st_ino) {}
        bool isValid() { return m_dev != bad || m_ino != bad; }
        bool operator==(const StatIdent &other)
        { return other.m_dev == m_dev && other.m_ino == m_ino; }
    };
    StatIdent m_last;

    static StatIdent identify(const char *path)
    {
        QT_STATBUF data;
        return QT_STAT(path, &data) == -1 ? StatIdent() : StatIdent(data);
    }

    static QByteArray etcLocalTime()
    {
        // On most distros /etc/localtime is a symlink to a real file so extract
        // name from the path
        const QString tzdir = qEnvironmentVariable("TZDIR");
        constexpr auto zoneinfo = "/zoneinfo/"_L1;
        QString path = QStringLiteral("/etc/localtime");
        long iteration = getSymloopMax();
        // Symlink may point to another symlink etc. before being under zoneinfo/
        // We stop on the first path under /zoneinfo/, even if it is itself a
        // symlink, like America/Montreal pointing to America/Toronto
        do {
            path = QFile::symLinkTarget(path);
            // If it's a zoneinfo file, extract the zone name from its path:
            int index = tzdir.isEmpty() ? -1 : path.indexOf(tzdir);
            if (index >= 0) {
                const auto tail = QStringView{ path }.sliced(index + tzdir.size()).toUtf8();
                return tail.startsWith(u'/') ? tail.sliced(1) : tail;
            }
            index = path.indexOf(zoneinfo);
            if (index >= 0)
                return QStringView{ path }.sliced(index + zoneinfo.size()).toUtf8();
        } while (!path.isEmpty() && --iteration > 0);

        return QByteArray();
    }

    static QByteArray etcContent(const QString &path)
    {
        QFile zone(path);
        if (zone.open(QIODevice::ReadOnly))
            return zone.readAll().trimmed();

        return QByteArray();
    }

    // Any chain of symlinks longer than this is assumed to be a loop:
    static long getSymloopMax()
    {
#ifdef SYMLOOP_MAX
        // If defined, at runtime it can only be greater than this, so this is a safe bet:
        return SYMLOOP_MAX;
#else
        errno = 0;
        long result = sysconf(_SC_SYMLOOP_MAX);
        if (result >= 0)
            return result;
        // result is -1, meaning either error or no limit
        Q_ASSERT(!errno); // ... but it can't be an error, POSIX mandates _SC_SYMLOOP_MAX

        // therefore we can make up our own limit
#  ifdef MAXSYMLINKS
        return MAXSYMLINKS;
#  else
        return 8;
#  endif
#endif
    }
};
}

QByteArray QTzTimeZonePrivate::systemTimeZoneId() const
{
    return staticSystemTimeZoneId();
}

QByteArray QTzTimeZonePrivate::staticSystemTimeZoneId()
{
    // Check TZ env var first, if not populated try find it
    QByteArray ianaId = qgetenv("TZ");

    // The TZ value can be ":/etc/localtime" which libc considers
    // to be a "default timezone", in which case it will be read
    // by one of the blocks below, so unset it here so it is not
    // considered as a valid/found ianaId
    if (ianaId == ":/etc/localtime")
        ianaId.clear();
    else if (ianaId.startsWith(':'))
        ianaId = ianaId.sliced(1);

    if (ianaId.isEmpty()) {
        Q_CONSTINIT thread_local static ZoneNameReader reader;
        ianaId = reader.name();
    }

    return ianaId;
}

QT_END_NAMESPACE
