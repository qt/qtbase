// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformdefs.h"
#include "private/qdatetimeparser_p.h"

#include "qdatastream.h"
#include "qdatetime.h"
#include "qdebug.h"
#include "qlocale.h"
#include "qset.h"
#include "qtimezone.h"
#include "qvarlengtharray.h"
#include "private/qlocale_p.h"

#include "private/qstringiterator_p.h"
#include "private/qtenvironmentvariables_p.h"

//#define QDATETIMEPARSER_DEBUG
#if defined (QDATETIMEPARSER_DEBUG) && !defined(QT_NO_DEBUG_STREAM)
#  define QDTPDEBUG qDebug()
#  define QDTPDEBUGN qDebug
#else
#  define QDTPDEBUG if (false) qDebug()
#  define QDTPDEBUGN if (false) qDebug
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

template <typename T>
using ShortVector = QVarLengthArray<T, 13>; // enough for month (incl. leap) and day-of-week names

QDateTimeParser::~QDateTimeParser()
{
}

/*!
  \internal
  Gets the digit from a datetime. E.g.

  QDateTime var(QDate(2004, 02, 02));
  int digit = getDigit(var, Year);
  // digit = 2004
*/

int QDateTimeParser::getDigit(const QDateTime &t, int index) const
{
    if (index < 0 || index >= sectionNodes.size()) {
        qWarning("QDateTimeParser::getDigit() Internal error (%ls %d)",
                 qUtf16Printable(t.toString()), index);
        return -1;
    }
    const SectionNode &node = sectionNodes.at(index);
    switch (node.type) {
    case TimeZoneSection: return t.offsetFromUtc();
    case Hour24Section: case Hour12Section: return t.time().hour();
    case MinuteSection: return t.time().minute();
    case SecondSection: return t.time().second();
    case MSecSection: return t.time().msec();
    case YearSection2Digits:
    case YearSection: return t.date().year(calendar);
    case MonthSection: return t.date().month(calendar);
    case DaySection: return t.date().day(calendar);
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong: return calendar.dayOfWeek(t.date());
    case AmPmSection: return t.time().hour() > 11 ? 1 : 0;

    default: break;
    }

    qWarning("QDateTimeParser::getDigit() Internal error 2 (%ls %d)",
             qUtf16Printable(t.toString()), index);
    return -1;
}

/*!
    \internal
    Difference between two days of the week.

    Returns a difference in the range from -3 through +3, so that steps by small
    numbers of days move us through the month in the same direction as through
    the week.
*/

static int dayOfWeekDiff(int sought, int held)
{
    const int diff = sought - held;
    return diff < -3 ? diff + 7 : diff > 3 ? diff - 7 : diff;
}

static bool preferDayOfWeek(const QList<QDateTimeParser::SectionNode> &nodes)
{
    // True precisely if there is a day-of-week field but no day-of-month field.
    bool result = false;
    for (const auto &node : nodes) {
        if (node.type & QDateTimeParser::DaySection)
            return false;
        if (node.type & QDateTimeParser::DayOfWeekSectionMask)
            result = true;
    }
    return result;
}

/*!
  \internal
  Sets a digit in a datetime. E.g.

  QDateTime var(QDate(2004, 02, 02));
  int digit = getDigit(var, Year);
  // digit = 2004
  setDigit(&var, Year, 2005);
  digit = getDigit(var, Year);
  // digit = 2005
*/

bool QDateTimeParser::setDigit(QDateTime &v, int index, int newVal) const
{
    if (index < 0 || index >= sectionNodes.size()) {
        qWarning("QDateTimeParser::setDigit() Internal error (%ls %d %d)",
                 qUtf16Printable(v.toString()), index, newVal);
        return false;
    }

    const QDate oldDate = v.date();
    QCalendar::YearMonthDay date = calendar.partsFromDate(oldDate);
    if (!date.isValid())
        return false;
    int weekDay = calendar.dayOfWeek(oldDate);
    enum { NoFix, MonthDay, WeekDay } fixDay = NoFix;

    const QTime time = v.time();
    int hour = time.hour();
    int minute = time.minute();
    int second = time.second();
    int msec = time.msec();
    QTimeZone timeZone = v.timeRepresentation();

    const SectionNode &node = sectionNodes.at(index);
    switch (node.type) {
    case Hour24Section: case Hour12Section: hour = newVal; break;
    case MinuteSection: minute = newVal; break;
    case SecondSection: second = newVal; break;
    case MSecSection: msec = newVal; break;
    case YearSection2Digits:
    case YearSection: date.year = newVal; break;
    case MonthSection: date.month = newVal; break;
    case DaySection:
        if (newVal > 31) {
            // have to keep legacy behavior. setting the
            // date to 32 should return false. Setting it
            // to 31 for february should return true
            return false;
        }
        date.day = newVal;
        fixDay = MonthDay;
        break;
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (newVal > 7 || newVal <= 0)
            return false;
        date.day += dayOfWeekDiff(newVal, weekDay);
        weekDay = newVal;
        fixDay = WeekDay;
        break;
    case TimeZoneSection:
        if (newVal < absoluteMin(index) || newVal > absoluteMax(index))
            return false;
        // Only offset from UTC is amenable to setting an int value:
        timeZone = QTimeZone::fromSecondsAheadOfUtc(newVal);
        break;
    case AmPmSection: hour = (newVal == 0 ? hour % 12 : (hour % 12) + 12); break;
    default:
        qWarning("QDateTimeParser::setDigit() Internal error (%ls)",
                 qUtf16Printable(node.name()));
        break;
    }

    if (!(node.type & DaySectionMask)) {
        if (date.day < cachedDay)
            date.day = cachedDay;
        fixDay = MonthDay;
        if (weekDay > 0 && weekDay <= 7 && preferDayOfWeek(sectionNodes)) {
            const int max = calendar.daysInMonth(date.month, date.year);
            if (max > 0 && date.day > max)
                date.day = max;
            const int newDoW = calendar.dayOfWeek(calendar.dateFromParts(date));
            if (newDoW > 0 && newDoW <= 7)
                date.day += dayOfWeekDiff(weekDay, newDoW);
            fixDay = WeekDay;
        }
    }

    if (fixDay != NoFix) {
        const int max = calendar.daysInMonth(date.month, date.year);
        // max > 0 precisely if the year does have such a month
        if (max > 0 && date.day > max)
            date.day = fixDay == WeekDay ? date.day - 7 : max;
        else if (date.day < 1)
            date.day = fixDay == WeekDay ? date.day + 7 : 1;
        Q_ASSERT(fixDay != WeekDay
                 || calendar.dayOfWeek(calendar.dateFromParts(date)) == weekDay);
    }

    const QDate newDate = calendar.dateFromParts(date);
    const QTime newTime(hour, minute, second, msec);
    if (!newDate.isValid() || !newTime.isValid())
        return false;

    v = QDateTime(newDate, newTime, timeZone);
    return true;
}



/*!
  \internal

  Returns the absolute maximum for a section
*/

int QDateTimeParser::absoluteMax(int s, const QDateTime &cur) const
{
    const SectionNode &sn = sectionNode(s);
    switch (sn.type) {
    case TimeZoneSection:
        return QTimeZone::MaxUtcOffsetSecs;
    case Hour24Section:
    case Hour12Section:
        // This is special-cased in parseSection.
        // We want it to be 23 for the stepBy case.
        return 23;
    case MinuteSection:
    case SecondSection:
        return 59;
    case MSecSection:
        return 999;
    case YearSection2Digits:
    case YearSection:
        // sectionMaxSize will prevent people from typing in a larger number in
        // count == 2 sections; stepBy() will work on real years anyway.
        return 9999;
    case MonthSection:
        return calendar.maximumMonthsInYear();
    case DaySection:
        return cur.isValid() ? cur.date().daysInMonth(calendar) : calendar.maximumDaysInMonth();
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        return 7;
    case AmPmSection:
        return int(UpperCase);
    default:
        break;
    }
    qWarning("QDateTimeParser::absoluteMax() Internal error (%ls)",
             qUtf16Printable(sn.name()));
    return -1;
}

/*!
  \internal

  Returns the absolute minimum for a section
*/

int QDateTimeParser::absoluteMin(int s) const
{
    const SectionNode &sn = sectionNode(s);
    switch (sn.type) {
    case TimeZoneSection:
        return QTimeZone::MinUtcOffsetSecs;
    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case MSecSection:
    case YearSection2Digits:
        return 0;
    case YearSection:
        return -9999;
    case MonthSection:
    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong: return 1;
    case AmPmSection: return int(NativeCase);
    default: break;
    }
    qWarning("QDateTimeParser::absoluteMin() Internal error (%ls, %0x)",
             qUtf16Printable(sn.name()), sn.type);
    return -1;
}

/*!
  \internal

  Returns the sectionNode for the Section \a s.
*/

const QDateTimeParser::SectionNode &QDateTimeParser::sectionNode(int sectionIndex) const
{
    if (sectionIndex < 0) {
        switch (sectionIndex) {
        case FirstSectionIndex:
            return first;
        case LastSectionIndex:
            return last;
        case NoSectionIndex:
            return none;
        }
    } else if (sectionIndex < sectionNodes.size()) {
        return sectionNodes.at(sectionIndex);
    }

    qWarning("QDateTimeParser::sectionNode() Internal error (%d)",
             sectionIndex);
    return none;
}

QDateTimeParser::Section QDateTimeParser::sectionType(int sectionIndex) const
{
    return sectionNode(sectionIndex).type;
}


/*!
  \internal

  Returns the starting position for section \a s.
*/

int QDateTimeParser::sectionPos(int sectionIndex) const
{
    return sectionPos(sectionNode(sectionIndex));
}

int QDateTimeParser::sectionPos(const SectionNode &sn) const
{
    switch (sn.type) {
    case FirstSection: return 0;
    case LastSection: return displayText().size() - 1;
    default: break;
    }
    if (sn.pos == -1) {
        qWarning("QDateTimeParser::sectionPos Internal error (%ls)", qUtf16Printable(sn.name()));
        return -1;
    }
    return sn.pos;
}

/*!
  \internal

  Helper function for parseSection.
*/

static qsizetype digitCount(QStringView str)
{
    qsizetype digits = 0;
    for (QStringIterator it(str); it.hasNext();) {
        if (!QChar::isDigit(it.next()))
            break;
        digits++;
    }
    return digits;
}

/*!
  \internal

  helper function for parseFormat. removes quotes that are
  not escaped and removes the escaping on those that are escaped

*/
static QString unquote(QStringView str)
{
    // ### Align unquoting format strings for both from/toString(), QTBUG-110669
    const QLatin1Char quote('\'');
    const QLatin1Char slash('\\');
    const QLatin1Char zero('0');
    QString ret;
    QChar status(zero);
    const int max = str.size();
    for (int i=0; i<max; ++i) {
        if (str.at(i) == quote) {
            if (status != quote)
                status = quote;
            else if (!ret.isEmpty() && str.at(i - 1) == slash)
                ret[ret.size() - 1] = quote;
            else
                status = zero;
        } else {
            ret += str.at(i);
        }
    }
    return ret;
}

static inline int countRepeat(QStringView str, int index, int maxCount)
{
    str = str.sliced(index);
    if (maxCount < str.size())
        str = str.first(maxCount);

    return qt_repeatCount(str);
}

static inline void appendSeparator(QStringList *list, QStringView string,
                                   int from, int size, int lastQuote)
{
    Q_ASSERT(size >= 0 && from + size <= string.size());
    const QStringView separator = string.sliced(from, size);
    list->append(lastQuote >= from ? unquote(separator) : separator.toString());
}

/*!
    \internal

    Parses the format \a newFormat. If successful, returns \c true and sets up
    the format. Else keeps the old format and returns \c false.
*/
bool QDateTimeParser::parseFormat(QStringView newFormat)
{
    const QLatin1Char quote('\'');
    const QLatin1Char slash('\\');
    const QLatin1Char zero('0');
    if (newFormat == displayFormat && !newFormat.isEmpty())
        return true;

    QDTPDEBUGN("parseFormat: %s", newFormat.toLatin1().constData());

    QList<SectionNode> newSectionNodes;
    Sections newDisplay;
    QStringList newSeparators;
    int i, index = 0;
    int add = 0;
    QLatin1Char status(zero);
    const int max = newFormat.size();
    int lastQuote = -1;
    for (i = 0; i<max; ++i) {
        if (newFormat.at(i) == quote) {
            lastQuote = i;
            ++add;
            if (status != quote)
                status = quote;
            else if (i > 0 && newFormat.at(i - 1) != slash)
                status = zero;
        } else if (status != quote) {
            const char sect = newFormat.at(i).toLatin1();
            switch (sect) {
            case 'H':
            case 'h':
                if (parserType != QMetaType::QDate) {
                    const Section hour = (sect == 'h') ? Hour12Section : Hour24Section;
                    const SectionNode sn = { hour, i - add, countRepeat(newFormat, i, 2), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= hour;
                }
                break;
            case 'm':
                if (parserType != QMetaType::QDate) {
                    const SectionNode sn = { MinuteSection, i - add, countRepeat(newFormat, i, 2), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= MinuteSection;
                }
                break;
            case 's':
                if (parserType != QMetaType::QDate) {
                    const SectionNode sn = { SecondSection, i - add, countRepeat(newFormat, i, 2), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= SecondSection;
                }
                break;

            case 'z':
                if (parserType != QMetaType::QDate) {
                    const int repeat = countRepeat(newFormat, i, 3);
                    const SectionNode sn = { MSecSection, i - add, repeat < 3 ? 1 : 3, 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += repeat - 1;
                    index = i + 1;
                    newDisplay |= MSecSection;
                }
                break;
            case 'A':
            case 'a':
                if (parserType != QMetaType::QDate) {
                    const int pos = i - add;
                    Case caseOpt = sect == 'A' ? UpperCase : LowerCase;
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    newDisplay |= AmPmSection;
                    if (i + 1 < newFormat.size()
                        && newFormat.sliced(i + 1).startsWith(u'p', Qt::CaseInsensitive)) {
                        ++i;
                        if (newFormat.at(i) != QLatin1Char(caseOpt == UpperCase ? 'P' : 'p'))
                            caseOpt = NativeCase;
                    }
                    const SectionNode sn = { AmPmSection, pos, int(caseOpt), 0 };
                    newSectionNodes.append(sn);
                    index = i + 1;
                }
                break;
            case 'y':
                if (parserType != QMetaType::QTime) {
                    const int repeat = countRepeat(newFormat, i, 4);
                    if (repeat >= 2) {
                        const SectionNode sn = { repeat == 4 ? YearSection : YearSection2Digits,
                                                 i - add, repeat == 4 ? 4 : 2, 0 };
                        newSectionNodes.append(sn);
                        appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                        i += sn.count - 1;
                        index = i + 1;
                        newDisplay |= sn.type;
                    }
                }
                break;
            case 'M':
                if (parserType != QMetaType::QTime) {
                    const SectionNode sn = { MonthSection, i - add, countRepeat(newFormat, i, 4), 0 };
                    newSectionNodes.append(sn);
                    newSeparators.append(unquote(newFormat.first(i).sliced(index)));
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= MonthSection;
                }
                break;
            case 'd':
                if (parserType != QMetaType::QTime) {
                    const int repeat = countRepeat(newFormat, i, 4);
                    const Section sectionType = (repeat == 4 ? DayOfWeekSectionLong
                        : (repeat == 3 ? DayOfWeekSectionShort : DaySection));
                    const SectionNode sn = { sectionType, i - add, repeat, 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= sn.type;
                }
                break;
            case 't':
                if (parserType == QMetaType::QDateTime) {
                    const SectionNode sn
                        = { TimeZoneSection, i - add, countRepeat(newFormat, i, 4), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= TimeZoneSection;
                }
                break;
            default:
                break;
            }
        }
    }
    if (newSectionNodes.isEmpty() && context == DateTimeEdit)
        return false;

    if ((newDisplay & (AmPmSection|Hour12Section)) == Hour12Section) {
        const int count = newSectionNodes.size();
        for (int i = 0; i < count; ++i) {
            SectionNode &node = newSectionNodes[i];
            if (node.type == Hour12Section)
                node.type = Hour24Section;
        }
    }

    if (index < max)
        appendSeparator(&newSeparators, newFormat, index, max - index, lastQuote);
    else
        newSeparators.append(QString());

    displayFormat = newFormat.toString();
    separators = newSeparators;
    sectionNodes = newSectionNodes;
    display = newDisplay;
    last.pos = -1;

//     for (int i=0; i<sectionNodes.size(); ++i) {
//         QDTPDEBUG << sectionNodes.at(i).name() << sectionNodes.at(i).count;
//     }

    QDTPDEBUG << newFormat << displayFormat;
    QDTPDEBUGN("separators:\n'%s'", separators.join("\n"_L1).toLatin1().constData());

    return true;
}

/*!
  \internal

  Returns the size of section \a s.
*/

int QDateTimeParser::sectionSize(int sectionIndex) const
{
    if (sectionIndex < 0)
        return 0;

    if (sectionIndex >= sectionNodes.size()) {
        qWarning("QDateTimeParser::sectionSize Internal error (%d)", sectionIndex);
        return -1;
    }

    if (sectionIndex == sectionNodes.size() - 1) {
        // In some cases there is a difference between displayText() and text.
        // e.g. when text is 2000/01/31 and displayText() is "2000/2/31" - text
        // is the previous value and displayText() is the new value.
        // The size difference is always due to leading zeroes.
        int sizeAdjustment = 0;
        const int displayTextSize = displayText().size();
        if (displayTextSize != m_text.size()) {
            // Any zeroes added before this section will affect our size.
            int preceedingZeroesAdded = 0;
            if (sectionNodes.size() > 1 && context == DateTimeEdit) {
                const auto begin = sectionNodes.cbegin();
                const auto end = begin + sectionIndex;
                for (auto sectionIt = begin; sectionIt != end; ++sectionIt)
                    preceedingZeroesAdded += sectionIt->zeroesAdded;
            }
            sizeAdjustment = preceedingZeroesAdded;
        }

        return displayTextSize + sizeAdjustment - sectionPos(sectionIndex) - separators.last().size();
    } else {
        return sectionPos(sectionIndex + 1) - sectionPos(sectionIndex)
            - separators.at(sectionIndex + 1).size();
    }
}


int QDateTimeParser::sectionMaxSize(Section s, int count) const
{
#if QT_CONFIG(textdate)
    int mcount = calendar.maximumMonthsInYear();
#endif

    switch (s) {
    case FirstSection:
    case NoSection:
    case LastSection:
        return 0;

    case AmPmSection:
        // Special: "count" here is a case flag, not field width !
        return qMax(getAmPmText(AmText, Case(count)).size(),
                    getAmPmText(PmText, Case(count)).size());

    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case DaySection:
        return 2;

    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
#if QT_CONFIG(textdate)
        mcount = 7;
        Q_FALLTHROUGH();
#endif
    case MonthSection:
#if QT_CONFIG(textdate)
        if (count <= 2)
            return 2;

        {
            int ret = 0;
            const QLocale l = locale();
            const QLocale::FormatType format = count == 4 ? QLocale::LongFormat : QLocale::ShortFormat;
            for (int i=1; i<=mcount; ++i) {
                const QString str = (s == MonthSection
                                     ? calendar.monthName(l, i, QCalendar::Unspecified, format)
                                     : l.dayName(i, format));
                ret = qMax(str.size(), ret);
            }
            return ret;
        }
#else
        return 2;
#endif // textdate
    case MSecSection:
        return 3;
    case YearSection:
        return 4;
    case YearSection2Digits:
        return 2;
    case TimeZoneSection:
        // Arbitrarily many tokens (each up to 14 bytes) joined with / separators:
        return std::numeric_limits<int>::max();

    case CalendarPopupSection:
    case Internal:
    case TimeSectionMask:
    case DateSectionMask:
    case HourSectionMask:
    case YearSectionMask:
    case DayOfWeekSectionMask:
    case DaySectionMask:
        qWarning("QDateTimeParser::sectionMaxSize: Invalid section %s",
                 SectionNode::name(s).toLatin1().constData());

    case NoSectionIndex:
    case FirstSectionIndex:
    case LastSectionIndex:
    case CalendarPopupIndex:
        // these cases can't happen
        break;
    }
    return -1;
}


int QDateTimeParser::sectionMaxSize(int index) const
{
    const SectionNode &sn = sectionNode(index);
    return sectionMaxSize(sn.type, sn.count);
}

// Separator matching
//
// QTBUG-114909: user may be oblivious to difference between visibly
// indistinguishable spacing characters. For now we only treat horizontal
// spacing characters, excluding tab, as equivalent.

static int matchesSeparator(QStringView text, QStringView separator)
{
    const auto isSimpleSpace = [](char32_t ch) {
        // Distinguish tab, CR and the vertical spaces from the rest:
        return ch == u' ' || (ch > 127 && QChar::isSpace(ch));
    };
    // -1 if not a match, else length of prefix of text that does match.
    // First check for exact match
    if (!text.startsWith(separator)) {
        // Failing that, check for space-identifying match:
        QStringIterator given(text), sep(separator);
        while (sep.hasNext()) {
            if (!given.hasNext())
                return -1;
            char32_t s = sep.next(), g = given.next();
            if (s != g && !(isSimpleSpace(s) && isSimpleSpace(g)))
                return -1;
        }
        // One side may have used a surrogate pair space where the other didn't:
        return given.index();
    }
    return separator.size();
}

/*!
  \internal

  Returns the text of section \a s. This function operates on the
  arg text rather than edit->text().
*/


QString QDateTimeParser::sectionText(const QString &text, int sectionIndex, int index) const
{
    const SectionNode &sn = sectionNode(sectionIndex);
    switch (sn.type) {
    case NoSectionIndex:
    case FirstSectionIndex:
    case LastSectionIndex:
        return QString();
    default: break;
    }

    return text.mid(index, sectionSize(sectionIndex));
}

QString QDateTimeParser::sectionText(int sectionIndex) const
{
    const SectionNode &sn = sectionNode(sectionIndex);
    return sectionText(displayText(), sectionIndex, sn.pos);
}

QDateTimeParser::ParsedSection
QDateTimeParser::parseSection(const QDateTime &currentValue, int sectionIndex, int offset) const
{
    ParsedSection result; // initially Invalid
    const SectionNode &sn = sectionNode(sectionIndex);
    Q_ASSERT_X(!(sn.type & Internal),
               "QDateTimeParser::parseSection", "Internal error");

    const int sectionmaxsize = sectionMaxSize(sectionIndex);
    const bool negate = (sn.type == YearSection && m_text.size() > offset
                         && m_text.at(offset) == u'-');
    const int negativeYearOffset = negate ? 1 : 0;

    // If the fields we've read thus far imply a time in a spring-forward,
    // coerce to a nearby valid time:
    const QDateTime defaultValue = currentValue.isValid() ? currentValue
        : QDateTime::fromMSecsSinceEpoch(currentValue.toMSecsSinceEpoch());

    QStringView sectionTextRef =
            QStringView { m_text }.mid(offset + negativeYearOffset, sectionmaxsize);

    QDTPDEBUG << "sectionValue for" << sn.name()
              << "with text" << m_text << "and (at" << offset
              << ") st:" << sectionTextRef;

    switch (sn.type) {
    case AmPmSection: {
        QString sectiontext = sectionTextRef.toString();
        int used;
        const int ampm = findAmPm(sectiontext, sectionIndex, &used);
        switch (ampm) {
        case AM: // sectiontext == AM
        case PM: // sectiontext == PM
            result = ParsedSection(Acceptable, ampm, used);
            break;
        case PossibleAM: // sectiontext => AM
        case PossiblePM: // sectiontext => PM
            result = ParsedSection(Intermediate, ampm - 2, used);
            break;
        case PossibleBoth: // sectiontext => AM|PM
            result = ParsedSection(Intermediate, 0, used);
            break;
        case Neither:
            QDTPDEBUG << "invalid because findAmPm(" << sectiontext << ") returned -1";
            break;
        default:
            QDTPDEBUGN("This should never happen (findAmPm returned %d)", ampm);
            break;
        }
        if (result.state != Invalid)
            m_text.replace(offset, used, sectiontext.constData(), used);
        break; }
    case TimeZoneSection:
        result = findTimeZone(sectionTextRef, defaultValue,
                              absoluteMax(sectionIndex),
                              absoluteMin(sectionIndex), sn.count);
        break;
    case MonthSection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (sn.count >= 3) {
            QString sectiontext = sectionTextRef.toString();
            int num = 0, used = 0;
            if (sn.type == MonthSection) {
                const QDate minDate = getMinimum().date();
                const int year = defaultValue.date().year(calendar);
                const int min = (year == minDate.year(calendar)) ? minDate.month(calendar) : 1;
                num = findMonth(sectiontext.toLower(), min, sectionIndex, year, &sectiontext, &used);
            } else {
                num = findDay(sectiontext.toLower(), 1, sectionIndex, &sectiontext, &used);
            }

            result = ParsedSection(Intermediate, num, used);
            if (num != -1) {
                m_text.replace(offset, used, sectiontext.constData(), used);
                if (used == sectiontext.size())
                    result = ParsedSection(Acceptable, num, used);
            }
            break;
        }
        Q_FALLTHROUGH();
        // All numeric:
    case DaySection:
    case YearSection:
    case YearSection2Digits:
    case Hour12Section:
    case Hour24Section:
    case MinuteSection:
    case SecondSection:
    case MSecSection: {
        const auto checkSeparator = [&result, field=QStringView{m_text}.sliced(offset),
                                     negativeYearOffset, sectionIndex, this]() {
            // No-digit field if next separator is here, otherwise invalid.
            const auto &sep = separators.at(sectionIndex + 1);
            if (matchesSeparator(field.sliced(negativeYearOffset), sep) != -1)
                result = ParsedSection(Intermediate, 0, negativeYearOffset);
            else if (negativeYearOffset && matchesSeparator(field, sep) != -1)
                result = ParsedSection(Intermediate, 0, 0);
            else
                return false;
            return true;
        };
        int used = negativeYearOffset;
        // We already sliced off the - sign if it was acceptable.
        // QLocale::toUInt() would accept a sign, so we must reject it overtly:
        if (sectionTextRef.startsWith(u'-')
            || sectionTextRef.startsWith(u'+')) {
            // However, a sign here may indicate a field with no digits, if it
            // starts the next separator:
            checkSeparator();
            break;
        }
        QStringView digitsStr = sectionTextRef.left(digitCount(sectionTextRef));

        if (digitsStr.isEmpty()) {
            result = ParsedSection(Intermediate, 0, used);
        } else {
            const QLocale loc = locale();
            const int absMax = absoluteMax(sectionIndex);
            const int absMin = absoluteMin(sectionIndex);

            int lastVal = -1;

            for (; digitsStr.size(); digitsStr.chop(1)) {
                bool ok = false;
                int value = int(loc.toUInt(digitsStr, &ok));
                if (!ok || (negate ? -value < absMin : value > absMax))
                    continue;

                if (sn.type == Hour12Section) {
                    if (value > 12)
                        continue;
                    if (value == 12)
                        value = 0;
                }

                QDTPDEBUG << digitsStr << value << digitsStr.size();
                lastVal = value;
                used += digitsStr.size();
                break;
            }

            if (lastVal == -1) {
                if (!checkSeparator()) {
                    QDTPDEBUG << "invalid because" << sectionTextRef << "can't become a uint"
                              << lastVal;
                }
            } else {
                if (negate)
                    lastVal = -lastVal;
                const FieldInfo fi = fieldInfo(sectionIndex);
                const bool unfilled = used - negativeYearOffset < sectionmaxsize;
                if (unfilled && fi & Fraction) { // typing 2 in a zzz field should be .200, not .002
                    for (int i = used; i < sectionmaxsize; ++i)
                        lastVal *= 10;
                }
                // Even those *= 10s can't take last above absMax:
                Q_ASSERT(negate ? lastVal >= absMin : lastVal <= absMax);
                if (negate ? lastVal > absMax : lastVal < absMin) {
                    if (unfilled) {
                        result = ParsedSection(Intermediate, lastVal, used);
                    } else if (negate) {
                        QDTPDEBUG << "invalid because" << lastVal << "is greater than absoluteMax"
                                  << absMax;
                    } else {
                        QDTPDEBUG << "invalid because" << lastVal << "is less than absoluteMin"
                                  << absMin;
                    }

                } else if (unfilled && (fi & (FixedWidth | Numeric)) == (FixedWidth | Numeric)) {
                    if (skipToNextSection(sectionIndex, defaultValue, digitsStr)) {
                        const int missingZeroes = sectionmaxsize - digitsStr.size();
                        result = ParsedSection(Acceptable, lastVal, sectionmaxsize, missingZeroes);
                        m_text.insert(offset, QString(missingZeroes, u'0'));
                        ++(const_cast<QDateTimeParser*>(this)->sectionNodes[sectionIndex].zeroesAdded);
                    } else {
                        result = ParsedSection(Intermediate, lastVal, used);;
                    }
                } else {
                    result = ParsedSection(Acceptable, lastVal, used);
                }
            }
        }
        break; }
    default:
        qWarning("QDateTimeParser::parseSection Internal error (%ls %d)",
                 qUtf16Printable(sn.name()), sectionIndex);
        return result;
    }
    Q_ASSERT(result.state != Invalid || result.value == -1);

    return result;
}

/*!
  \internal

  Returns the day-number of a day, as close as possible to the given \a day, in
  the specified \a month of \a year for the given \a calendar, that falls on the
  day of the week indicated by \a weekDay.
*/

static int weekDayWithinMonth(QCalendar calendar, int year, int month, int day, int weekDay)
{
    // TODO: can we adapt this to cope gracefully with intercallary days (day of
    // week > 7) without making it slower for more widely-used calendars ?
    const int maxDay = calendar.daysInMonth(month, year); // 0 if no such month
    day = maxDay > 1 ? qBound(1, day, maxDay) : qMax(1, day);
    day += dayOfWeekDiff(weekDay, calendar.dayOfWeek(QDate(year, month, day, calendar)));
    return day <= 0 ? day + 7 : maxDay > 0 && day > maxDay ? day - 7 : day;
}

/*!
  \internal

  Returns a date consistent with the given data on parts specified by known,
  while staying as close to the given data as it can.  Returns an invalid date
  when on valid date is consistent with the data.
*/

static QDate actualDate(QDateTimeParser::Sections known, const QCalendar &calendar,
                        int year, int year2digits, int month, int day, int dayofweek)
{
    QDate actual(year, month, day, calendar);
    if (actual.isValid() && year % 100 == year2digits && calendar.dayOfWeek(actual) == dayofweek)
        return actual; // The obvious candidate is fine :-)

    if (dayofweek < 1 || dayofweek > 7) // Intercallary (or invalid): ignore
        known &= ~QDateTimeParser::DayOfWeekSectionMask;

    // Assuming year > 0 ...
    if (year % 100 != year2digits) {
        if (known & QDateTimeParser::YearSection2Digits) {
            // Over-ride year, even if specified:
            year += year2digits - year % 100;
            known &= ~QDateTimeParser::YearSection;
        } else {
            year2digits = year % 100;
        }
    }
    Q_ASSERT(year % 100 == year2digits);

    if (month < 1) { // If invalid, clip to nearest valid and ignore in known.
        month = 1;
        known &= ~QDateTimeParser::MonthSection;
    } else if (month > 12) {
        month = 12;
        known &= ~QDateTimeParser::MonthSection;
    }

    QDate first(year, month, 1, calendar);
    int last = known & QDateTimeParser::MonthSection
        ? (known & QDateTimeParser::YearSection
           ? calendar.daysInMonth(month, year) : calendar.daysInMonth(month))
        : 0;
    // We can only fix DOW if we know year as well as month (hence last):
    const bool fixDayOfWeek = last && known & QDateTimeParser::YearSection
            && known & QDateTimeParser::DayOfWeekSectionMask;
    // If we also know day-of-week, tweak last to the last in the month that matches it:
    if (fixDayOfWeek) {
        const int diff = (dayofweek - calendar.dayOfWeek(first) - last) % 7;
        Q_ASSERT(diff <= 0); // C++11 specifies (-ve) % (+ve) to be <= 0.
        last += diff;
    }
    if (day < 1) {
        if (fixDayOfWeek) {
            day = 1 + dayofweek - calendar.dayOfWeek(first);
            if (day < 1)
                day += 7;
        } else {
            day = 1;
        }
        known &= ~QDateTimeParser::DaySection;
    } else if (day > calendar.maximumDaysInMonth()) {
        day = last;
        known &= ~QDateTimeParser::DaySection;
    } else if (last && day > last && (known & QDateTimeParser::DaySection) == 0) {
        day = last;
    }

    actual = QDate(year, month, day, calendar);
    if (!actual.isValid() // We can't do better than we have, in this case
        || (known & QDateTimeParser::DaySection
            && known & QDateTimeParser::MonthSection
            && known & QDateTimeParser::YearSection) // ditto
        || calendar.dayOfWeek(actual) == dayofweek // Good enough, use it.
        || (known & QDateTimeParser::DayOfWeekSectionMask) == 0) { // No contradiction, use it.
        return actual;
    }

    /*
      Now it gets trickier.

      We have some inconsistency in our data; we've been told day of week, but
      it doesn't fit with our year, month and day.  At least one of these is
      unknown, though: so we can fix day of week by tweaking it.
    */

    if ((known & QDateTimeParser::DaySection) == 0) {
        // Relatively easy to fix.
        day = weekDayWithinMonth(calendar, year, month, day, dayofweek);
        actual = QDate(year, month, day, calendar);
        return actual;
    }

    if ((known & QDateTimeParser::MonthSection) == 0) {
        /*
          Try possible month-offsets, m, preferring small; at least one (present
          month doesn't work) and at most 11 (max month, 12, minus min, 1); try
          in both directions, ignoring any offset that takes us out of range.
        */
        for (int m = 1; m < 12; m++) {
            if (m < month) {
                actual = QDate(year, month - m, day, calendar);
                if (calendar.dayOfWeek(actual) == dayofweek)
                    return actual;
            }
            if (m + month <= 12) {
                actual = QDate(year, month + m, day, calendar);
                if (calendar.dayOfWeek(actual) == dayofweek)
                    return actual;
            }
        }
        // Should only get here in corner cases; e.g. day == 31
        actual = QDate(year, month, day, calendar); // Restore from trial values.
    }

    if ((known & QDateTimeParser::YearSection) == 0) {
        if (known & QDateTimeParser::YearSection2Digits) {
            /*
              Two-digit year and month are specified; choice of century can only
              fix this if diff is in one of {1, 2, 5} or {2, 4, 6}; but not if
              diff is in the other.  It's also only reasonable to consider
              adjacent century, e.g. if year thinks it's 2012 and two-digit year
              is '97, it makes sense to consider 1997.  If either adjacent
              century does work, the other won't.
            */
            actual = QDate(year + 100, month, day, calendar);
            if (calendar.dayOfWeek(actual) == dayofweek)
                return actual;
            actual = QDate(year - 100, month, day, calendar);
            if (calendar.dayOfWeek(actual) == dayofweek)
                return actual;
        } else {
            // Offset by 7 is usually enough, but rare cases may need more:
            for (int y = 1; y < 12; y++) {
                actual = QDate(year - y, month, day, calendar);
                if (calendar.dayOfWeek(actual) == dayofweek)
                    return actual;
                actual = QDate(year + y, month, day, calendar);
                if (calendar.dayOfWeek(actual) == dayofweek)
                    return actual;
            }
        }
        actual = QDate(year, month, day, calendar); // Restore from trial values.
    }

    return actual; // It'll just have to do :-(
}

/*!
  \internal
*/

static QTime actualTime(QDateTimeParser::Sections known,
                        int hour, int hour12, int ampm,
                        int minute, int second, int msec)
{
    // If we have no conflict, or don't know enough to diagonose one, use this:
    QTime actual(hour, minute, second, msec);
    if (hour12 < 0 || hour12 > 12) { // ignore bogus value
        known &= ~QDateTimeParser::Hour12Section;
        hour12 = hour % 12;
    }

    if (ampm == -1 || (known & QDateTimeParser::AmPmSection) == 0) {
        if ((known & QDateTimeParser::Hour12Section) == 0 || hour % 12 == hour12)
            return actual;

        if ((known & QDateTimeParser::Hour24Section) == 0)
            hour = hour12 + (hour > 12 ? 12 : 0);
    } else {
        Q_ASSERT(ampm == 0 || ampm == 1);
        if (hour - hour12 == ampm * 12)
            return actual;

        if ((known & QDateTimeParser::Hour24Section) == 0
            && known & QDateTimeParser::Hour12Section) {
            hour = hour12 + ampm * 12;
        }
    }
    actual = QTime(hour, minute, second, msec);
    return actual;
}

/*
  \internal
*/
static int startsWithLocalTimeZone(QStringView name, const QDateTime &when)
{
    // On MS-Win, at least when system zone is UTC, the tzname[]s may be empty.
    for (int i = 0; i < 2; ++i) {
        const QString zone(qTzName(i));
        if (!zone.isEmpty() && name.startsWith(zone))
            return zone.size();
    }
    // Mimic what QLocale::toString() would have used, to ensure round-trips work:
    const QString local = QDateTime(when.date(), when.time()).timeZoneAbbreviation();
    if (name.startsWith(local))
        return local.size();
    return 0;
}

/*!
  \internal
*/
QDateTimeParser::StateNode
QDateTimeParser::scanString(const QDateTime &defaultValue, bool fixup) const
{
    State state = Acceptable;
    bool conflicts = false;
    const int sectionNodesCount = sectionNodes.size();
    int padding = 0;
    int pos = 0;
    int year, month, day;
    const QDate defaultDate = defaultValue.date();
    const QTime defaultTime = defaultValue.time();
    defaultDate.getDate(&year, &month, &day);
    int year2digits = year % 100;
    int hour = defaultTime.hour();
    int hour12 = -1;
    int minute = defaultTime.minute();
    int second = defaultTime.second();
    int msec = defaultTime.msec();
    int dayofweek = calendar.dayOfWeek(defaultDate);
    QTimeZone timeZone = defaultValue.timeRepresentation();

    int ampm = -1;
    Sections isSet = NoSection;

    for (int index = 0; index < sectionNodesCount; ++index) {
        Q_ASSERT(state != Invalid);
        const QString &separator = separators.at(index);
        int step = matchesSeparator(QStringView{m_text}.sliced(pos), separator);
        if (step == -1) {
            QDTPDEBUG << "invalid because" << QStringView{m_text}.sliced(pos)
                      << "does not start with" << separator
                      << index << pos << currentSectionIndex;
            return StateNode();
        }
        pos += step;
        sectionNodes[index].pos = pos;
        int *current = nullptr;
        int zoneOffset; // Needed to serve as *current when setting zone
        const SectionNode sn = sectionNodes.at(index);
        const QDateTime usedDateTime = [&] {
            const QDate date = actualDate(isSet, calendar, year, year2digits,
                                          month, day, dayofweek);
            const QTime time = actualTime(isSet, hour, hour12, ampm, minute, second, msec);
            return QDateTime(date, time, timeZone);
        }();
        ParsedSection sect = parseSection(usedDateTime, index, pos);

        QDTPDEBUG << "sectionValue" << sn.name() << m_text
                  << "pos" << pos << "used" << sect.used << stateName(sect.state);

        padding += sect.zeroes;
        if (fixup && sect.state == Intermediate && sect.used < sn.count) {
            const FieldInfo fi = fieldInfo(index);
            if ((fi & (Numeric|FixedWidth)) == (Numeric|FixedWidth)) {
                const QString newText = QString("%1"_L1).arg(sect.value, sn.count, 10, '0'_L1);
                m_text.replace(pos, sect.used, newText);
                sect.used = sn.count;
            }
        }

        state = qMin<State>(state, sect.state);
        // QDateTimeEdit can fix Intermediate and zeroes, but input needing that didn't match format:
        if (state == Invalid || (context == FromString && (state == Intermediate || sect.zeroes)))
            return StateNode();

        switch (sn.type) {
        case TimeZoneSection:
            current = &zoneOffset;
            if (sect.used > 0) {
                // Synchronize with what findTimeZone() found:
                QStringView zoneName = QStringView{m_text}.sliced(pos, sect.used);
                Q_ASSERT(!zoneName.isEmpty()); // sect.used > 0

                const QStringView offsetStr
                    = zoneName.startsWith("UTC"_L1) ? zoneName.sliced(3) : zoneName;
                const bool isUtcOffset = offsetStr.startsWith(u'+') || offsetStr.startsWith(u'-');
                const bool isUtc = zoneName == "Z"_L1 || zoneName == "UTC"_L1;

                if (isUtc || isUtcOffset) {
                    timeZone = QTimeZone::fromSecondsAheadOfUtc(sect.value);
#if QT_CONFIG(timezone)
                } else if (startsWithLocalTimeZone(zoneName, usedDateTime) != sect.used) {
                    QTimeZone namedZone = QTimeZone(zoneName.toLatin1());
                    Q_ASSERT(namedZone.isValid());
                    timeZone = namedZone;
#endif
                } else {
                    timeZone = QTimeZone::LocalTime;
                }
            }
            break;
        case Hour24Section: current = &hour; break;
        case Hour12Section: current = &hour12; break;
        case MinuteSection: current = &minute; break;
        case SecondSection: current = &second; break;
        case MSecSection: current = &msec; break;
        case YearSection: current = &year; break;
        case YearSection2Digits: current = &year2digits; break;
        case MonthSection: current = &month; break;
        case DayOfWeekSectionShort:
        case DayOfWeekSectionLong: current = &dayofweek; break;
        case DaySection: current = &day; sect.value = qMax<int>(1, sect.value); break;
        case AmPmSection: current = &ampm; break;
        default:
            qWarning("QDateTimeParser::parse Internal error (%ls)",
                     qUtf16Printable(sn.name()));
            return StateNode();
        }
        Q_ASSERT(current);
        Q_ASSERT(sect.state != Invalid);

        if (sect.used > 0)
            pos += sect.used;
        QDTPDEBUG << index << sn.name() << "is set to"
                  << pos << "state is" << stateName(state);

        if (isSet & sn.type && *current != sect.value) {
            QDTPDEBUG << "CONFLICT " << sn.name() << *current << sect.value;
            conflicts = true;
            if (index != currentSectionIndex)
                continue;
        }
        *current = sect.value;

        // Record the present section:
        isSet |= sn.type;
    }

    int step = matchesSeparator(QStringView{m_text}.sliced(pos), separators.last());
    if (step == -1 || step + pos < m_text.size()) {
        QDTPDEBUG << "invalid because" << QStringView{m_text}.sliced(pos)
                  << "does not match" << separators.last() << pos;
        return StateNode();
    }

    if (parserType != QMetaType::QTime) {
        if (year % 100 != year2digits && (isSet & YearSection2Digits)) {
            if (!(isSet & YearSection)) {
                year = (year / 100) * 100;
                year += year2digits;
            } else {
                conflicts = true;
                const SectionNode &sn = sectionNode(currentSectionIndex);
                if (sn.type == YearSection2Digits) {
                    year = (year / 100) * 100;
                    year += year2digits;
                }
            }
        }

        const auto fieldType = sectionType(currentSectionIndex);
        const QDate date(year, month, day, calendar);
        if ((!date.isValid() || dayofweek != calendar.dayOfWeek(date))
                && state == Acceptable && isSet & DayOfWeekSectionMask) {
            if (isSet & DaySection)
                conflicts = true;
            // Change to day of week should adjust day of month;
            // when day of month isn't set, so should change to year or month.
            if (currentSectionIndex == -1 || fieldType & DayOfWeekSectionMask
                    || (!conflicts && (fieldType & (YearSectionMask | MonthSection)))) {
                day = weekDayWithinMonth(calendar, year, month, day, dayofweek);
                QDTPDEBUG << year << month << day << dayofweek
                          << calendar.dayOfWeek(QDate(year, month, day, calendar));
            }
        }

        bool needfixday = false;
        if (fieldType & DaySectionMask) {
            cachedDay = day;
        } else if (cachedDay > day && !(isSet & DayOfWeekSectionMask && state == Acceptable)) {
            day = cachedDay;
            needfixday = true;
        }

        if (!calendar.isDateValid(year, month, day)) {
            if (day <= calendar.maximumDaysInMonth())
                cachedDay = day;
            if (day > calendar.minimumDaysInMonth() && calendar.isDateValid(year, month, 1))
                needfixday = true;
        }
        if (needfixday) {
            if (context == FromString)
                return StateNode();
            if (state == Acceptable && fixday) {
                day = qMin<int>(day, calendar.daysInMonth(month, year));

                const QLocale loc = locale();
                for (int i=0; i<sectionNodesCount; ++i) {
                    const SectionNode sn = sectionNode(i);
                    if (sn.type & DaySection) {
                        m_text.replace(sectionPos(sn), sectionSize(i), loc.toString(day));
                    } else if (sn.type & DayOfWeekSectionMask) {
                        const int dayOfWeek = calendar.dayOfWeek(QDate(year, month, day, calendar));
                        const QLocale::FormatType dayFormat =
                            (sn.type == DayOfWeekSectionShort
                             ? QLocale::ShortFormat : QLocale::LongFormat);
                        const QString dayName(loc.dayName(dayOfWeek, dayFormat));
                        m_text.replace(sectionPos(sn), sectionSize(i), dayName);
                    }
                }
            } else if (state > Intermediate) {
                state = Intermediate;
            }
        }
    }

    if (parserType != QMetaType::QDate) {
        if (isSet & Hour12Section) {
            const bool hasHour = isSet.testAnyFlag(Hour24Section);
            if (ampm == -1) // If we don't know from hour, assume am:
                ampm = !hasHour || hour < 12 ? 0 : 1;
            hour12 = hour12 % 12 + ampm * 12;
            if (!hasHour)
                hour = hour12;
            else if (hour != hour12)
                conflicts = true;
        } else if (ampm != -1) {
            if (!(isSet & (Hour24Section)))
                hour = 12 * ampm; // Special case: only ap section
            else if ((ampm == 0) != (hour < 12))
                conflicts = true;
        }
    }

    QDTPDEBUG << year << month << day << hour << minute << second << msec;
    Q_ASSERT(state != Invalid);

    const QDate date(year, month, day, calendar);
    const QTime time(hour, minute, second, msec);
    const QDateTime when = QDateTime(date, time, timeZone);

    // If hour wasn't specified, check the default we're using exists on the
    // given date (which might be a spring-forward, skipping an hour).
    if (!(isSet & HourSectionMask) && !when.isValid()) {
        switch (parserType) {
        case QMetaType::QDateTime: {
            qint64 msecs = when.toMSecsSinceEpoch();
            // Fortunately, that gets a useful answer, even though when is invalid ...
            const QDateTime replace = QDateTime::fromMSecsSinceEpoch(msecs, timeZone);
            const QTime tick = replace.time();
            if (replace.date() == date
                && (!(isSet & MinuteSection) || tick.minute() == minute)
                && (!(isSet & SecondSection) || tick.second() == second)
                && (!(isSet & MSecSection)   || tick.msec() == msec)) {
                return StateNode(replace, state, padding, conflicts);
            }
        } break;
        case QMetaType::QDate:
            // Don't care about time, so just use start of day (and ignore spec):
            return StateNode(date.startOfDay(QTimeZone::UTC), state, padding, conflicts);
            break;
        case QMetaType::QTime:
            // Don't care about date or representation, so pick a safe representation:
            return StateNode(QDateTime(date, time, QTimeZone::UTC), state, padding, conflicts);
        default:
            Q_UNREACHABLE_RETURN(StateNode());
        }
    }

    return StateNode(when, state, padding, conflicts);
}

/*!
  \internal
*/

QDateTimeParser::StateNode
QDateTimeParser::parse(const QString &input, int position,
                       const QDateTime &defaultValue, bool fixup) const
{
    const QDateTime minimum = getMinimum();
    const QDateTime maximum = getMaximum();
    m_text = input;

    QDTPDEBUG << "parse" << input;
    StateNode scan = scanString(defaultValue, fixup);
    QDTPDEBUGN("'%s' => '%s'(%s)", m_text.toLatin1().constData(),
               scan.value.toString("yyyy/MM/dd hh:mm:ss.zzz"_L1).toLatin1().constData(),
               stateName(scan.state).toLatin1().constData());

    if (scan.value.isValid() && scan.state != Invalid) {
        if (context != FromString && scan.value < minimum) {
            const QLatin1Char space(' ');
            if (scan.value >= minimum)
                qWarning("QDateTimeParser::parse Internal error 3 (%ls %ls)",
                         qUtf16Printable(scan.value.toString()), qUtf16Printable(minimum.toString()));

            bool done = false;
            scan.state = Invalid;
            const int sectionNodesCount = sectionNodes.size();
            for (int i=0; i<sectionNodesCount && !done; ++i) {
                const SectionNode &sn = sectionNodes.at(i);
                QString t = sectionText(m_text, i, sn.pos).toLower();
                if ((t.size() < sectionMaxSize(i)
                     && ((fieldInfo(i) & (FixedWidth|Numeric)) != Numeric))
                    || t.contains(space)) {
                    switch (sn.type) {
                    case AmPmSection:
                        switch (findAmPm(t, i)) {
                        case AM:
                        case PM:
                            scan.state = Acceptable;
                            done = true;
                            break;
                        case Neither:
                            scan.state = Invalid;
                            done = true;
                            break;
                        case PossibleAM:
                        case PossiblePM:
                        case PossibleBoth: {
                            const QDateTime copy(scan.value.addSecs(12 * 60 * 60));
                            if (copy >= minimum && copy <= maximum) {
                                scan.state = Intermediate;
                                done = true;
                            }
                            break; }
                        }
                        Q_FALLTHROUGH();
                    case MonthSection:
                        if (sn.count >= 3) {
                            const QDate when = scan.value.date();
                            const int finalMonth = when.month(calendar);
                            int tmp = finalMonth;
                            // I know the first possible month makes the date too early
                            while ((tmp = findMonth(t, tmp + 1, i, when.year(calendar))) != -1) {
                                const QDateTime copy(scan.value.addMonths(tmp - finalMonth));
                                if (copy >= minimum && copy <= maximum)
                                    break; // break out of while
                            }
                            if (tmp != -1) {
                                scan.state = Intermediate;
                                done = true;
                            }
                            break;
                        }
                        Q_FALLTHROUGH();
                    default: {
                        int toMin;
                        int toMax;

                        if (sn.type & TimeSectionMask) {
                            if (scan.value.daysTo(minimum) != 0)
                                break;

                            const QTime time = scan.value.time();
                            toMin = time.msecsTo(minimum.time());
                            if (scan.value.daysTo(maximum) > 0)
                                toMax = -1; // can't get to max
                            else
                                toMax = time.msecsTo(maximum.time());
                        } else {
                            toMin = scan.value.daysTo(minimum);
                            toMax = scan.value.daysTo(maximum);
                        }
                        const int maxChange = sn.maxChange();
                        if (toMin > maxChange) {
                            QDTPDEBUG << "invalid because toMin > maxChange" << toMin
                                      << maxChange << t << scan.value << minimum;
                            scan.state = Invalid;
                            done = true;
                            break;
                        } else if (toMax > maxChange) {
                            toMax = -1; // can't get to max
                        }

                        const int min = getDigit(minimum, i);
                        if (min == -1) {
                            qWarning("QDateTimeParser::parse Internal error 4 (%ls)",
                                     qUtf16Printable(sn.name()));
                            scan.state = Invalid;
                            done = true;
                            break;
                        }

                        int max = toMax != -1 ? getDigit(maximum, i) : absoluteMax(i, scan.value);
                        int pos = position + scan.padded - sn.pos;
                        if (pos < 0 || pos >= t.size())
                            pos = -1;
                        if (!potentialValue(t.simplified(), min, max, i, scan.value, pos)) {
                            QDTPDEBUG << "invalid because potentialValue(" << t.simplified() << min << max
                                      << sn.name() << "returned" << toMax << toMin << pos;
                            scan.state = Invalid;
                            done = true;
                            break;
                        }
                        scan.state = Intermediate;
                        done = true;
                        break; }
                    }
                }
            }
        } else {
            if (context == FromString) {
                // optimization
                Q_ASSERT(maximum.date().toJulianDay() == 5373484);
                if (scan.value.date().toJulianDay() > 5373484)
                    scan.state = Invalid;
            } else if (scan.value > maximum) {
                scan.state = Invalid;
            }

            QDTPDEBUG << "not checking intermediate because scanned value is" << scan.value << minimum << maximum;
        }
    }

    /*
        We might have ended up with an invalid datetime: the non-existent hour
        during dst changes, for instance.
    */
    if (!scan.value.isValid() && scan.state == Acceptable)
        scan.state = Intermediate;

    return scan;
}

/*
  \internal
  \brief Returns the index in \a entries with the best prefix match to \a text

  Scans \a entries looking for an entry overlapping \a text as much as possible
  (an exact match beats any prefix match; a match of the full entry as prefix of
  text beats any entry but one matching a longer prefix; otherwise, the match of
  longest prefix wins, earlier entries beating later on a draw).  Records the
  length of overlap in *used (if \a used is non-NULL) and the first entry that
  overlapped this much in *usedText (if \a usedText is non-NULL).
 */
static int findTextEntry(const QString &text, const ShortVector<QString> &entries, QString *usedText, int *used)
{
    if (text.isEmpty())
        return -1;

    int bestMatch = -1;
    int bestCount = 0;
    for (int n = 0; n < entries.size(); ++n)
    {
        const QString &name = entries.at(n);

        const int limit = qMin(text.size(), name.size());
        int i = 0;
        while (i < limit && text.at(i) == name.at(i).toLower())
            ++i;
        // Full match beats an equal prefix match:
        if (i > bestCount || (i == bestCount && i == name.size())) {
            bestCount = i;
            bestMatch = n;
            if (i == name.size() && i == text.size())
                break; // Exact match, name == text, wins.
        }
    }
    if (usedText && bestMatch != -1)
        *usedText = entries.at(bestMatch);
    if (used)
        *used = bestCount;

    return bestMatch;
}

/*!
  \internal
  finds the first possible monthname that \a str1 can
  match. Starting from \a index; str should already by lowered
*/

int QDateTimeParser::findMonth(const QString &str1, int startMonth, int sectionIndex,
                               int year, QString *usedMonth, int *used) const
{
    const SectionNode &sn = sectionNode(sectionIndex);
    if (sn.type != MonthSection) {
        qWarning("QDateTimeParser::findMonth Internal error");
        return -1;
    }

    QLocale::FormatType type = sn.count == 3 ? QLocale::ShortFormat : QLocale::LongFormat;
    QLocale l = locale();
    ShortVector<QString> monthNames;
    monthNames.reserve(13 - startMonth);
    for (int month = startMonth; month <= 12; ++month)
        monthNames.append(calendar.monthName(l, month, year, type));

    const int index = findTextEntry(str1, monthNames, usedMonth, used);
    return index < 0 ? index : index + startMonth;
}

int QDateTimeParser::findDay(const QString &str1, int startDay, int sectionIndex, QString *usedDay, int *used) const
{
    const SectionNode &sn = sectionNode(sectionIndex);
    if (!(sn.type & DaySectionMask)) {
        qWarning("QDateTimeParser::findDay Internal error");
        return -1;
    }

    QLocale::FormatType type = sn.count == 4 ? QLocale::LongFormat : QLocale::ShortFormat;
    QLocale l = locale();
    ShortVector<QString> daysOfWeek;
    daysOfWeek.reserve(8 - startDay);
    for (int day = startDay; day <= 7; ++day)
        daysOfWeek.append(l.dayName(day, type));

    const int index = findTextEntry(str1, daysOfWeek, usedDay, used);
    return index < 0 ? index : index + startDay;
}

/*!
  \internal

  Return's .value is UTC offset in seconds.
  The caller must verify that the offset is within a valid range.
  The mode is 1 for permissive parsing, 2 and 3 for strict offset-only format
  (no UTC prefix) with no colon for 2 and a colon for 3.
 */
QDateTimeParser::ParsedSection QDateTimeParser::findUtcOffset(QStringView str, int mode) const
{
    Q_ASSERT(mode > 0 && mode < 4);
    const bool startsWithUtc = str.startsWith("UTC"_L1);
    // Deal with UTC prefix if present:
    if (startsWithUtc) {
        if (mode != 1)
            return ParsedSection();
        str = str.sliced(3);
        if (str.isEmpty())
            return ParsedSection(Acceptable, 0, 3);
    }

    const bool negativeSign = str.startsWith(u'-');
    // Must start with a sign:
    if (!negativeSign && !str.startsWith(u'+'))
        return ParsedSection();
    str = str.sliced(1);  // drop sign

    const int colonPosition = str.indexOf(u':');
    // Colon that belongs to offset is at most at position 2 (hh:mm)
    bool hasColon = (colonPosition >= 0 && colonPosition < 3);

    // We deal only with digits at this point (except ':'), so collect them
    const int digits = hasColon ? colonPosition + 3 : 4;
    int i = 0;
    for (const int offsetLength = qMin(qsizetype(digits), str.size()); i < offsetLength; ++i) {
        if (i != colonPosition && !str.at(i).isDigit())
            break;
    }
    const int hoursLength = qMin(i, hasColon ? colonPosition : 2);
    if (hoursLength < 1)
        return ParsedSection();
    // Field either ends with hours or also has two digits of minutes
    if (i < digits) {
        // Only allow single-digit hours with UTC prefix or :mm suffix
        if (!startsWithUtc && hoursLength != 2)
            return ParsedSection();
        i = hoursLength;
        hasColon = false;
    }
    if (mode == (hasColon ? 2 : 3))
        return ParsedSection();
    str.truncate(i);  // The rest of the string is not part of the UTC offset

    bool isInt = false;
    const int hours = str.first(hoursLength).toInt(&isInt);
    if (!isInt)
        return ParsedSection();
    const QStringView minutesStr = str.mid(hasColon ? colonPosition + 1 : 2, 2);
    const int minutes = minutesStr.isEmpty() ? 0 : minutesStr.toInt(&isInt);
    if (!isInt)
        return ParsedSection();

    // Keep in sync with QTimeZone::maxUtcOffset hours (14 at most). Also, user
    // could be in the middle of updating the offset (e.g. UTC+14:23) which is
    // an intermediate state
    const State status = (hours > 14 || minutes >= 60) ? Invalid
                         : (hours == 14 && minutes > 0) ? Intermediate : Acceptable;

    int offset = 3600 * hours + 60 * minutes;
    if (negativeSign)
        offset = -offset;

    // Used: UTC, sign, hours, colon, minutes
    const int usedSymbols = (startsWithUtc ? 3 : 0) + 1 + hoursLength + (hasColon ? 1 : 0)
                            + minutesStr.size();

    return ParsedSection(status, offset, usedSymbols);
}

/*!
  \internal

  Return's .value is zone's offset, zone time - UTC time, in seconds.
  The caller must verify that the offset is within a valid range.
  See QTimeZonePrivate::isValidId() for the format of zone names.
 */
QDateTimeParser::ParsedSection
QDateTimeParser::findTimeZoneName(QStringView str, const QDateTime &when) const
{
    const int systemLength = startsWithLocalTimeZone(str, when);
#if QT_CONFIG(timezone)
    // Collect up plausibly-valid characters; let QTimeZone work out what's
    // truly valid.
    const auto invalidZoneNameCharacter = [] (const QChar &c) {
        const auto cu = c.unicode();
        return cu >= 127u || !(memchr("+-./:_", char(cu), 6) || c.isLetterOrNumber());
    };
    int index = std::distance(str.cbegin(),
                              std::find_if(str.cbegin(), str.cend(), invalidZoneNameCharacter));

    // Limit name fragments (between slashes) to 20 characters.
    // (Valid time-zone IDs are allowed up to 14 and Android has quirks up to 17.)
    // Limit number of fragments to six; no known zone name has more than four.
    int lastSlash = -1;
    int count = 0;
    Q_ASSERT(index <= str.size());
    while (lastSlash < index) {
        int slash = str.indexOf(u'/', lastSlash + 1);
        if (slash < 0 || slash > index)
            slash = index; // i.e. the end of the candidate text
        else if (++count > 5)
            index = slash; // Truncate
        if (slash - lastSlash > 20)
            index = lastSlash + 20; // Truncate
        // If any of those conditions was met, index <= slash, so this exits the loop:
        lastSlash = slash;
    }

    for (; index > systemLength; --index) {  // Find longest match
        str.truncate(index);
        QTimeZone zone(str.toLatin1());
        if (zone.isValid())
            return ParsedSection(Acceptable, zone.offsetFromUtc(when), index);
    }
#endif
    if (systemLength > 0)  // won't actually use the offset, but need it to be valid
        return ParsedSection(Acceptable, when.toLocalTime().offsetFromUtc(), systemLength);
    return ParsedSection();
}

/*!
  \internal

  Return's .value is zone's offset, zone time - UTC time, in seconds.
  See QTimeZonePrivate::isValidId() for the format of zone names.

  The mode is the number of 't' characters in the field specifier:
  * 1: any recognized format
  * 2: only the simple offset format, without colon
  * 3: only the simple offset format, with colon
  * 4: only a zone name
*/
QDateTimeParser::ParsedSection
QDateTimeParser::findTimeZone(QStringView str, const QDateTime &when,
                              int maxVal, int minVal, int mode) const
{
    Q_ASSERT(mode > 0 && mode <= 4);
    // Short-cut Zulu suffix when it's all there is (rather than a prefix match):
    if (mode == 1 && str == u'Z')
        return ParsedSection(Acceptable, 0, 1);

    ParsedSection section;
    if (mode != 4)
        section = findUtcOffset(str, mode);
    if (mode != 2 && mode != 3 && section.used <= 0)  // if nothing used, try time zone parsing
        section = findTimeZoneName(str, when);
    // It can be a well formed time zone specifier, but with value out of range
    if (section.state == Acceptable && (section.value < minVal || section.value > maxVal))
        section.state = Intermediate;
    if (section.used > 0)
        return section;

    if (mode == 1) {
        // Check if string is UTC or alias to UTC, after all other options
        if (str.startsWith("UTC"_L1))
            return ParsedSection(Acceptable, 0, 3);
        if (str.startsWith(u'Z'))
            return ParsedSection(Acceptable, 0, 1);
    }

    return ParsedSection();
}

/*!
  \internal

  Compares str to the am/pm texts returned by getAmPmText().
  Returns AM or PM if str is one of those texts. Failing that, it looks to see
  whether, ignoring spaces and case, each character of str appears in one of
  the am/pm texts.
  If neither text can be the result of the user typing more into str, returns
  Neither. If both texts are possible results of further typing, returns
  PossibleBoth. Otherwise, only one of them is a possible completion, so this
  returns PossibleAM or PossiblePM to indicate which.

  \sa getAmPmText()
*/
QDateTimeParser::AmPmFinder QDateTimeParser::findAmPm(QString &str, int sectionIndex, int *used) const
{
    const SectionNode &s = sectionNode(sectionIndex);
    if (s.type != AmPmSection) {
        qWarning("QDateTimeParser::findAmPm Internal error");
        return Neither;
    }
    if (used)
        *used = str.size();
    if (QStringView(str).trimmed().isEmpty())
        return PossibleBoth;

    const QLatin1Char space(' ');
    int size = sectionMaxSize(sectionIndex);

    enum {
        amindex = 0,
        pmindex = 1
    };
    QString ampm[2];
    ampm[amindex] = getAmPmText(AmText, Case(s.count));
    ampm[pmindex] = getAmPmText(PmText, Case(s.count));
    for (int i = 0; i < 2; ++i)
        ampm[i].truncate(size);

    QDTPDEBUG << "findAmPm" << str << ampm[0] << ampm[1];

    if (str.startsWith(ampm[amindex], Qt::CaseInsensitive)) {
        str = ampm[amindex];
        return AM;
    } else if (str.startsWith(ampm[pmindex], Qt::CaseInsensitive)) {
        str = ampm[pmindex];
        return PM;
    } else if (context == FromString || (str.count(space) == 0 && str.size() >= size)) {
        return Neither;
    }
    size = qMin(size, str.size());

    bool broken[2] = {false, false};
    for (int i=0; i<size; ++i) {
        const QChar ch = str.at(i);
        if (ch != space) {
            for (int j=0; j<2; ++j) {
                if (!broken[j]) {
                    int index = ampm[j].indexOf(ch);
                    QDTPDEBUG << "looking for" << ch
                              << "in" << ampm[j] << "and got" << index;
                    if (index == -1) {
                        if (ch.category() == QChar::Letter_Uppercase) {
                            index = ampm[j].indexOf(ch.toLower());
                            QDTPDEBUG << "trying with" << ch.toLower()
                                      << "in" << ampm[j] << "and got" << index;
                        } else if (ch.category() == QChar::Letter_Lowercase) {
                            index = ampm[j].indexOf(ch.toUpper());
                            QDTPDEBUG << "trying with" << ch.toUpper()
                                      << "in" << ampm[j] << "and got" << index;
                        }
                        if (index == -1) {
                            broken[j] = true;
                            if (broken[amindex] && broken[pmindex]) {
                                QDTPDEBUG << str << "didn't make it";
                                return Neither;
                            }
                            continue;
                        } else {
                            str[i] = ampm[j].at(index); // fix case
                        }
                    }
                    ampm[j].remove(index, 1);
                }
            }
        }
    }
    if (!broken[pmindex] && !broken[amindex])
        return PossibleBoth;
    return (!broken[amindex] ? PossibleAM : PossiblePM);
}

/*!
  \internal
  Max number of units that can be changed by this section.
*/

int QDateTimeParser::SectionNode::maxChange() const
{
    switch (type) {
        // Time. unit is msec
    case MSecSection: return 999;
    case SecondSection: return 59 * 1000;
    case MinuteSection: return 59 * 60 * 1000;
    case Hour24Section: case Hour12Section: return 59 * 60 * 60 * 1000;

        // Date. unit is day
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong: return 7;
    case DaySection: return 30;
    case MonthSection: return 365 - 31;
    case YearSection: return 9999 * 365;
    case YearSection2Digits: return 100 * 365;
    default:
        qWarning("QDateTimeParser::maxChange() Internal error (%ls)",
                 qUtf16Printable(name()));
    }

    return -1;
}

QDateTimeParser::FieldInfo QDateTimeParser::fieldInfo(int index) const
{
    FieldInfo ret;
    const SectionNode &sn = sectionNode(index);
    switch (sn.type) {
    case MSecSection:
        ret |= Fraction;
        Q_FALLTHROUGH();
    case SecondSection:
    case MinuteSection:
    case Hour24Section:
    case Hour12Section:
    case YearSection2Digits:
        ret |= AllowPartial;
        Q_FALLTHROUGH();
    case YearSection:
        ret |= Numeric;
        if (sn.count != 1)
            ret |= FixedWidth;
        break;
    case MonthSection:
    case DaySection:
        switch (sn.count) {
        case 2:
            ret |= FixedWidth;
            Q_FALLTHROUGH();
        case 1:
            ret |= (Numeric|AllowPartial);
            break;
        }
        break;
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (sn.count == 3)
            ret |= FixedWidth;
        break;
    case AmPmSection:
        // Some locales have different length AM and PM texts.
        if (getAmPmText(AmText, Case(sn.count)).size()
            == getAmPmText(PmText, Case(sn.count)).size()) {
            // Only relevant to DateTimeEdit's fixups in parse().
            ret |= FixedWidth;
        }
        break;
    case TimeZoneSection:
        break;
    default:
        qWarning("QDateTimeParser::fieldInfo Internal error 2 (%d %ls %d)",
                 index, qUtf16Printable(sn.name()), sn.count);
        break;
    }
    return ret;
}

QString QDateTimeParser::SectionNode::format() const
{
    QChar fillChar;
    switch (type) {
    case AmPmSection: return count == 1 ? "ap"_L1 : count == 2 ? "AP"_L1 : "Ap"_L1;
    case MSecSection: fillChar = u'z'; break;
    case SecondSection: fillChar = u's'; break;
    case MinuteSection: fillChar = u'm'; break;
    case Hour24Section: fillChar = u'H'; break;
    case Hour12Section: fillChar = u'h'; break;
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
    case DaySection: fillChar = u'd'; break;
    case MonthSection: fillChar = u'M'; break;
    case YearSection2Digits:
    case YearSection: fillChar = u'y'; break;
    default:
        qWarning("QDateTimeParser::sectionFormat Internal error (%ls)",
                 qUtf16Printable(name(type)));
        return QString();
    }
    if (fillChar.isNull()) {
        qWarning("QDateTimeParser::sectionFormat Internal error 2");
        return QString();
    }
    return QString(count, fillChar);
}


/*!
  \internal

  Returns \c true if str can be modified to represent a
  number that is within min and max.
*/

bool QDateTimeParser::potentialValue(QStringView str, int min, int max, int index,
                                     const QDateTime &currentValue, int insert) const
{
    if (str.isEmpty())
        return true;

    const int size = sectionMaxSize(index);
    int val = (int)locale().toUInt(str);
    const SectionNode &sn = sectionNode(index);
    if (sn.type == YearSection2Digits) {
        const int year = currentValue.date().year(calendar);
        val += year - (year % 100);
    }
    if (val >= min && val <= max && str.size() == size)
        return true;
    if (val > max || (str.size() == size && val < min))
        return false;

    const int len = size - str.size();
    for (int i=0; i<len; ++i) {
        for (int j=0; j<10; ++j) {
            if (potentialValue(str + QLatin1Char('0' + j), min, max, index, currentValue, insert)) {
                return true;
            } else if (insert >= 0) {
                const QString tmp = str.left(insert) + QLatin1Char('0' + j) + str.mid(insert);
                if (potentialValue(tmp, min, max, index, currentValue, insert))
                    return true;
            }
        }
    }

    return false;
}

/*!
  \internal
*/
bool QDateTimeParser::skipToNextSection(int index, const QDateTime &current, QStringView text) const
{
    Q_ASSERT(text.size() < sectionMaxSize(index));
    const SectionNode &node = sectionNode(index);
    int min = absoluteMin(index);
    int max = absoluteMax(index, current);
    // Time-zone field is only numeric if given as offset from UTC:
    if (node.type != TimeZoneSection || current.timeSpec() == Qt::OffsetFromUTC) {
        const QDateTime maximum = getMaximum();
        const QDateTime minimum = getMinimum();
        Q_ASSERT(current >= minimum && current <= maximum);

        QDateTime tmp = current;
        if (!setDigit(tmp, index, min) || tmp < minimum)
            min = getDigit(minimum, index);

        if (!setDigit(tmp, index, max) || tmp > maximum)
            max = getDigit(maximum, index);
    }
    int pos = cursorPosition() - node.pos;
    if (pos < 0 || pos >= text.size())
        pos = -1;

    /*
      If the value potentially can become another valid entry we don't want to
      skip to the next. E.g. In a M field (month without leading 0) if you type
      1 we don't want to autoskip (there might be [012] following) but if you
      type 3 we do.
    */
    return !potentialValue(text, min, max, index, current, pos);
}

/*!
  \internal
  For debugging. Returns the name of the section \a s.
*/

QString QDateTimeParser::SectionNode::name(QDateTimeParser::Section s)
{
    switch (s) {
    case QDateTimeParser::AmPmSection: return "AmPmSection"_L1;
    case QDateTimeParser::DaySection: return "DaySection"_L1;
    case QDateTimeParser::DayOfWeekSectionShort: return "DayOfWeekSectionShort"_L1;
    case QDateTimeParser::DayOfWeekSectionLong: return "DayOfWeekSectionLong"_L1;
    case QDateTimeParser::Hour24Section: return "Hour24Section"_L1;
    case QDateTimeParser::Hour12Section: return "Hour12Section"_L1;
    case QDateTimeParser::MSecSection: return "MSecSection"_L1;
    case QDateTimeParser::MinuteSection: return "MinuteSection"_L1;
    case QDateTimeParser::MonthSection: return "MonthSection"_L1;
    case QDateTimeParser::SecondSection: return "SecondSection"_L1;
    case QDateTimeParser::TimeZoneSection: return "TimeZoneSection"_L1;
    case QDateTimeParser::YearSection: return "YearSection"_L1;
    case QDateTimeParser::YearSection2Digits: return "YearSection2Digits"_L1;
    case QDateTimeParser::NoSection: return "NoSection"_L1;
    case QDateTimeParser::FirstSection: return "FirstSection"_L1;
    case QDateTimeParser::LastSection: return "LastSection"_L1;
    default: return "Unknown section "_L1 + QString::number(int(s));
    }
}

/*!
  \internal
  For debugging. Returns the name of the state \a s.
*/

QString QDateTimeParser::stateName(State s) const
{
    switch (s) {
    case Invalid: return "Invalid"_L1;
    case Intermediate: return "Intermediate"_L1;
    case Acceptable: return "Acceptable"_L1;
    default: return "Unknown state "_L1 + QString::number(s);
    }
}

// Only called when we want only one of date or time; use UTC to avoid bogus DST issues.
bool QDateTimeParser::fromString(const QString &t, QDate *date, QTime *time) const
{
    QDateTime val(QDate(1900, 1, 1).startOfDay(QTimeZone::UTC));
    const StateNode tmp = parse(t, -1, val, false);
    if (tmp.state != Acceptable || tmp.conflicts)
        return false;

    if (time) {
        Q_ASSERT(!date);
        const QTime t = tmp.value.time();
        if (!t.isValid())
            return false;
        *time = t;
    }

    if (date) {
        Q_ASSERT(!time);
        const QDate d = tmp.value.date();
        if (!d.isValid())
            return false;
        *date = d;
    }
    return true;
}

// Only called when we want both date and time; default to local time.
bool QDateTimeParser::fromString(const QString &t, QDateTime *datetime) const
{
    static const QDateTime defaultLocalTime = QDate(1900, 1, 1).startOfDay();
    const StateNode tmp = parse(t, -1, defaultLocalTime, false);
    if (datetime)
        *datetime = tmp.value;
    return tmp.state == Acceptable && !tmp.conflicts && tmp.value.isValid();
}

QDateTime QDateTimeParser::getMinimum() const
{
    // NB: QDateTimeParser always uses Qt::LocalTime time spec by default. If
    //     any subclass needs a changing time spec, it must override this
    //     method. At the time of writing, this is done by QDateTimeEditPrivate.

    // Cache the only case
    static const QDateTime localTimeMin(QDATETIMEEDIT_DATE_MIN.startOfDay());
    return localTimeMin;
}

QDateTime QDateTimeParser::getMaximum() const
{
    // NB: QDateTimeParser always uses Qt::LocalTime time spec by default. If
    //     any subclass needs a changing time spec, it must override this
    //     method. At the time of writing, this is done by QDateTimeEditPrivate.

    // Cache the only case
    static const QDateTime localTimeMax(QDATETIMEEDIT_DATE_MAX.endOfDay());
    return localTimeMax;
}

QString QDateTimeParser::getAmPmText(AmPm ap, Case cs) const
{
    const QLocale loc = locale();
    QString raw = ap == AmText ? loc.amText() : loc.pmText();
    switch (cs)
    {
    case UpperCase: return raw.toUpper();
    case LowerCase: return raw.toLower();
    case NativeCase: return raw;
    }
    Q_UNREACHABLE_RETURN(raw);
}

/*
  \internal

  I give arg2 preference because arg1 is always a QDateTime.
*/

bool operator==(const QDateTimeParser::SectionNode &s1, const QDateTimeParser::SectionNode &s2)
{
    return (s1.type == s2.type) && (s1.pos == s2.pos) && (s1.count == s2.count);
}

/*!
  Sets \a cal as the calendar to use. The default is Gregorian.
*/

void QDateTimeParser::setCalendar(const QCalendar &cal)
{
    calendar = cal;
}

QT_END_NAMESPACE
