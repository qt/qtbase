// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qhitrace_p.h>
#include <private/qtools_p.h>

#include <QtCore/qlogging.h>

#include <hitrace/trace.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <type_traits>

QT_BEGIN_NAMESPACE

namespace {

constexpr char elidedMark[] = "...";
constexpr char16_t firstPrintable = 0x20;
constexpr char16_t lastPrintable = 0x7e;
constexpr char escapeMark = '\\';
constexpr char unicodeEscape = 'u';
constexpr char separatorReplacement = ' ';
constexpr int escapeDigits = 2 * sizeof(char16_t);
constexpr int bitsPerHexDigit = 4;
constexpr qsizetype shortEscapeSize = 2;
constexpr qsizetype unicodeEscapeSize = 2 + escapeDigits;
constexpr int32_t unregisteredListener = -1;

Q_CONSTINIT int32_t qtHiTraceListener = unregisteredListener;

bool isSeparator(QChar unit)
{
    return unit == u',' || unit == u'=' || unit == u'|';
}

bool isWrittenVerbatim(QChar unit)
{
    return unit.unicode() >= firstPrintable && unit.unicode() <= lastPrintable
            && unit != QLatin1Char(escapeMark);
}

char shortEscape(QChar unit)
{
    switch (unit.unicode()) {
    case '\\':
        return '\\';
    case '\b':
        return 'b';
    case '\f':
        return 'f';
    case '\n':
        return 'n';
    case '\r':
        return 'r';
    case '\t':
        return 't';
    default:
        return 0;
    }
}

qsizetype escapedSize(QChar unit)
{
    if (isWrittenVerbatim(unit))
        return 1;
    return shortEscape(unit) ? shortEscapeSize : unicodeEscapeSize;
}

QByteArray escaped(QStringView text)
{
    QByteArray result;
    result.reserve(text.size());
    for (const QChar unit : text) {
        if (isSeparator(unit)) {
            result += separatorReplacement;
            continue;
        }
        if (isWrittenVerbatim(unit)) {
            result += char(unit.unicode());
            continue;
        }
        result += escapeMark;
        if (const char shortForm = shortEscape(unit)) {
            result += shortForm;
            continue;
        }
        result += unicodeEscape;
        for (int digit = escapeDigits - 1; digit >= 0; --digit)
            result += QtMiscUtils::toHexLower(unit.unicode() >> (digit * bitsPerHexDigit));
    }
    return result;
}

struct FittingUnits
{
    qsizetype count = 0;
    qsizetype escapedSize = 0;
};

template <typename Iterator>
bool startsSurrogatePair(Iterator unit, Iterator end)
{
    constexpr bool isReversed = std::is_same_v<Iterator, QStringView::const_reverse_iterator>;
    if (unit + 1 == end)
        return false;
    const QChar high = isReversed ? unit[1] : unit[0];
    const QChar low = isReversed ? unit[0] : unit[1];
    return high.isHighSurrogate() && low.isLowSurrogate();
}

template <typename Iterator>
FittingUnits fittingUnits(Iterator unit, Iterator end, qsizetype maximumSize)
{
    FittingUnits fitting;
    while (unit != end) {
        const qsizetype step = startsSurrogatePair(unit, end) ? 2 : 1;
        qsizetype stepSize = escapedSize(unit[0]);
        if (step == 2)
            stepSize += escapedSize(unit[1]);
        if (fitting.escapedSize + stepSize > maximumSize)
            break;
        fitting.escapedSize += stepSize;
        fitting.count += step;
        unit += step;
    }
    return fitting;
}

QByteArray elided(QStringView value, qsizetype maximumSize)
{
    Q_ASSERT(maximumSize >= qtHiTraceSmallestValueBudget);

    if (fittingUnits(value.cbegin(), value.cend(), maximumSize).count == value.size())
        return escaped(value);

    const qsizetype kept = maximumSize - (qsizetype(sizeof(elidedMark)) - 1);
    const FittingUnits head = fittingUnits(value.cbegin(), value.cend(), kept / 2);
    const FittingUnits tail =
            fittingUnits(value.crbegin(), value.crend(), kept - head.escapedSize);
    return escaped(value.first(head.count)) + elidedMark + escaped(value.last(tail.count));
}

void qtHiTraceStatusChanged(bool enabled)
{
    qtHiTraceEnabled.storeRelaxed(enabled);
}

void qtHiTraceWatchStatus()
{
    const int32_t listener = OH_HiTrace_RegisterTraceListener(qtHiTraceStatusChanged);
    if (listener < 0) {
        qtHiTraceEnabled.storeRelaxed(true);
        qWarning("Qt tracing: OH_HiTrace_RegisterTraceListener() failed with %d, so tracepoint "
                 "arguments are rendered whether or not a capture is running",
                 listener);
        return;
    }

    qtHiTraceListener = listener;
    if (OH_HiTrace_IsTraceEnabled())
        qtHiTraceEnabled.storeRelaxed(true);
}

void qtHiTraceUnwatchStatus()
{
    if (qtHiTraceListener != unregisteredListener)
        OH_HiTrace_UnregisterTraceListener(qtHiTraceListener);
}

} // namespace

QByteArray qtHiTraceArgument(const char *value, qsizetype maximumSize)
{
    return elided(QString::fromUtf8(value), maximumSize);
}

QByteArray qtHiTraceArgument(const QString &value, qsizetype maximumSize)
{
    return elided(value, maximumSize);
}

const char *qtHiTraceFormatArguments(int sliceNameSize, const char *format, ...)
{
    Q_CONSTINIT static thread_local char renderedArguments[qtHiTraceRecordBudget] = {};

    const int budget = qtHiTraceRecordBudget - sliceNameSize;
    if (budget <= int(sizeof(elidedMark)))
        return "";

    va_list values;
    va_start(values, format);
    const int renderedSize = std::vsnprintf(renderedArguments, budget, format, values);
    va_end(values);

    if (renderedSize < 0)
        return "";
    if (renderedSize >= budget) {
        std::memcpy(renderedArguments + budget - sizeof(elidedMark), elidedMark,
                    sizeof(elidedMark));
    }
    return renderedArguments;
}

Q_CONSTINIT QBasicAtomicInteger<bool> qtHiTraceEnabled = Q_BASIC_ATOMIC_INITIALIZER(false);

Q_CONSTRUCTOR_FUNCTION(qtHiTraceWatchStatus)
Q_DESTRUCTOR_FUNCTION(qtHiTraceUnwatchStatus)

QT_END_NAMESPACE
