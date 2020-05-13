/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformdefs.h"
#include "private/qdatetimeparser_p.h"

#include "qdatastream.h"
#include "qset.h"
#include "qlocale.h"
#include "qdatetime.h"
#if QT_CONFIG(timezone)
#include "qtimezone.h"
#endif
#include "qdebug.h"

//#define QDATETIMEPARSER_DEBUG
#if defined (QDATETIMEPARSER_DEBUG) && !defined(QT_NO_DEBUG_STREAM)
#  define QDTPDEBUG qDebug()
#  define QDTPDEBUGN qDebug
#else
#  define QDTPDEBUG if (false) qDebug()
#  define QDTPDEBUGN if (false) qDebug
#endif

QT_BEGIN_NAMESPACE

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
#if QT_CONFIG(datestring)
        qWarning("QDateTimeParser::getDigit() Internal error (%ls %d)",
                 qUtf16Printable(t.toString()), index);
#else
        qWarning("QDateTimeParser::getDigit() Internal error (%d)", index);
#endif
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
    case DayOfWeekSectionLong: return t.date().day(calendar);
    case AmPmSection: return t.time().hour() > 11 ? 1 : 0;

    default: break;
    }

#if QT_CONFIG(datestring)
    qWarning("QDateTimeParser::getDigit() Internal error 2 (%ls %d)",
             qUtf16Printable(t.toString()), index);
#else
    qWarning("QDateTimeParser::getDigit() Internal error 2 (%d)", index);
#endif
    return -1;
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
#if QT_CONFIG(datestring)
        qWarning("QDateTimeParser::setDigit() Internal error (%ls %d %d)",
                 qUtf16Printable(v.toString()), index, newVal);
#else
        qWarning("QDateTimeParser::setDigit() Internal error (%d %d)", index, newVal);
#endif
        return false;
    }

    QCalendar::YearMonthDay date = calendar.partsFromDate(v.date());
    if (!date.isValid())
        return false;

    const QTime time = v.time();
    int hour = time.hour();
    int minute = time.minute();
    int second = time.second();
    int msec = time.msec();
    Qt::TimeSpec tspec = v.timeSpec();
    // Only offset from UTC is amenable to setting an int value:
    int offset = tspec == Qt::OffsetFromUTC ? v.offsetFromUtc() : 0;

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
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (newVal > 31) {
            // have to keep legacy behavior. setting the
            // date to 32 should return false. Setting it
            // to 31 for february should return true
            return false;
        }
        date.day = newVal;
        break;
    case TimeZoneSection:
        if (newVal < absoluteMin(index) || newVal > absoluteMax(index))
            return false;
        tspec = Qt::OffsetFromUTC;
        offset = newVal;
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
        const int max = calendar.daysInMonth(date.month, date.year);
        if (date.day > max)
            date.day = max;
    }

    const QDate newDate = calendar.dateFromParts(date);
    const QTime newTime(hour, minute, second, msec);
    if (!newDate.isValid() || !newTime.isValid())
        return false;

    // Preserve zone:
    v =
#if QT_CONFIG(timezone)
         tspec == Qt::TimeZone ? QDateTime(newDate, newTime, v.timeZone()) :
#endif
         QDateTime(newDate, newTime, tspec, offset);
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
#if QT_CONFIG(timezone)
        return QTimeZone::MaxUtcOffsetSecs;
#else
        return +14 * 3600;  // NB: copied from QTimeZone
#endif
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
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        return cur.isValid() ? cur.date().daysInMonth(calendar) : calendar.maximumDaysInMonth();
    case AmPmSection:
        return 1;
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
#if QT_CONFIG(timezone)
        return QTimeZone::MinUtcOffsetSecs;
#else
        return -14 * 3600;  // NB: copied from QTimeZone
#endif
    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case MSecSection:
    case YearSection2Digits:
    case YearSection: return 0;
    case MonthSection:
    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong: return 1;
    case AmPmSection: return 0;
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

  helper function for parseFormat. removes quotes that are
  not escaped and removes the escaping on those that are escaped

*/

static QString unquote(const QStringRef &str)
{
    const QChar quote(QLatin1Char('\''));
    const QChar slash(QLatin1Char('\\'));
    const QChar zero(QLatin1Char('0'));
    QString ret;
    QChar status(zero);
    const int max = str.size();
    for (int i=0; i<max; ++i) {
        if (str.at(i) == quote) {
            if (status != quote) {
                status = quote;
            } else if (!ret.isEmpty() && str.at(i - 1) == slash) {
                ret[ret.size() - 1] = quote;
            } else {
                status = zero;
            }
        } else {
            ret += str.at(i);
        }
    }
    return ret;
}

static inline int countRepeat(const QString &str, int index, int maxCount)
{
    int count = 1;
    const QChar ch(str.at(index));
    const int max = qMin(index + maxCount, str.size());
    while (index + count < max && str.at(index + count) == ch) {
        ++count;
    }
    return count;
}

static inline void appendSeparator(QStringList *list, const QString &string, int from, int size, int lastQuote)
{
    const QStringRef separator = string.midRef(from, size);
    list->append(lastQuote >= from ? unquote(separator) : separator.toString());
}

/*!
    \internal

    Parses the format \a newFormat. If successful, returns \c true and sets up
    the format. Else keeps the old format and returns \c false.
*/
bool QDateTimeParser::parseFormat(const QString &newFormat)
{
    const QLatin1Char quote('\'');
    const QLatin1Char slash('\\');
    const QLatin1Char zero('0');
    if (newFormat == displayFormat && !newFormat.isEmpty()) {
        return true;
    }

    QDTPDEBUGN("parseFormat: %s", newFormat.toLatin1().constData());

    QVector<SectionNode> newSectionNodes;
    Sections newDisplay;
    QStringList newSeparators;
    int i, index = 0;
    int add = 0;
    QChar status(zero);
    const int max = newFormat.size();
    int lastQuote = -1;
    for (i = 0; i<max; ++i) {
        if (newFormat.at(i) == quote) {
            lastQuote = i;
            ++add;
            if (status != quote) {
                status = quote;
            } else if (i > 0 && newFormat.at(i - 1) != slash) {
                status = zero;
            }
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
                    const SectionNode sn = { MSecSection, i - add, countRepeat(newFormat, i, 3) < 3 ? 1 : 3, 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= MSecSection;
                }
                break;
            case 'A':
            case 'a':
                if (parserType != QMetaType::QDate) {
                    const bool cap = (sect == 'A');
                    const SectionNode sn = { AmPmSection, i - add, (cap ? 1 : 0), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    newDisplay |= AmPmSection;
                    if (i + 1 < newFormat.size()
                        && newFormat.at(i+1) == (cap ? QLatin1Char('P') : QLatin1Char('p'))) {
                        ++i;
                    }
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
                    newSeparators.append(unquote(newFormat.midRef(index, i - index)));
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
                    const SectionNode sn = { TimeZoneSection, i - add, countRepeat(newFormat, i, 4), 0 };
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
    if (newSectionNodes.isEmpty() && context == DateTimeEdit) {
        return false;
    }

    if ((newDisplay & (AmPmSection|Hour12Section)) == Hour12Section) {
        const int count = newSectionNodes.size();
        for (int i = 0; i < count; ++i) {
            SectionNode &node = newSectionNodes[i];
            if (node.type == Hour12Section)
                node.type = Hour24Section;
        }
    }

    if (index < max) {
        appendSeparator(&newSeparators, newFormat, index, index - max, lastQuote);
    } else {
        newSeparators.append(QString());
    }

    displayFormat = newFormat;
    separators = newSeparators;
    sectionNodes = newSectionNodes;
    display = newDisplay;
    last.pos = -1;

//     for (int i=0; i<sectionNodes.size(); ++i) {
//         QDTPDEBUG << sectionNodes.at(i).name() << sectionNodes.at(i).count;
//     }

    QDTPDEBUG << newFormat << displayFormat;
    QDTPDEBUGN("separators:\n'%s'", separators.join(QLatin1String("\n")).toLatin1().constData());

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
        if (displayTextSize != text.size()) {
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

    case AmPmSection: {
        const int lowerMax = qMax(getAmPmText(AmText, LowerCase).size(),
                                  getAmPmText(PmText, LowerCase).size());
        const int upperMax = qMax(getAmPmText(AmText, UpperCase).size(),
                                  getAmPmText(PmText, UpperCase).size());
        return qMax(lowerMax, upperMax);
    }

    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case DaySection:
        return 2;

    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
#if !QT_CONFIG(textdate)
        return 2;
#else
        mcount = 7;
        Q_FALLTHROUGH();
#endif
    case MonthSection:
#if !QT_CONFIG(textdate)
        return 2;
#else
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
#endif
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


#if QT_CONFIG(datestring)

QDateTimeParser::ParsedSection
QDateTimeParser::parseSection(const QDateTime &currentValue, int sectionIndex,
                              int offset, QString *text) const
{
    ParsedSection result; // initially Invalid
    const SectionNode &sn = sectionNode(sectionIndex);
    if (sn.type & Internal) {
        qWarning("QDateTimeParser::parseSection Internal error (%ls %d)",
                 qUtf16Printable(sn.name()), sectionIndex);
        return result;
    }

    const int sectionmaxsize = sectionMaxSize(sectionIndex);
    QStringRef sectionTextRef = text->midRef(offset, sectionmaxsize);

    QDTPDEBUG << "sectionValue for" << sn.name()
              << "with text" << *text << "and (at" << offset
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
            text->replace(offset, used, sectiontext.constData(), used);
        break; }
    case TimeZoneSection:
        result = findTimeZone(sectionTextRef, currentValue,
                              absoluteMax(sectionIndex),
                              absoluteMin(sectionIndex));
        break;
    case MonthSection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (sn.count >= 3) {
            QString sectiontext = sectionTextRef.toString();
            int num = 0, used = 0;
            if (sn.type == MonthSection) {
                const QDate minDate = getMinimum().date();
                const int year = currentValue.date().year(calendar);
                const int min = (year == minDate.year(calendar)) ? minDate.month(calendar) : 1;
                num = findMonth(sectiontext.toLower(), min, sectionIndex, year, &sectiontext, &used);
            } else {
                num = findDay(sectiontext.toLower(), 1, sectionIndex, &sectiontext, &used);
            }

            result = ParsedSection(Intermediate, num, used);
            if (num != -1) {
                text->replace(offset, used, sectiontext.constData(), used);
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
        int sectiontextSize = sectionTextRef.size();
        if (sectiontextSize == 0) {
            result = ParsedSection(Intermediate);
        } else {
            for (int i = 0; i < sectiontextSize; ++i) {
                if (sectionTextRef.at(i).isSpace())
                    sectiontextSize = i; // which exits the loop
            }

            const int absMax = absoluteMax(sectionIndex);
            QLocale loc;
            bool ok = true;
            int last = -1, used = -1;

            Q_ASSERT(sectiontextSize <= sectionmaxsize);
            QStringRef digitsStr = sectionTextRef.left(sectiontextSize);
            for (int digits = sectiontextSize; digits >= 1; --digits) {
                digitsStr.truncate(digits);
                int tmp = (int)loc.toUInt(digitsStr, &ok);
                if (ok && sn.type == Hour12Section) {
                    if (tmp > 12) {
                        tmp = -1;
                        ok = false;
                    } else if (tmp == 12) {
                        tmp = 0;
                    }
                }
                if (ok && tmp <= absMax) {
                    QDTPDEBUG << sectionTextRef.left(digits) << tmp << digits;
                    last = tmp;
                    used = digits;
                    break;
                }
            }

            if (last == -1) {
                QChar first(sectionTextRef.at(0));
                if (separators.at(sectionIndex + 1).startsWith(first))
                    result = ParsedSection(Intermediate, 0, used);
                else
                    QDTPDEBUG << "invalid because" << sectionTextRef << "can't become a uint" << last << ok;
            } else {
                const FieldInfo fi = fieldInfo(sectionIndex);
                const bool unfilled = used < sectionmaxsize;
                if (unfilled && fi & Fraction) { // typing 2 in a zzz field should be .200, not .002
                    for (int i = used; i < sectionmaxsize; ++i)
                        last *= 10;
                }
                // Even those *= 10s can't take last above absMax:
                Q_ASSERT(last <= absMax);
                const int absMin = absoluteMin(sectionIndex);
                if (last < absMin) {
                    if (unfilled)
                        result = ParsedSection(Intermediate, last, used);
                    else
                        QDTPDEBUG << "invalid because" << last << "is less than absoluteMin" << absMin;
                } else if (unfilled && (fi & (FixedWidth|Numeric)) == (FixedWidth|Numeric)) {
                    if (skipToNextSection(sectionIndex, currentValue, digitsStr)) {
                        const int missingZeroes = sectionmaxsize - digitsStr.size();
                        result = ParsedSection(Acceptable, last, sectionmaxsize, missingZeroes);
                        text->insert(offset, QString(missingZeroes, QLatin1Char('0')));
                        ++(const_cast<QDateTimeParser*>(this)->sectionNodes[sectionIndex].zeroesAdded);
                    } else {
                        result = ParsedSection(Intermediate, last, used);;
                    }
                } else {
                    result = ParsedSection(Acceptable, last, used);
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

  Returns a day-number, in the same month as \a rough and as close to \a rough's
  day number as is valid, that \a calendar puts on the day of the week indicated
  by \a weekDay.
*/

static int weekDayWithinMonth(QCalendar calendar, QDate rough, int weekDay)
{
    // TODO: can we adapt this to cope gracefully with intercallary days (day of
    // week > 7) without making it slower for more widely-used calendars ?
    int day = rough.day(calendar) + weekDay - calendar.dayOfWeek(rough);
    if (day <= 0)
        return day + 7;
    if (day > rough.daysInMonth(calendar))
        return day - 7;
    return day;
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

    if (dayofweek < 1 || dayofweek > 7) // Invalid: ignore
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
    int last = known & QDateTimeParser::YearSection && known & QDateTimeParser::MonthSection
        ? first.daysInMonth(calendar) : 0;
    // If we also know day-of-week, tweak last to the last in the month that matches it:
    if (last && known & QDateTimeParser::DayOfWeekSectionMask) {
        int diff = (dayofweek - calendar.dayOfWeek(first) - last) % 7;
        Q_ASSERT(diff <= 0); // C++11 specifies (-ve) % (+ve) to be <= 0.
        last += diff;
    }
    if (day < 1) {
        if (known & QDateTimeParser::DayOfWeekSectionMask && last) {
            day = 1 + dayofweek - calendar.dayOfWeek(first);
            if (day < 1)
                day += 7;
        } else {
            day = 1;
        }
        known &= ~QDateTimeParser::DaySection;
    } else if (day > 31) {
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
        day = weekDayWithinMonth(calendar, actual, dayofweek);
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

/*!
  \internal
*/
QDateTimeParser::StateNode
QDateTimeParser::scanString(const QDateTime &defaultValue,
                            bool fixup, QString *input) const
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
    Qt::TimeSpec tspec = defaultValue.timeSpec();
    int zoneOffset = 0; // In seconds; local - UTC
#if QT_CONFIG(timezone)
    QTimeZone timeZone;
#endif
    switch (tspec) {
    case Qt::OffsetFromUTC: // timeZone is ignored
        zoneOffset = defaultValue.offsetFromUtc();
        break;
#if QT_CONFIG(timezone)
    case Qt::TimeZone:
        timeZone = defaultValue.timeZone();
        if (timeZone.isValid())
            zoneOffset = timeZone.offsetFromUtc(defaultValue);
        // else: is there anything we can do about this ?
        break;
#endif
    default: // zoneOffset and timeZone are ignored
        break;
    }

    int ampm = -1;
    Sections isSet = NoSection;

    for (int index = 0; index < sectionNodesCount; ++index) {
        Q_ASSERT(state != Invalid);
        const QString &separator = separators.at(index);
        if (input->midRef(pos, separator.size()) != separator) {
            QDTPDEBUG << "invalid because" << input->midRef(pos, separator.size())
                      << "!=" << separator
                      << index << pos << currentSectionIndex;
            return StateNode();
        }
        pos += separator.size();
        sectionNodes[index].pos = pos;
        int *current = nullptr;
        const SectionNode sn = sectionNodes.at(index);
        ParsedSection sect;

        {
            const QDate date = actualDate(isSet, calendar, year, year2digits,
                                          month, day, dayofweek);
            const QTime time = actualTime(isSet, hour, hour12, ampm, minute, second, msec);
            sect = parseSection(
#if QT_CONFIG(timezone)
                                tspec == Qt::TimeZone ? QDateTime(date, time, timeZone) :
#endif
                                QDateTime(date, time, tspec, zoneOffset),
                                index, pos, input);
        }

        QDTPDEBUG << "sectionValue" << sn.name() << *input
                  << "pos" << pos << "used" << sect.used << stateName(sect.state);

        padding += sect.zeroes;
        if (fixup && sect.state == Intermediate && sect.used < sn.count) {
            const FieldInfo fi = fieldInfo(index);
            if ((fi & (Numeric|FixedWidth)) == (Numeric|FixedWidth)) {
                const QString newText = QString::fromLatin1("%1").arg(sect.value, sn.count, 10, QLatin1Char('0'));
                input->replace(pos, sect.used, newText);
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
                QStringRef zoneName = input->midRef(pos, sect.used);
                Q_ASSERT(!zoneName.isEmpty()); // sect.used > 0

                const QStringRef offsetStr = zoneName.startsWith(QLatin1String("UTC"))
                                             ? zoneName.mid(3) : zoneName;
                const bool isUtcOffset = offsetStr.startsWith(QLatin1Char('+'))
                                         || offsetStr.startsWith(QLatin1Char('-'));
                const bool isUtc = zoneName == QLatin1String("Z")
                                   || zoneName == QLatin1String("UTC");

                if (isUtc || isUtcOffset) {
                    tspec = sect.value ? Qt::OffsetFromUTC : Qt::UTC;
                } else {
#if QT_CONFIG(timezone)
                    timeZone = QTimeZone(zoneName.toLatin1());
                    tspec = timeZone.isValid()
                        ? Qt::TimeZone
                        : (Q_ASSERT(startsWithLocalTimeZone(zoneName)), Qt::LocalTime);
#else
                    tspec = Qt::LocalTime;
#endif
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
            break;
        }

        if (sect.used > 0)
            pos += sect.used;
        QDTPDEBUG << index << sn.name() << "is set to"
                  << pos << "state is" << stateName(state);

        if (!current) {
            qWarning("QDateTimeParser::parse Internal error 2");
            return StateNode();
        }
        if (isSet & sn.type && *current != sect.value) {
            QDTPDEBUG << "CONFLICT " << sn.name() << *current << sect.value;
            conflicts = true;
            if (index != currentSectionIndex || sect.state == Invalid) {
                continue;
            }
        }
        if (sect.state != Invalid)
            *current = sect.value;

        // Record the present section:
        isSet |= sn.type;
    }

    if (input->midRef(pos) != separators.last()) {
        QDTPDEBUG << "invalid because" << input->midRef(pos)
                  << "!=" << separators.last() << pos;
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

        const QDate date(year, month, day, calendar);
        if (dayofweek != calendar.dayOfWeek(date)
                && state == Acceptable && isSet & DayOfWeekSectionMask) {
            if (isSet & DaySection)
                conflicts = true;
            const SectionNode &sn = sectionNode(currentSectionIndex);
            if (sn.type & DayOfWeekSectionMask || currentSectionIndex == -1) {
                // dayofweek should be preferred
                day = weekDayWithinMonth(calendar, date, dayofweek);
                QDTPDEBUG << year << month << day << dayofweek
                          << calendar.dayOfWeek(QDate(year, month, day, calendar));
            }
        }

        bool needfixday = false;
        if (sectionType(currentSectionIndex) & DaySectionMask) {
            cachedDay = day;
        } else if (cachedDay > day) {
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
            if (context == FromString) {
                return StateNode();
            }
            if (state == Acceptable && fixday) {
                day = qMin<int>(day, calendar.daysInMonth(month, year));

                const QLocale loc = locale();
                for (int i=0; i<sectionNodesCount; ++i) {
                    const SectionNode sn = sectionNode(i);
                    if (sn.type & DaySection) {
                        input->replace(sectionPos(sn), sectionSize(i), loc.toString(day));
                    } else if (sn.type & DayOfWeekSectionMask) {
                        const int dayOfWeek = calendar.dayOfWeek(QDate(year, month, day, calendar));
                        const QLocale::FormatType dayFormat =
                            (sn.type == DayOfWeekSectionShort
                             ? QLocale::ShortFormat : QLocale::LongFormat);
                        const QString dayName(loc.dayName(dayOfWeek, dayFormat));
                        input->replace(sectionPos(sn), sectionSize(i), dayName);
                    }
                }
            } else if (state > Intermediate) {
                state = Intermediate;
            }
        }
    }

    if (parserType != QMetaType::QDate) {
        if (isSet & Hour12Section) {
            const bool hasHour = isSet & Hour24Section;
            if (ampm == -1) {
                if (hasHour) {
                    ampm = (hour < 12 ? 0 : 1);
                } else {
                    ampm = 0; // no way to tell if this is am or pm so I assume am
                }
            }
            hour12 = (ampm == 0 ? hour12 % 12 : (hour12 % 12) + 12);
            if (!hasHour) {
                hour = hour12;
            } else if (hour != hour12) {
                conflicts = true;
            }
        } else if (ampm != -1) {
            if (!(isSet & (Hour24Section))) {
                hour = (12 * ampm); // special case. Only ap section
            } else if ((ampm == 0) != (hour < 12)) {
                conflicts = true;
            }
        }
    }

    QDTPDEBUG << year << month << day << hour << minute << second << msec;
    Q_ASSERT(state != Invalid);

    const QDate date(year, month, day, calendar);
    const QTime time(hour, minute, second, msec);
    const QDateTime when =
#if QT_CONFIG(timezone)
            tspec == Qt::TimeZone ? QDateTime(date, time, timeZone) :
#endif
            QDateTime(date, time, tspec, zoneOffset);

    // If hour wasn't specified, check the default we're using exists on the
    // given date (which might be a spring-forward, skipping an hour).
    if (parserType == QMetaType::QDateTime && !(isSet & HourSectionMask) && !when.isValid()) {
        qint64 msecs = when.toMSecsSinceEpoch();
        // Fortunately, that gets a useful answer, even though when is invalid ...
        const QDateTime replace =
#if QT_CONFIG(timezone)
            tspec == Qt::TimeZone
            ? QDateTime::fromMSecsSinceEpoch(msecs, timeZone) :
#endif
            QDateTime::fromMSecsSinceEpoch(msecs, tspec, zoneOffset);
        const QTime tick = replace.time();
        if (replace.date() == date
            && (!(isSet & MinuteSection) || tick.minute() == minute)
            && (!(isSet & SecondSection) || tick.second() == second)
            && (!(isSet & MSecSection)   || tick.msec() == msec)) {
            return StateNode(replace, state, padding, conflicts);
        }
    }

    return StateNode(when, state, padding, conflicts);
}

/*!
  \internal
*/

QDateTimeParser::StateNode
QDateTimeParser::parse(QString input, int position, const QDateTime &defaultValue, bool fixup) const
{
    const QDateTime minimum = getMinimum();
    const QDateTime maximum = getMaximum();

    QDTPDEBUG << "parse" << input;
    StateNode scan = scanString(defaultValue, fixup, &input);
    QDTPDEBUGN("'%s' => '%s'(%s)", input.toLatin1().constData(),
               scan.value.toString(QLatin1String("yyyy/MM/dd hh:mm:ss.zzz")).toLatin1().constData(),
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
                QString t = sectionText(input, i, sn.pos).toLower();
                if ((t.size() < sectionMaxSize(i)
                     && (((int)fieldInfo(i) & (FixedWidth|Numeric)) != Numeric))
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
                            if (scan.value.daysTo(minimum) != 0) {
                                break;
                            }
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
            } else {
                if (scan.value > maximum)
                    scan.state = Invalid;
            }

            QDTPDEBUG << "not checking intermediate because scanned value is" << scan.value << minimum << maximum;
        }
    }
    text = scan.input = input;

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
 */
QDateTimeParser::ParsedSection QDateTimeParser::findUtcOffset(QStringRef str) const
{
    const bool startsWithUtc = str.startsWith(QLatin1String("UTC"));
    // Get rid of UTC prefix if it exists
    if (startsWithUtc)
        str = str.mid(3);

    const bool negativeSign = str.startsWith(QLatin1Char('-'));
    // Must start with a sign:
    if (!negativeSign && !str.startsWith(QLatin1Char('+')))
        return ParsedSection();
    str = str.mid(1);  // drop sign

    const int colonPosition = str.indexOf(QLatin1Char(':'));
    // Colon that belongs to offset is at most at position 2 (hh:mm)
    bool hasColon = (colonPosition >= 0 && colonPosition < 3);

    // We deal only with digits at this point (except ':'), so collect them
    const int digits = hasColon ? colonPosition + 3 : 4;
    int i = 0;
    for (const int offsetLength = qMin(digits, str.size()); i < offsetLength; ++i) {
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
    str.truncate(i);  // The rest of the string is not part of the UTC offset

    bool isInt = false;
    const int hours = str.mid(0, hoursLength).toInt(&isInt);
    if (!isInt)
        return ParsedSection();
    const QStringRef minutesStr = str.mid(hasColon ? colonPosition + 1 : 2, 2);
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
QDateTimeParser::findTimeZoneName(QStringRef str, const QDateTime &when) const
{
    const int systemLength = startsWithLocalTimeZone(str);
#if QT_CONFIG(timezone)
    // Collect up plausibly-valid characters; let QTimeZone work out what's
    // truly valid.
    const auto invalidZoneNameCharacter = [] (const QChar &c) {
        return c.unicode() >= 127u
               || (!c.isLetterOrNumber() && !QLatin1String("+-./:_").contains(c));
    };
    int index = std::distance(str.cbegin(),
                              std::find_if(str.cbegin(), str.cend(), invalidZoneNameCharacter));

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
 */
QDateTimeParser::ParsedSection
QDateTimeParser::findTimeZone(QStringRef str, const QDateTime &when,
                              int maxVal, int minVal) const
{
    ParsedSection section = findUtcOffset(str);
    if (section.used <= 0)  // if nothing used, try time zone parsing
        section = findTimeZoneName(str, when);
    // It can be a well formed time zone specifier, but with value out of range
    if (section.state == Acceptable && (section.value < minVal || section.value > maxVal))
        section.state = Intermediate;
    if (section.used > 0)
        return section;

    // Check if string is UTC or alias to UTC, after all other options
    if (str.startsWith(QLatin1String("UTC")))
        return ParsedSection(Acceptable, 0, 3);
    if (str.startsWith(QLatin1Char('Z')))
        return ParsedSection(Acceptable, 0, 1);

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
    if (QStringRef(&str).trimmed().isEmpty()) {
        return PossibleBoth;
    }
    const QLatin1Char space(' ');
    int size = sectionMaxSize(sectionIndex);

    enum {
        amindex = 0,
        pmindex = 1
    };
    QString ampm[2];
    ampm[amindex] = getAmPmText(AmText, s.count == 1 ? UpperCase : LowerCase);
    ampm[pmindex] = getAmPmText(PmText, s.count == 1 ? UpperCase : LowerCase);
    for (int i=0; i<2; ++i)
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
        if (str.at(i) != space) {
            for (int j=0; j<2; ++j) {
                if (!broken[j]) {
                    int index = ampm[j].indexOf(str.at(i));
                    QDTPDEBUG << "looking for" << str.at(i)
                              << "in" << ampm[j] << "and got" << index;
                    if (index == -1) {
                        if (str.at(i).category() == QChar::Letter_Uppercase) {
                            index = ampm[j].indexOf(str.at(i).toLower());
                            QDTPDEBUG << "trying with" << str.at(i).toLower()
                                      << "in" << ampm[j] << "and got" << index;
                        } else if (str.at(i).category() == QChar::Letter_Lowercase) {
                            index = ampm[j].indexOf(str.at(i).toUpper());
                            QDTPDEBUG << "trying with" << str.at(i).toUpper()
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
#endif // datestring

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
        ret |= FixedWidth;
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
    case AmPmSection: return count == 1 ? QLatin1String("AP") : QLatin1String("ap");
    case MSecSection: fillChar = QLatin1Char('z'); break;
    case SecondSection: fillChar = QLatin1Char('s'); break;
    case MinuteSection: fillChar = QLatin1Char('m'); break;
    case Hour24Section: fillChar = QLatin1Char('H'); break;
    case Hour12Section: fillChar = QLatin1Char('h'); break;
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
    case DaySection: fillChar = QLatin1Char('d'); break;
    case MonthSection: fillChar = QLatin1Char('M'); break;
    case YearSection2Digits:
    case YearSection: fillChar = QLatin1Char('y'); break;
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

bool QDateTimeParser::potentialValue(const QStringRef &str, int min, int max, int index,
                                     const QDateTime &currentValue, int insert) const
{
    if (str.isEmpty()) {
        return true;
    }
    const int size = sectionMaxSize(index);
    int val = (int)locale().toUInt(str);
    const SectionNode &sn = sectionNode(index);
    if (sn.type == YearSection2Digits) {
        const int year = currentValue.date().year(calendar);
        val += year - (year % 100);
    }
    if (val >= min && val <= max && str.size() == size) {
        return true;
    } else if (val > max) {
        return false;
    } else if (str.size() == size && val < min) {
        return false;
    }

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
bool QDateTimeParser::skipToNextSection(int index, const QDateTime &current, const QStringRef &text) const
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
    case QDateTimeParser::AmPmSection: return QLatin1String("AmPmSection");
    case QDateTimeParser::DaySection: return QLatin1String("DaySection");
    case QDateTimeParser::DayOfWeekSectionShort: return QLatin1String("DayOfWeekSectionShort");
    case QDateTimeParser::DayOfWeekSectionLong: return QLatin1String("DayOfWeekSectionLong");
    case QDateTimeParser::Hour24Section: return QLatin1String("Hour24Section");
    case QDateTimeParser::Hour12Section: return QLatin1String("Hour12Section");
    case QDateTimeParser::MSecSection: return QLatin1String("MSecSection");
    case QDateTimeParser::MinuteSection: return QLatin1String("MinuteSection");
    case QDateTimeParser::MonthSection: return QLatin1String("MonthSection");
    case QDateTimeParser::SecondSection: return QLatin1String("SecondSection");
    case QDateTimeParser::TimeZoneSection: return QLatin1String("TimeZoneSection");
    case QDateTimeParser::YearSection: return QLatin1String("YearSection");
    case QDateTimeParser::YearSection2Digits: return QLatin1String("YearSection2Digits");
    case QDateTimeParser::NoSection: return QLatin1String("NoSection");
    case QDateTimeParser::FirstSection: return QLatin1String("FirstSection");
    case QDateTimeParser::LastSection: return QLatin1String("LastSection");
    default: return QLatin1String("Unknown section ") + QString::number(int(s));
    }
}

/*!
  \internal
  For debugging. Returns the name of the state \a s.
*/

QString QDateTimeParser::stateName(State s) const
{
    switch (s) {
    case Invalid: return QLatin1String("Invalid");
    case Intermediate: return QLatin1String("Intermediate");
    case Acceptable: return QLatin1String("Acceptable");
    default: return QLatin1String("Unknown state ") + QString::number(s);
    }
}

#if QT_CONFIG(datestring)
bool QDateTimeParser::fromString(const QString &t, QDate *date, QTime *time) const
{
    QDateTime datetime;
    if (!fromString(t, &datetime))
        return false;

    if (time) {
        const QTime t = datetime.time();
        if (!t.isValid()) {
            return false;
        }
        *time = t;
    }

    if (date) {
        const QDate d = datetime.date();
        if (!d.isValid()) {
            return false;
        }
        *date = d;
    }
    return true;
}

bool QDateTimeParser::fromString(const QString &t, QDateTime* datetime) const
{
    QDateTime val(QDate(1900, 1, 1).startOfDay());
    const StateNode tmp = parse(t, -1, val, false);
    if (tmp.state != Acceptable || tmp.conflicts)
        return false;
    if (datetime) {
        if (!tmp.value.isValid())
            return false;
        *datetime = tmp.value;
    }

    return true;
}
#endif // datestring

QDateTime QDateTimeParser::getMinimum() const
{
    // NB: QDateTimeParser always uses Qt::LocalTime time spec by default. If
    //     any subclass needs a changing time spec, it must override this
    //     method. At the time of writing, this is done by QDateTimeEditPrivate.

    // Cache the only case
    static const QDateTime localTimeMin(QDATETIMEEDIT_DATE_MIN.startOfDay(Qt::LocalTime));
    return localTimeMin;
}

QDateTime QDateTimeParser::getMaximum() const
{
    // NB: QDateTimeParser always uses Qt::LocalTime time spec by default. If
    //     any subclass needs a changing time spec, it must override this
    //     method. At the time of writing, this is done by QDateTimeEditPrivate.

    // Cache the only case
    static const QDateTime localTimeMax(QDATETIMEEDIT_DATE_MAX.endOfDay(Qt::LocalTime));
    return localTimeMax;
}

QString QDateTimeParser::getAmPmText(AmPm ap, Case cs) const
{
    const QLocale loc = locale();
    QString raw = ap == AmText ? loc.amText() : loc.pmText();
    return cs == UpperCase ? raw.toUpper() : raw.toLower();
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
