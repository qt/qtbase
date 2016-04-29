/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#include "qregexp.h"
#include "qdebug.h"

//#define QDATETIMEPARSER_DEBUG
#if defined (QDATETIMEPARSER_DEBUG) && !defined(QT_NO_DEBUG_STREAM)
#  define QDTPDEBUG qDebug() << QString("%1:%2").arg(__FILE__).arg(__LINE__)
#  define QDTPDEBUGN qDebug
#else
#  define QDTPDEBUG if (false) qDebug()
#  define QDTPDEBUGN if (false) qDebug
#endif

QT_BEGIN_NAMESPACE

#ifndef QT_BOOTSTRAPPED

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
#ifndef QT_NO_DATESTRING
        qWarning("QDateTimeParser::getDigit() Internal error (%s %d)",
                 qPrintable(t.toString()), index);
#else
        qWarning("QDateTimeParser::getDigit() Internal error (%d)", index);
#endif
        return -1;
    }
    const SectionNode &node = sectionNodes.at(index);
    switch (node.type) {
    case Hour24Section: case Hour12Section: return t.time().hour();
    case MinuteSection: return t.time().minute();
    case SecondSection: return t.time().second();
    case MSecSection: return t.time().msec();
    case YearSection2Digits:
    case YearSection: return t.date().year();
    case MonthSection: return t.date().month();
    case DaySection: return t.date().day();
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong: return t.date().day();
    case AmPmSection: return t.time().hour() > 11 ? 1 : 0;

    default: break;
    }

#ifndef QT_NO_DATESTRING
    qWarning("QDateTimeParser::getDigit() Internal error 2 (%s %d)",
             qPrintable(t.toString()), index);
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
#ifndef QT_NO_DATESTRING
        qWarning("QDateTimeParser::setDigit() Internal error (%s %d %d)",
                 qPrintable(v.toString()), index, newVal);
#else
        qWarning("QDateTimeParser::setDigit() Internal error (%d %d)", index, newVal);
#endif
        return false;
    }
    const SectionNode &node = sectionNodes.at(index);

    const QDate date = v.date();
    const QTime time = v.time();
    int year = date.year();
    int month = date.month();
    int day = date.day();
    int hour = time.hour();
    int minute = time.minute();
    int second = time.second();
    int msec = time.msec();

    switch (node.type) {
    case Hour24Section: case Hour12Section: hour = newVal; break;
    case MinuteSection: minute = newVal; break;
    case SecondSection: second = newVal; break;
    case MSecSection: msec = newVal; break;
    case YearSection2Digits:
    case YearSection: year = newVal; break;
    case MonthSection: month = newVal; break;
    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (newVal > 31) {
            // have to keep legacy behavior. setting the
            // date to 32 should return false. Setting it
            // to 31 for february should return true
            return false;
        }
        day = newVal;
        break;
    case AmPmSection: hour = (newVal == 0 ? hour % 12 : (hour % 12) + 12); break;
    default:
        qWarning("QDateTimeParser::setDigit() Internal error (%s)",
                 qPrintable(node.name()));
        break;
    }

    if (!(node.type & DaySectionMask)) {
        if (day < cachedDay)
            day = cachedDay;
        const int max = QDate(year, month, 1).daysInMonth();
        if (day > max) {
            day = max;
        }
    }
    if (QDate::isValid(year, month, day) && QTime::isValid(hour, minute, second, msec)) {
        v = QDateTime(QDate(year, month, day), QTime(hour, minute, second, msec), spec);
        return true;
    }
    return false;
}



/*!
  \

  Returns the absolute maximum for a section
*/

int QDateTimeParser::absoluteMax(int s, const QDateTime &cur) const
{
    const SectionNode &sn = sectionNode(s);
    switch (sn.type) {
    case Hour24Section:
    case Hour12Section: return 23; // this is special-cased in
                                   // parseSection. We want it to be
                                   // 23 for the stepBy case.
    case MinuteSection:
    case SecondSection: return 59;
    case MSecSection: return 999;
    case YearSection2Digits:
    case YearSection: return 9999; // sectionMaxSize will prevent
                                   // people from typing in a larger
                                   // number in count == 2 sections.
                                   // stepBy() will work on real years anyway
    case MonthSection: return 12;
    case DaySection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong: return cur.isValid() ? cur.date().daysInMonth() : 31;
    case AmPmSection: return 1;
    default: break;
    }
    qWarning("QDateTimeParser::absoluteMax() Internal error (%s)",
             qPrintable(sn.name()));
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
    qWarning("QDateTimeParser::absoluteMin() Internal error (%s, %0x)",
             qPrintable(sn.name()), sn.type);
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
        qWarning("QDateTimeParser::sectionPos Internal error (%s)", qPrintable(sn.name()));
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
/*!
  \internal

  Parses the format \a newFormat. If successful, returns \c true and
  sets up the format. Else keeps the old format and returns \c false.

*/

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
    Sections newDisplay = 0;
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
                if (parserType != QVariant::Date) {
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
                if (parserType != QVariant::Date) {
                    const SectionNode sn = { MinuteSection, i - add, countRepeat(newFormat, i, 2), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= MinuteSection;
                }
                break;
            case 's':
                if (parserType != QVariant::Date) {
                    const SectionNode sn = { SecondSection, i - add, countRepeat(newFormat, i, 2), 0 };
                    newSectionNodes.append(sn);
                    appendSeparator(&newSeparators, newFormat, index, i - index, lastQuote);
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= SecondSection;
                }
                break;

            case 'z':
                if (parserType != QVariant::Date) {
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
                if (parserType != QVariant::Date) {
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
                if (parserType != QVariant::Time) {
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
                if (parserType != QVariant::Time) {
                    const SectionNode sn = { MonthSection, i - add, countRepeat(newFormat, i, 4), 0 };
                    newSectionNodes.append(sn);
                    newSeparators.append(unquote(newFormat.midRef(index, i - index)));
                    i += sn.count - 1;
                    index = i + 1;
                    newDisplay |= MonthSection;
                }
                break;
            case 'd':
                if (parserType != QVariant::Time) {
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
#ifndef QT_NO_TEXTDATE
    int mcount = 12;
#endif

    switch (s) {
    case FirstSection:
    case NoSection:
    case LastSection: return 0;

    case AmPmSection: {
        const int lowerMax = qMin(getAmPmText(AmText, LowerCase).size(),
                                  getAmPmText(PmText, LowerCase).size());
        const int upperMax = qMin(getAmPmText(AmText, UpperCase).size(),
                                  getAmPmText(PmText, UpperCase).size());
        return qMin(4, qMin(lowerMax, upperMax));
    }

    case Hour24Section:
    case Hour12Section:
    case MinuteSection:
    case SecondSection:
    case DaySection: return 2;
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
#ifdef QT_NO_TEXTDATE
        return 2;
#else
        mcount = 7;
        // fall through
#endif
    case MonthSection:
#ifdef QT_NO_TEXTDATE
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
                                     ? l.monthName(i, format)
                                     : l.dayName(i, format));
                ret = qMax(str.size(), ret);
            }
            return ret;
        }
#endif
    case MSecSection: return 3;
    case YearSection: return 4;
    case YearSection2Digits: return 2;

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


#ifndef QT_NO_TEXTDATE
/*!
  \internal:skipToNextSection

  Parses the part of \a text that corresponds to \a s and returns
  the value of that field. Sets *stateptr to the right state if
  stateptr != 0.
*/

int QDateTimeParser::parseSection(const QDateTime &currentValue, int sectionIndex,
                                  QString &text, int &cursorPosition, int index,
                                  State &state, int *usedptr) const
{
    state = Invalid;
    int num = 0;
    const SectionNode &sn = sectionNode(sectionIndex);
    if ((sn.type & Internal) == Internal) {
        qWarning("QDateTimeParser::parseSection Internal error (%s %d)",
                 qPrintable(sn.name()), sectionIndex);
        return -1;
    }

    const int sectionmaxsize = sectionMaxSize(sectionIndex);
    QString sectiontext = text.mid(index, sectionmaxsize);
    int sectiontextSize = sectiontext.size();

    QDTPDEBUG << "sectionValue for" << sn.name()
              << "with text" << text << "and st" << sectiontext
              << text.midRef(index, sectionmaxsize)
              << index;

    int used = 0;
    switch (sn.type) {
    case AmPmSection: {
        const int ampm = findAmPm(sectiontext, sectionIndex, &used);
        switch (ampm) {
        case AM: // sectiontext == AM
        case PM: // sectiontext == PM
            num = ampm;
            state = Acceptable;
            break;
        case PossibleAM: // sectiontext => AM
        case PossiblePM: // sectiontext => PM
            num = ampm - 2;
            state = Intermediate;
            break;
        case PossibleBoth: // sectiontext => AM|PM
            num = 0;
            state = Intermediate;
            break;
        case Neither:
            state = Invalid;
            QDTPDEBUG << "invalid because findAmPm(" << sectiontext << ") returned -1";
            break;
        default:
            QDTPDEBUGN("This should never happen (findAmPm returned %d)", ampm);
            break;
        }
        if (state != Invalid)
            text.replace(index, used, sectiontext.constData(), used);
        break; }
    case MonthSection:
    case DayOfWeekSectionShort:
    case DayOfWeekSectionLong:
        if (sn.count >= 3) {
            if (sn.type == MonthSection) {
                int min = 1;
                const QDate minDate = getMinimum().date();
                if (currentValue.date().year() == minDate.year()) {
                    min = minDate.month();
                }
                num = findMonth(sectiontext.toLower(), min, sectionIndex, &sectiontext, &used);
            } else {
                num = findDay(sectiontext.toLower(), 1, sectionIndex, &sectiontext, &used);
            }

            if (num != -1) {
                state = (used == sectiontext.size() ? Acceptable : Intermediate);
                text.replace(index, used, sectiontext.constData(), used);
            } else {
                state = Intermediate;
            }
            break;
        } // else: fall through
    case DaySection:
    case YearSection:
    case YearSection2Digits:
    case Hour12Section:
    case Hour24Section:
    case MinuteSection:
    case SecondSection:
    case MSecSection: {
        if (sectiontextSize == 0) {
            num = 0;
            used = 0;
            state = Intermediate;
        } else {
            const int absMax = absoluteMax(sectionIndex);
            QLocale loc;
            bool ok = true;
            int last = -1;
            used = -1;

            QString digitsStr(sectiontext);
            for (int i = 0; i < sectiontextSize; ++i) {
                if (digitsStr.at(i).isSpace()) {
                    sectiontextSize = i;
                    break;
                }
            }

            const int max = qMin(sectionmaxsize, sectiontextSize);
            for (int digits = max; digits >= 1; --digits) {
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
                    QDTPDEBUG << sectiontext.leftRef(digits) << tmp << digits;
                    last = tmp;
                    used = digits;
                    break;
                }
            }

            if (last == -1) {
                QChar first(sectiontext.at(0));
                if (separators.at(sectionIndex + 1).startsWith(first)) {
                    used = 0;
                    state = Intermediate;
                } else {
                    state = Invalid;
                    QDTPDEBUG << "invalid because" << sectiontext << "can't become a uint" << last << ok;
                }
            } else {
                num += last;
                const FieldInfo fi = fieldInfo(sectionIndex);
                const bool done = (used == sectionmaxsize);
                if (!done && fi & Fraction) { // typing 2 in a zzz field should be .200, not .002
                    for (int i=used; i<sectionmaxsize; ++i) {
                        num *= 10;
                    }
                }
                const int absMin = absoluteMin(sectionIndex);
                if (num < absMin) {
                    state = done ? Invalid : Intermediate;
                    if (done)
                        QDTPDEBUG << "invalid because" << num << "is less than absoluteMin" << absMin;
                } else if (num > absMax) {
                    state = Intermediate;
                } else if (!done && (fi & (FixedWidth|Numeric)) == (FixedWidth|Numeric)) {
                    if (skipToNextSection(sectionIndex, currentValue, digitsStr)) {
                        state = Acceptable;
                        const int missingZeroes = sectionmaxsize - digitsStr.size();
                        text.insert(index, QString(missingZeroes, QLatin1Char('0')));
                        used = sectionmaxsize;
                        cursorPosition += missingZeroes;
                        ++(const_cast<QDateTimeParser*>(this)->sectionNodes[sectionIndex].zeroesAdded);
                    } else {
                        state = Intermediate;;
                    }
                } else {
                    state = Acceptable;
                }
            }
        }
        break; }
    default:
        qWarning("QDateTimeParser::parseSection Internal error (%s %d)",
                 qPrintable(sn.name()), sectionIndex);
        return -1;
    }

    if (usedptr)
        *usedptr = used;

    return (state != Invalid ? num : -1);
}
#endif // QT_NO_TEXTDATE

#ifndef QT_NO_DATESTRING
/*!
  \internal
*/

QDateTimeParser::StateNode QDateTimeParser::parse(QString &input, int &cursorPosition,
                                                  const QDateTime &currentValue, bool fixup) const
{
    const QDateTime minimum = getMinimum();
    const QDateTime maximum = getMaximum();

    State state = Acceptable;

    QDateTime newCurrentValue;
    int pos = 0;
    bool conflicts = false;
    const int sectionNodesCount = sectionNodes.size();

    QDTPDEBUG << "parse" << input;
    {
        int year, month, day;
        const QDate currentDate = currentValue.date();
        const QTime currentTime = currentValue.time();
        currentDate.getDate(&year, &month, &day);
        int year2digits = year % 100;
        int hour = currentTime.hour();
        int hour12 = -1;
        int minute = currentTime.minute();
        int second = currentTime.second();
        int msec = currentTime.msec();
        int dayofweek = currentDate.dayOfWeek();

        int ampm = -1;
        Sections isSet = NoSection;
        int num;
        State tmpstate;

        for (int index=0; state != Invalid && index<sectionNodesCount; ++index) {
            if (QStringRef(&input, pos, separators.at(index).size()) != separators.at(index)) {
                QDTPDEBUG << "invalid because" << input.midRef(pos, separators.at(index).size())
                          << "!=" << separators.at(index)
                          << index << pos << currentSectionIndex;
                state = Invalid;
                goto end;
            }
            pos += separators.at(index).size();
            sectionNodes[index].pos = pos;
            int *current = 0;
            const SectionNode sn = sectionNodes.at(index);
            int used;

            num = parseSection(currentValue, index, input, cursorPosition, pos, tmpstate, &used);
            QDTPDEBUG << "sectionValue" << sn.name() << input
                      << "pos" << pos << "used" << used << stateName(tmpstate);
            if (fixup && tmpstate == Intermediate && used < sn.count) {
                const FieldInfo fi = fieldInfo(index);
                if ((fi & (Numeric|FixedWidth)) == (Numeric|FixedWidth)) {
                    const QString newText = QString::fromLatin1("%1").arg(num, sn.count, 10, QLatin1Char('0'));
                    input.replace(pos, used, newText);
                    used = sn.count;
                }
            }
            pos += qMax(0, used);

            state = qMin<State>(state, tmpstate);
            if (state == Intermediate && context == FromString) {
                state = Invalid;
                break;
            }

            QDTPDEBUG << index << sn.name() << "is set to"
                      << pos << "state is" << stateName(state);


            if (state != Invalid) {
                switch (sn.type) {
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
                case DaySection: current = &day; num = qMax<int>(1, num); break;
                case AmPmSection: current = &ampm; break;
                default:
                    qWarning("QDateTimeParser::parse Internal error (%s)",
                             qPrintable(sn.name()));
                    break;
                }
                if (!current) {
                    qWarning("QDateTimeParser::parse Internal error 2");
                    return StateNode();
                }
                if (isSet & sn.type && *current != num) {
                    QDTPDEBUG << "CONFLICT " << sn.name() << *current << num;
                    conflicts = true;
                    if (index != currentSectionIndex || num == -1) {
                        continue;
                    }
                }
                if (num != -1)
                    *current = num;
                isSet |= sn.type;
            }
        }

        if (state != Invalid && QStringRef(&input, pos, input.size() - pos) != separators.last()) {
            QDTPDEBUG << "invalid because" << input.midRef(pos)
                      << "!=" << separators.last() << pos;
            state = Invalid;
        }

        if (state != Invalid) {
            if (parserType != QVariant::Time) {
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

                const QDate date(year, month, day);
                const int diff = dayofweek - date.dayOfWeek();
                if (diff != 0 && state == Acceptable && isSet & DayOfWeekSectionMask) {
                    if (isSet & DaySection)
                        conflicts = true;
                    const SectionNode &sn = sectionNode(currentSectionIndex);
                    if (sn.type & DayOfWeekSectionMask || currentSectionIndex == -1) {
                        // dayofweek should be preferred
                        day += diff;
                        if (day <= 0) {
                            day += 7;
                        } else if (day > date.daysInMonth()) {
                            day -= 7;
                        }
                        QDTPDEBUG << year << month << day << dayofweek
                                  << diff << QDate(year, month, day).dayOfWeek();
                    }
                }

                bool needfixday = false;
                if (sectionType(currentSectionIndex) & DaySectionMask) {
                    cachedDay = day;
                } else if (cachedDay > day) {
                    day = cachedDay;
                    needfixday = true;
                }

                if (!QDate::isValid(year, month, day)) {
                    if (day < 32) {
                        cachedDay = day;
                    }
                    if (day > 28 && QDate::isValid(year, month, 1)) {
                        needfixday = true;
                    }
                }
                if (needfixday) {
                    if (context == FromString) {
                        state = Invalid;
                        goto end;
                    }
                    if (state == Acceptable && fixday) {
                        day = qMin<int>(day, QDate(year, month, 1).daysInMonth());

                        const QLocale loc = locale();
                        for (int i=0; i<sectionNodesCount; ++i) {
                            const SectionNode sn = sectionNode(i);
                            if (sn.type & DaySection) {
                                input.replace(sectionPos(sn), sectionSize(i), loc.toString(day));
                            } else if (sn.type & DayOfWeekSectionMask) {
                                const int dayOfWeek = QDate(year, month, day).dayOfWeek();
                                const QLocale::FormatType dayFormat =
                                    (sn.type == DayOfWeekSectionShort
                                     ? QLocale::ShortFormat : QLocale::LongFormat);
                                const QString dayName(loc.dayName(dayOfWeek, dayFormat));
                                input.replace(sectionPos(sn), sectionSize(i), dayName);
                            }
                        }
                    } else {
                        state = qMin(Intermediate, state);
                    }
                }
            }

            if (parserType != QVariant::Date) {
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

            newCurrentValue = QDateTime(QDate(year, month, day), QTime(hour, minute, second, msec), spec);
            QDTPDEBUG << year << month << day << hour << minute << second << msec;
        }
        QDTPDEBUGN("'%s' => '%s'(%s)", input.toLatin1().constData(),
                   newCurrentValue.toString(QLatin1String("yyyy/MM/dd hh:mm:ss.zzz")).toLatin1().constData(),
                   stateName(state).toLatin1().constData());
    }
end:
    if (newCurrentValue.isValid()) {
        if (context != FromString && state != Invalid && newCurrentValue < minimum) {
            const QLatin1Char space(' ');
            if (newCurrentValue >= minimum)
                qWarning("QDateTimeParser::parse Internal error 3 (%s %s)",
                         qPrintable(newCurrentValue.toString()), qPrintable(minimum.toString()));

            bool done = false;
            state = Invalid;
            for (int i=0; i<sectionNodesCount && !done; ++i) {
                const SectionNode &sn = sectionNodes.at(i);
                QString t = sectionText(input, i, sn.pos).toLower();
                if ((t.size() < sectionMaxSize(i) && (((int)fieldInfo(i) & (FixedWidth|Numeric)) != Numeric))
                    || t.contains(space)) {
                    switch (sn.type) {
                    case AmPmSection:
                        switch (findAmPm(t, i)) {
                        case AM:
                        case PM:
                            state = Acceptable;
                            done = true;
                            break;
                        case Neither:
                            state = Invalid;
                            done = true;
                            break;
                        case PossibleAM:
                        case PossiblePM:
                        case PossibleBoth: {
                            const QDateTime copy(newCurrentValue.addSecs(12 * 60 * 60));
                            if (copy >= minimum && copy <= maximum) {
                                state = Intermediate;
                                done = true;
                            }
                            break; }
                        }
                    case MonthSection:
                        if (sn.count >= 3) {
                            const int currentMonth = newCurrentValue.date().month();
                            int tmp = currentMonth;
                            // I know the first possible month makes the date too early
                            while ((tmp = findMonth(t, tmp + 1, i)) != -1) {
                                const QDateTime copy(newCurrentValue.addMonths(tmp - currentMonth));
                                if (copy >= minimum && copy <= maximum)
                                    break; // break out of while
                            }
                            if (tmp == -1) {
                                break;
                            }
                            state = Intermediate;
                            done = true;
                            break;
                        }
                        // fallthrough
                    default: {
                        int toMin;
                        int toMax;

                        if (sn.type & TimeSectionMask) {
                            if (newCurrentValue.daysTo(minimum) != 0) {
                                break;
                            }
                            const QTime time = newCurrentValue.time();
                            toMin = time.msecsTo(minimum.time());
                            if (newCurrentValue.daysTo(maximum) > 0) {
                                toMax = -1; // can't get to max
                            } else {
                                toMax = time.msecsTo(maximum.time());
                            }
                        } else {
                            toMin = newCurrentValue.daysTo(minimum);
                            toMax = newCurrentValue.daysTo(maximum);
                        }
                        const int maxChange = sn.maxChange();
                        if (toMin > maxChange) {
                            QDTPDEBUG << "invalid because toMin > maxChange" << toMin
                                      << maxChange << t << newCurrentValue << minimum;
                            state = Invalid;
                            done = true;
                            break;
                        } else if (toMax > maxChange) {
                            toMax = -1; // can't get to max
                        }

                        const int min = getDigit(minimum, i);
                        if (min == -1) {
                            qWarning("QDateTimeParser::parse Internal error 4 (%s)",
                                     qPrintable(sn.name()));
                            state = Invalid;
                            done = true;
                            break;
                        }

                        int max = toMax != -1 ? getDigit(maximum, i) : absoluteMax(i, newCurrentValue);
                        int pos = cursorPosition - sn.pos;
                        if (pos < 0 || pos >= t.size())
                            pos = -1;
                        if (!potentialValue(t.simplified(), min, max, i, newCurrentValue, pos)) {
                            QDTPDEBUG << "invalid because potentialValue(" << t.simplified() << min << max
                                      << sn.name() << "returned" << toMax << toMin << pos;
                            state = Invalid;
                            done = true;
                            break;
                        }
                        state = Intermediate;
                        done = true;
                        break; }
                    }
                }
            }
        } else {
            if (context == FromString) {
                // optimization
                Q_ASSERT(getMaximum().date().toJulianDay() == 4642999);
                if (newCurrentValue.date().toJulianDay() > 4642999)
                    state = Invalid;
            } else {
                if (newCurrentValue > getMaximum())
                    state = Invalid;
            }

            QDTPDEBUG << "not checking intermediate because newCurrentValue is" << newCurrentValue << getMinimum() << getMaximum();
        }
    }
    StateNode node;
    node.input = input;
    node.state = state;
    node.conflicts = conflicts;
    node.value = newCurrentValue.toTimeSpec(spec);
    text = input;
    return node;
}
#endif // QT_NO_DATESTRING

#ifndef QT_NO_TEXTDATE
/*!
  \internal
  finds the first possible monthname that \a str1 can
  match. Starting from \a index; str should already by lowered
*/

int QDateTimeParser::findMonth(const QString &str1, int startMonth, int sectionIndex,
                               QString *usedMonth, int *used) const
{
    int bestMatch = -1;
    int bestCount = 0;
    if (!str1.isEmpty()) {
        const SectionNode &sn = sectionNode(sectionIndex);
        if (sn.type != MonthSection) {
            qWarning("QDateTimeParser::findMonth Internal error");
            return -1;
        }

        QLocale::FormatType type = sn.count == 3 ? QLocale::ShortFormat : QLocale::LongFormat;
        QLocale l = locale();

        for (int month=startMonth; month<=12; ++month) {
            const QString monthName = l.monthName(month, type);
            QString str2 = monthName.toLower();

            if (str1.startsWith(str2)) {
                if (used) {
                    QDTPDEBUG << "used is set to" << str2.size();
                    *used = str2.size();
                }
                if (usedMonth)
                    *usedMonth = monthName;

                return month;
            }
            if (context == FromString)
                continue;

            const int limit = qMin(str1.size(), str2.size());

            QDTPDEBUG << "limit is" << limit << str1 << str2;
            bool equal = true;
            for (int i=0; i<limit; ++i) {
                if (str1.at(i) != str2.at(i)) {
                    equal = false;
                    if (i > bestCount) {
                        bestCount = i;
                        bestMatch = month;
                    }
                    break;
                }
            }
            if (equal) {
                if (used)
                    *used = limit;
                if (usedMonth)
                    *usedMonth = monthName;
                return month;
            }
        }
        if (usedMonth && bestMatch != -1)
            *usedMonth = l.monthName(bestMatch, type);
    }
    if (used) {
        QDTPDEBUG << "used is set to" << bestCount;
        *used = bestCount;
    }
    return bestMatch;
}

int QDateTimeParser::findDay(const QString &str1, int startDay, int sectionIndex, QString *usedDay, int *used) const
{
    int bestMatch = -1;
    int bestCount = 0;
    if (!str1.isEmpty()) {
        const SectionNode &sn = sectionNode(sectionIndex);
        if (!(sn.type & DaySectionMask)) {
            qWarning("QDateTimeParser::findDay Internal error");
            return -1;
        }
        const QLocale l = locale();
        for (int day=startDay; day<=7; ++day) {
            const QString str2 = l.dayName(day, sn.count == 4 ? QLocale::LongFormat : QLocale::ShortFormat);

            if (str1.startsWith(str2.toLower())) {
                if (used)
                    *used = str2.size();
                if (usedDay) {
                    *usedDay = str2;
                }
                return day;
            }
            if (context == FromString)
                continue;

            const int limit = qMin(str1.size(), str2.size());
            bool found = true;
            for (int i=0; i<limit; ++i) {
                if (str1.at(i) != str2.at(i) && !str1.at(i).isSpace()) {
                    if (i > bestCount) {
                        bestCount = i;
                        bestMatch = day;
                    }
                    found = false;
                    break;
                }

            }
            if (found) {
                if (used)
                    *used = limit;
                if (usedDay)
                    *usedDay = str2;

                return day;
            }
        }
        if (usedDay && bestMatch != -1) {
            *usedDay = l.dayName(bestMatch, sn.count == 4 ? QLocale::LongFormat : QLocale::ShortFormat);
        }
    }
    if (used)
        *used = bestCount;

    return bestMatch;
}
#endif // QT_NO_TEXTDATE

/*!
  \internal

  Returns
  AM if str == tr("AM")
  PM if str == tr("PM")
  PossibleAM if str can become tr("AM")
  PossiblePM if str can become tr("PM")
  PossibleBoth if str can become tr("PM") and can become tr("AM")
  Neither if str can't become anything sensible
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

    if (str.indexOf(ampm[amindex], 0, Qt::CaseInsensitive) == 0) {
        str = ampm[amindex];
        return AM;
    } else if (str.indexOf(ampm[pmindex], 0, Qt::CaseInsensitive) == 0) {
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
        qWarning("QDateTimeParser::maxChange() Internal error (%s)",
                 qPrintable(name()));
    }

    return -1;
}

QDateTimeParser::FieldInfo QDateTimeParser::fieldInfo(int index) const
{
    FieldInfo ret = 0;
    const SectionNode &sn = sectionNode(index);
    switch (sn.type) {
    case MSecSection:
        ret |= Fraction;
        // fallthrough
    case SecondSection:
    case MinuteSection:
    case Hour24Section:
    case Hour12Section:
    case YearSection:
    case YearSection2Digits:
        ret |= Numeric;
        if (sn.type != YearSection) {
            ret |= AllowPartial;
        }
        if (sn.count != 1) {
            ret |= FixedWidth;
        }
        break;
    case MonthSection:
    case DaySection:
        switch (sn.count) {
        case 2:
            ret |= FixedWidth;
            // fallthrough
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
    default:
        qWarning("QDateTimeParser::fieldInfo Internal error 2 (%d %s %d)",
                 index, qPrintable(sn.name()), sn.count);
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
        qWarning("QDateTimeParser::sectionFormat Internal error (%s)",
                 qPrintable(name(type)));
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

bool QDateTimeParser::potentialValue(const QString &str, int min, int max, int index,
                                     const QDateTime &currentValue, int insert) const
{
    if (str.isEmpty()) {
        return true;
    }
    const int size = sectionMaxSize(index);
    int val = (int)locale().toUInt(str);
    const SectionNode &sn = sectionNode(index);
    if (sn.type == YearSection2Digits) {
        const int year = currentValue.date().year();
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
                QString tmp = str;
                tmp.insert(insert, QLatin1Char('0' + j));
                if (potentialValue(tmp, min, max, index, currentValue, insert))
                    return true;
            }
        }
    }

    return false;
}

bool QDateTimeParser::skipToNextSection(int index, const QDateTime &current, const QString &text) const
{
    Q_ASSERT(current >= getMinimum() && current <= getMaximum());

    const SectionNode &node = sectionNode(index);
    Q_ASSERT(text.size() < sectionMaxSize(index));

    const QDateTime maximum = getMaximum();
    const QDateTime minimum = getMinimum();
    QDateTime tmp = current;
    int min = absoluteMin(index);
    setDigit(tmp, index, min);
    if (tmp < minimum) {
        min = getDigit(minimum, index);
    }

    int max = absoluteMax(index, current);
    setDigit(tmp, index, max);
    if (tmp > maximum) {
        max = getDigit(maximum, index);
    }
    int pos = cursorPosition() - node.pos;
    if (pos < 0 || pos >= text.size())
        pos = -1;

    const bool potential = potentialValue(text, min, max, index, current, pos);
    return !potential;

    /* If the value potentially can become another valid entry we
     * don't want to skip to the next. E.g. In a M field (month
     * without leading 0 if you type 1 we don't want to autoskip but
     * if you type 3 we do
    */
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

#ifndef QT_NO_DATESTRING
bool QDateTimeParser::fromString(const QString &t, QDate *date, QTime *time) const
{
    QDateTime val(QDate(1900, 1, 1), QDATETIMEEDIT_TIME_MIN);
    QString text = t;
    int copy = -1;
    const StateNode tmp = parse(text, copy, val, false);
    if (tmp.state != Acceptable || tmp.conflicts) {
        return false;
    }
    if (time) {
        const QTime t = tmp.value.time();
        if (!t.isValid()) {
            return false;
        }
        *time = t;
    }

    if (date) {
        const QDate d = tmp.value.date();
        if (!d.isValid()) {
            return false;
        }
        *date = d;
    }
    return true;
}
#endif // QT_NO_DATESTRING

QDateTime QDateTimeParser::getMinimum() const
{
    return QDateTime(QDATETIMEEDIT_DATE_MIN, QDATETIMEEDIT_TIME_MIN, spec);
}

QDateTime QDateTimeParser::getMaximum() const
{
    return QDateTime(QDATETIMEEDIT_DATE_MAX, QDATETIMEEDIT_TIME_MAX, spec);
}

QString QDateTimeParser::getAmPmText(AmPm ap, Case cs) const
{
    if (ap == AmText) {
        return (cs == UpperCase ? tr("AM") : tr("am"));
    } else {
        return (cs == UpperCase ? tr("PM") : tr("pm"));
    }
}

/*
  \internal

  I give arg2 preference because arg1 is always a QDateTime.
*/

bool operator==(const QDateTimeParser::SectionNode &s1, const QDateTimeParser::SectionNode &s2)
{
    return (s1.type == s2.type) && (s1.pos == s2.pos) && (s1.count == s2.count);
}

#endif // QT_BOOTSTRAPPED

QT_END_NAMESPACE
