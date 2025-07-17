// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parsing

#include "qdebug.h"
#include "private/qdebug_p.h"
#include "qmetaobject.h"
#include <private/qlogging_p.h>
#include <private/qtextstream_p.h>
#include <private/qtools_p.h>

#include <array>
#include <q20chrono.h>
#include <cstdio>

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

/*
    Returns a human readable representation of the first \a maxSize
    characters in \a data. The size, \a len, is a 64-bit quantity to
    avoid truncation due to implicit conversions in callers.
*/
QByteArray QtDebugUtils::toPrintable(const char *data, qint64 len, qsizetype maxSize)
{
    if (!data)
        return "(null)";

    QByteArray out;
    for (qsizetype i = 0; i < qMin(len, maxSize); ++i) {
        char c = data[i];
        if (isAsciiPrintable(c)) {
            out += c;
        } else {
            switch (c) {
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default: {
                const char buf[] = {
                    '\\',
                    'x',
                    toHexLower(uchar(c) / 16),
                    toHexLower(uchar(c) % 16),
                    0
                };
                out += buf;
            }
            }
        }
    }

    if (maxSize < len)
        out += "...";

    return out;
}

// This file is needed to force compilation of QDebug into the kernel library.

/*!
    \class QDebug
    \inmodule QtCore
    \ingroup shared

    \brief The QDebug class provides an output stream for debugging information.

    QDebug is used whenever the developer needs to write out debugging or tracing
    information to a device, file, string or console.

    \section1 Basic Use

    In the common case, it is useful to call the qDebug() function to obtain a
    default QDebug object to use for writing debugging information.

    \snippet qdebug/qdebugsnippet.cpp 1

    This constructs a QDebug object using the constructor that accepts a QtMsgType
    value of QtDebugMsg. Similarly, the qInfo(), qWarning(), qCritical() and qFatal()
    functions also return QDebug objects for the corresponding message types.

    The class also provides several constructors for other situations, including
    a constructor that accepts a QFile or any other QIODevice subclass that is
    used to write debugging information to files and other devices. The constructor
    that accepts a QString is used to write to a string for display or serialization.

    \section1 Formatting Options

    QDebug formats output so that it's easily readable. It automatically adds spaces
    between arguments, and adds quotes around QString, QByteArray, QChar arguments.

    You can tweak these options through the space(), nospace() and quote(), noquote()
    methods. Furthermore, \l{QTextStream manipulators} can be piped into a QDebug
    stream.

    QDebugStateSaver limits changes to the formatting to the current scope.
    resetFormat() resets the options to the default ones.

    \section1 Writing Custom Types to a Stream

    Many standard types can be written to QDebug objects, and Qt provides support for
    most Qt value types. To add support for custom types, you need to implement a
    streaming operator, as in the following example:

    \snippet qdebug/qdebugsnippet.cpp 0

    This is described in the \l{Debugging Techniques} and
    \l{Creating Custom Qt Types#Making the Type Printable}{Creating Custom Qt Types}
    documents.
*/

/*!
    \fn QDebug::QDebug(QIODevice *device)

    Constructs a debug stream that writes to the given \a device.
*/

/*!
    \fn QDebug::QDebug(QString *string)

    Constructs a debug stream that writes to the given \a string.
*/

/*!
    \fn QDebug::QDebug(QtMsgType t)

    Constructs a debug stream that writes to the handler for the message type \a t.
*/

/*!
    \fn QDebug::QDebug(const QDebug &o)

    Constructs a copy of the other debug stream \a o.
*/

/*!
    \fn QDebug &QDebug::operator=(const QDebug &other)

    Assigns the \a other debug stream to this stream and returns a reference to
    this stream.
*/

/*!
    \fn QDebug::~QDebug()

    Flushes any pending data to be written and destroys the debug stream.
*/
QDebug::~QDebug()
{
    if (stream && !--stream->ref) {
        if (stream->space && stream->buffer.endsWith(u' '))
            stream->buffer.chop(1);
        if (stream->message_output) {
            QInternalMessageLogContext ctxt(stream->context);
            qt_message_output(stream->type,
                              ctxt,
                              stream->buffer);
        }
        delete stream;
    }
}

/*!
    \internal
*/
void QDebug::putUcs4(uint ucs4)
{
    maybeQuote('\'');
    if (ucs4 < 0x20) {
        stream->ts << "\\x" << Qt::hex << ucs4 << Qt::reset;
    } else if (ucs4 < 0x80) {
        stream->ts << char(ucs4);
    } else {
        if (ucs4 < 0x10000)
            stream->ts << "\\u" << qSetFieldWidth(4);
        else
            stream->ts << "\\U" << qSetFieldWidth(8);
        stream->ts << Qt::hex << qSetPadChar(u'0') << ucs4 << Qt::reset;
    }
    maybeQuote('\'');
}

// These two functions return true if the character should be printed by QDebug.
// For QByteArray, this is technically identical to US-ASCII isprint();
// for QString, we use QChar::isPrint, which requires a full UCS-4 decode.
static inline bool isPrintable(char32_t ucs4) { return QChar::isPrint(ucs4); }
static inline bool isPrintable(char16_t uc) { return QChar::isPrint(uc); }
static inline bool isPrintable(uchar c)
{ return isAsciiPrintable(c); }

template <typename Char>
static inline void putEscapedString(QTextStreamPrivate *d, const Char *begin, size_t length, bool isUnicode = true)
{
    constexpr char16_t quotes[] = uR"("")";
    constexpr char16_t quote = quotes[0];
    d->write(quote);

    bool lastWasHexEscape = false;
    const Char *end = begin + length;
    for (const Char *p = begin; p != end; ++p) {
        // check if we need to insert "" to break an hex escape sequence
        if (Q_UNLIKELY(lastWasHexEscape)) {
            if (fromHex(*p) != -1) {
                // yes, insert it
                d->write(quotes);
            }
            lastWasHexEscape = false;
        }

        if constexpr (sizeof(Char) == sizeof(QChar)) {
            // Surrogate characters are category Cs (Other_Surrogate), so isPrintable = false for them
            qsizetype runLength = 0;
            while (p + runLength != end &&
                   isPrintable(p[runLength]) && p[runLength] != '\\' && p[runLength] != '"')
                ++runLength;
            if (runLength) {
                d->write(QStringView{p, runLength});
                p += runLength - 1;
                continue;
            }
        } else if (isPrintable(*p) && *p != '\\' && *p != '"') {
            d->write(char16_t{uchar(*p)});
            continue;
        }

        // print as an escape sequence (maybe, see below for surrogate pairs)
        qsizetype buflen = 2;
        char16_t buf[std::char_traits<char>::length("\\U12345678")];
        buf[0] = '\\';

        switch (*p) {
        case '"':
        case '\\':
            buf[1] = *p;
            break;
        case '\b':
            buf[1] = 'b';
            break;
        case '\f':
            buf[1] = 'f';
            break;
        case '\n':
            buf[1] = 'n';
            break;
        case '\r':
            buf[1] = 'r';
            break;
        case '\t':
            buf[1] = 't';
            break;
        default:
            if (!isUnicode) {
                // print as hex escape
                buf[1] = 'x';
                buf[2] = toHexUpper(uchar(*p) >> 4);
                buf[3] = toHexUpper(uchar(*p));
                buflen = 4;
                lastWasHexEscape = true;
                break;
            }
            if (QChar::isHighSurrogate(*p)) {
                if ((p + 1) != end && QChar::isLowSurrogate(p[1])) {
                    // properly-paired surrogates
                    char32_t ucs4 = QChar::surrogateToUcs4(*p, p[1]);
                    if (isPrintable(ucs4)) {
                        buf[0] = *p;
                        buf[1] = p[1];
                        buflen = 2;
                    } else {
                        buf[1] = 'U';
                        buf[2] = '0'; // toHexUpper(ucs4 >> 28);
                        buf[3] = '0'; // toHexUpper(ucs4 >> 24);
                        buf[4] = toHexUpper(ucs4 >> 20);
                        buf[5] = toHexUpper(ucs4 >> 16);
                        buf[6] = toHexUpper(ucs4 >> 12);
                        buf[7] = toHexUpper(ucs4 >> 8);
                        buf[8] = toHexUpper(ucs4 >> 4);
                        buf[9] = toHexUpper(ucs4);
                        buflen = 10;
                    }
                    ++p;
                    break;
                }
                // improperly-paired surrogates, fall through
            }
            buf[1] = 'u';
            buf[2] = toHexUpper(char16_t(*p) >> 12);
            buf[3] = toHexUpper(char16_t(*p) >> 8);
            buf[4] = toHexUpper(*p >> 4);
            buf[5] = toHexUpper(*p);
            buflen = 6;
        }
        d->write(QStringView{buf, buflen});
    }

    d->write(quote);
}

/*!
    \internal
    Duplicated from QtTest::toPrettyUnicode().
*/
void QDebug::putString(const QChar *begin, size_t length)
{
    if (stream->noQuotes) {
        // no quotes, write the string directly too (no pretty-printing)
        // this respects the QTextStream state, though
        stream->ts.d_ptr->putString(QStringView{begin, qsizetype(length)});
    } else {
        // we'll reset the QTextStream formatting mechanisms, so save the state
        QDebugStateSaver saver(*this);
        stream->ts.d_ptr->params.reset();
        putEscapedString(stream->ts.d_ptr.get(), reinterpret_cast<const char16_t *>(begin), length);
    }
}

/*!
    \internal
    Duplicated from QtTest::toPrettyCString().
*/
void QDebug::putByteArray(const char *begin, size_t length, Latin1Content content)
{
    if (stream->noQuotes) {
        // no quotes, write the string directly too (no pretty-printing)
        // this respects the QTextStream state, though
        switch (content) {
        case Latin1Content::ContainsLatin1:
            stream->ts.d_ptr->putString(QLatin1StringView{begin, qsizetype(length)});
            break;
        case Latin1Content::ContainsBinary:
            stream->ts.d_ptr->putString(QUtf8StringView{begin, qsizetype(length)});
            break;
        }
    } else {
        // we'll reset the QTextStream formatting mechanisms, so save the state
        QDebugStateSaver saver(*this);
        stream->ts.d_ptr->params.reset();
        putEscapedString(stream->ts.d_ptr.get(), reinterpret_cast<const uchar *>(begin),
                         length, content == ContainsLatin1);
    }
}

static QByteArray timeUnit(qint64 num, qint64 den)
{
    using namespace std::chrono;
    using namespace q20::chrono;

    if (num == 1 && den > 1) {
        // sub-multiple of seconds
        char prefix = '\0';
        auto tryprefix = [&](auto d, char c) {
            static_assert(decltype(d)::num == 1, "not an SI prefix");
            if (den == decltype(d)::den)
                prefix = c;
        };

        // "u" should be "µ", but debugging output is not always UTF-8-safe
        tryprefix(std::milli{}, 'm');
        tryprefix(std::micro{}, 'u');
        tryprefix(std::nano{}, 'n');
        tryprefix(std::pico{}, 'p');
        tryprefix(std::femto{}, 'f');
        tryprefix(std::atto{}, 'a');
        // uncommon ones later
        tryprefix(std::centi{}, 'c');
        tryprefix(std::deci{}, 'd');
        if (prefix) {
            char unit[3] = { prefix, 's' };
            return QByteArray(unit, sizeof(unit) - 1);
        }
    }

    const char *unit = nullptr;
    if (num > 1 && den == 1) {
        // multiple of seconds - but we don't use SI prefixes
        auto tryunit = [&](auto d, const char *name) {
            static_assert(decltype(d)::period::den == 1, "not a multiple of a second");
            if (unit || num % decltype(d)::period::num)
                return;
            unit = name;
            num /= decltype(d)::period::num;
        };
        tryunit(years{}, "yr");
        tryunit(weeks{}, "wk");
        tryunit(days{}, "d");
        tryunit(hours{}, "h");
        tryunit(minutes{}, "min");
    }
    if (!unit)
        unit = "s";

    if (num == 1 && den == 1)
        return unit;
    if (Q_UNLIKELY(num < 1 || den < 1))
        return QString::asprintf("<invalid time unit %lld/%lld>", num, den).toLatin1();

    // uncommon units: will return something like "[2/3]s"
    //  strlen("[/]min") = 6
    char buf[2 * (std::numeric_limits<qint64>::digits10 + 2) + 10];
    size_t len = 0;
    auto appendChar = [&](char c) {
        Q_ASSERT(len < sizeof(buf));
        buf[len++] = c;
    };
    auto appendNumber = [&](qint64 value) {
        if (value >= 10'000 && (value % 1000) == 0)
            len += std::snprintf(buf + len, sizeof(buf) - len, "%.6g", double(value));  // "1e+06"
        else
            len += std::snprintf(buf + len, sizeof(buf) - len, "%lld", value);
    };
    appendChar('[');
    appendNumber(num);
    if (den != 1) {
        appendChar('/');
        appendNumber(den);
    }
    appendChar(']');
    memcpy(buf + len, unit, strlen(unit));
    return QByteArray(buf, len + strlen(unit));
}

/*!
    \since 6.6
    \internal
    Helper to the std::chrono::duration debug streaming output.
 */
void QDebug::putTimeUnit(qint64 num, qint64 den)
{
    stream->ts << timeUnit(num, den); // ### optimize
}

namespace {

#ifdef QT_SUPPORTS_INT128

constexpr char Q_INT128_MIN_STR[] = "-170141183460469231731687303715884105728";

constexpr int Int128BufferSize = sizeof(Q_INT128_MIN_STR);
using Int128Buffer = std::array<char, Int128BufferSize>;
                                           // numeric_limits<qint128>::digits10 may not exist

static char *i128ToStringHelper(Int128Buffer &buffer, quint128 n)
{
    auto dst = buffer.data() + buffer.size();
    *--dst = '\0'; // NUL-terminate
    if (n == 0) {
        *--dst = '0'; // and done
    } else {
        while (n != 0) {
            *--dst = "0123456789"[n % 10];
            n /= 10;
        }
    }
    return dst;
}
#endif // QT_SUPPORTS_INT128

[[maybe_unused]]
static const char *int128Warning()
{
    const char *msg = "Qt was not compiled with int128 support.";
    qWarning("%s", msg);
    return msg;
}

} // unnamed namespace

/*!
    \since 6.7
    \internal
    Helper to the qint128 debug streaming output.
 */
void QDebug::putInt128([[maybe_unused]] const void *p)
{
#ifdef QT_SUPPORTS_INT128
    Q_ASSERT(p);
    qint128 i;
    memcpy(&i, p, sizeof(i)); // alignment paranoia
    if (i == Q_INT128_MIN) {
        // -i is not representable, hardcode the result:
        stream->ts << Q_INT128_MIN_STR;
    } else {
        Int128Buffer buffer;
        auto dst = i128ToStringHelper(buffer, i < 0 ? -i : i);
        if (i < 0)
            *--dst = '-';
        stream->ts << dst;
    }
    return;
#endif // QT_SUPPORTS_INT128
    stream->ts << int128Warning();
}

/*!
    \since 6.7
    \internal
    Helper to the quint128 debug streaming output.
 */
void QDebug::putUInt128([[maybe_unused]] const void *p)
{
#ifdef QT_SUPPORTS_INT128
    Q_ASSERT(p);
    quint128 i;
    memcpy(&i, p, sizeof(i)); // alignment paranoia
    Int128Buffer buffer;
    stream->ts << i128ToStringHelper(buffer, i);
    return;
#endif // QT_SUPPORTS_INT128
    stream->ts << int128Warning();
}

/*!
    \since 6.9
    \internal
    Helper to the <Std/Qt>::<>_ordering debug output.
    It generates the string in following format:
    <Qt/Std>::<weak/partial/strong>_ordering::<less/equal/greater/unordered>
 */
void QDebug::putQtOrdering(QtOrderingPrivate::QtOrderingTypeFlag flags, Qt::partial_ordering order)
{
    using QtOrderingPrivate::QtOrderingType;
    std::string result;
    if ((flags & QtOrderingType::StdOrder) == QtOrderingType::StdOrder)
        result += "std";
    else if ((flags & QtOrderingType::QtOrder) == QtOrderingType::QtOrder)
        result += "Qt";

    result += "::";
    const bool isStrong = ((flags & QtOrderingType::Strong) == QtOrderingType::Strong);
    if (isStrong)
        result += "strong";
    else if ((flags & QtOrderingType::Weak) == QtOrderingType::Weak)
        result += "weak";
    else if ((flags & QtOrderingType::Partial) == QtOrderingType::Partial)
        result += "partial";
    result += "_ordering::";

    if (order == Qt::partial_ordering::equivalent) {
        if (isStrong)
            result += "equal";
        else
            result += "equivalent";
    } else if (order == Qt::partial_ordering::greater) {
        result += "greater";
    } else if (order == Qt::partial_ordering::less) {
        result += "less";
    } else {
        result += "unordered";
    }
    stream->ts << result.data();
}

/*!
    \fn QDebug::swap(QDebug &other)
    \since 5.0
    \memberswap{debug stream instance}
*/

/*!
    Resets the stream formatting options, bringing it back to its original constructed state.

    \sa space(), quote()
    \since 5.4
*/
QDebug &QDebug::resetFormat()
{
    stream->ts.reset();
    stream->space = true;
    stream->noQuotes = false;
    stream->verbosity = DefaultVerbosity;
    return *this;
}

/*!
    \fn QDebug &QDebug::space()

    Writes a space character to the debug stream and returns a reference to
    the stream.

    The stream remembers that automatic insertion of spaces is
    enabled for future writes.

    \sa nospace(), maybeSpace()
*/

/*!
    \fn QDebug &QDebug::nospace()

    Disables automatic insertion of spaces and returns a reference to the stream.

    \sa space(), maybeSpace()
*/

/*!
    \fn QDebug &QDebug::maybeSpace()

    Writes a space character to the debug stream, depending on the current
    setting for automatic insertion of spaces, and returns a reference to the stream.

    \sa space(), nospace()
*/

/*!
    \fn bool QDebug::autoInsertSpaces() const

    Returns \c true if this QDebug instance will automatically insert spaces
    between writes.

    \since 5.0

    \sa QDebugStateSaver
*/

/*!
    \fn void QDebug::setAutoInsertSpaces(bool b)

    Enables automatic insertion of spaces between writes if \a b is true; otherwise
    automatic insertion of spaces is disabled.

    \since 5.0

    \sa QDebugStateSaver
*/


/*!
    \fn bool QDebug::quoteStrings() const
    \since 6.7

    Returns \c true if this QDebug instance will quote strings streamed into
    it (which is the default).

    \sa QDebugStateSaver, quote(), noquote(), setQuoteStrings()
*/

/*!
    \fn void QDebug::setQuoteStrings(bool b)
    \since 6.7

    Enables quoting of strings streamed into this QDebug instance if \a b is
    \c true; otherwise quoting is disabled.

    The default is to quote strings.

    \sa QDebugStateSaver, quote(), noquote(), quoteStrings()
*/


/*!
    \fn QDebug &QDebug::quote()
    \since 5.4

    Enables automatic insertion of quotation characters around QChar, QString and QByteArray
    contents and returns a reference to the stream.

    Quoting is enabled by default.

    \sa noquote(), maybeQuote()
*/

/*!
    \fn QDebug &QDebug::noquote()
    \since 5.4

    Disables automatic insertion of quotation characters around QChar, QString and QByteArray
    contents and returns a reference to the stream.

    When quoting is disabled, these types are printed without quotation
    characters and without escaping of non-printable characters.

    \sa quote(), maybeQuote()
*/

/*!
    \fn QDebug &QDebug::maybeQuote(char c)
    \since 5.4

    Writes a character \a c to the debug stream, depending on the
    current setting for automatic insertion of quotes, and returns a reference to the stream.

    The default character is a double quote \c{"}.

    \sa quote(), noquote()
*/

/*!
    \fn int QDebug::verbosity() const
    \since 5.6

    Returns the verbosity of the debug stream.

    Streaming operators can check the value to decide whether
    verbose output is desired and print more information depending on the
    level. Higher values indicate that more information is desired.

    The allowed range is from 0 to 7. The default value is 2.

    \sa setVerbosity(), VerbosityLevel
*/

/*!
    \fn void QDebug::setVerbosity(int verbosityLevel)
    \since 5.6

    Sets the verbosity of the stream to \a verbosityLevel.

    The allowed range is from 0 to 7. The default value is 2.

    \sa verbosity(), VerbosityLevel
*/

/*!
    \fn QDebug &QDebug::verbosity(int verbosityLevel)
    \since 5.13

    Sets the verbosity of the stream to \a verbosityLevel and returns a reference to the stream.

    The allowed range is from 0 to 7. The default value is 2.

    \sa verbosity(), setVerbosity(), VerbosityLevel
*/

/*!
    \enum QDebug::VerbosityLevel
    \since 5.13

    This enum describes the range of verbosity levels.

    \value MinimumVerbosity
    \value DefaultVerbosity
    \value MaximumVerbosity

    \sa verbosity(), setVerbosity()
*/

/*!
    \fn QDebug &QDebug::operator<<(QChar t)

    Writes the character, \a t, to the stream and returns a reference to the
    stream. Normally, QDebug prints control characters and non-US-ASCII
    characters as their C escape sequences or their Unicode value (\\u1234). To
    print non-printable characters without transformation, enable the noquote()
    functionality, but note that some QDebug backends may not be 8-bit clean
    and may not be able to represent \c t.
*/

/*!
    \fn QDebug &QDebug::operator<<(bool t)

    Writes the boolean value, \a t, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(char t)

    Writes the character, \a t, to the stream and returns a reference to the
    stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed short t)

    Writes the signed short integer, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned short t)

    Writes then unsigned short integer, \a t, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed int t)

    Writes the signed integer, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned int t)

    Writes then unsigned integer, \a t, to the stream and returns a reference to
    the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed long t)

    Writes the signed long integer, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned long t)

    Writes then unsigned long integer, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(qint64 t)

    Writes the signed 64-bit integer, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(quint64 t)

    Writes then unsigned 64-bit integer, \a t, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(float t)

    Writes the 32-bit floating point number, \a t, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(double t)

    Writes the 64-bit floating point number, \a t, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const char *t)

    Writes the '\\0'-terminated UTF-8 string, \a t, to the stream and returns a
    reference to the stream. The string is never quoted or escaped for the
    output. Note that QDebug buffers internally as UTF-16 and may need to
    transform to 8-bit using the locale's codec in order to use some backends,
    which may cause garbled output (mojibake). Restricting to US-ASCII strings
    is recommended.
*/

/*!
    \fn QDebug &QDebug::operator<<(const char16_t *t)
    \since 6.0

    Writes the u'\\0'-terminated UTF-16 string, \a t, to the stream and returns
    a reference to the stream. The string is never quoted or escaped for the
    output. Note that QDebug buffers internally as UTF-16 and may need to
    transform to 8-bit using the locale's codec in order to use some backends,
    which may cause garbled output (mojibake). Restricting to US-ASCII strings
    is recommended.
*/

/*!
    \fn QDebug &QDebug::operator<<(char16_t t)
    \since 5.5

    Writes the UTF-16 character, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(char32_t t)
    \since 5.5

    Writes the UTF-32 character, \a t, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QString &t)

    Writes the string, \a t, to the stream and returns a reference to the
    stream. Normally, QDebug prints the string inside quotes and transforms
    non-printable characters to their Unicode values (\\u1234).

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    Output examples:
    \snippet code/src_corelib_io_qdebug.cpp 0
*/

/*!
    \since 5.10
    \fn QDebug &QDebug::operator<<(QStringView s)

    Writes the string view, \a s, to the stream and returns a reference to the
    stream. Normally, QDebug prints the string inside quotes and transforms
    non-printable characters to their Unicode values (\\u1234).

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    See the QString overload for examples.
*/

/*!
    \since 6.0
    \fn QDebug &QDebug::operator<<(QUtf8StringView s)

    Writes the string view, \a s, to the stream and returns a reference to the
    stream.

    Normally, QDebug prints the data inside quotes and transforms control or
    non-US-ASCII characters to their C escape sequences (\\xAB). This way, the
    output is always 7-bit clean and the string can be copied from the output
    and pasted back into C++ sources, if necessary.

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.
*/

/*!
    \fn QDebug &QDebug::operator<<(QLatin1StringView t)

    Writes the string, \a t, to the stream and returns a reference to the
    stream. Normally, QDebug prints the string inside quotes and transforms
    non-printable characters to their Unicode values (\\u1234).

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    See the QString overload for examples.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QByteArray &t)

    Writes the byte array, \a t, to the stream and returns a reference to the
    stream. Normally, QDebug prints the array inside quotes and transforms
    control or non-US-ASCII characters to their C escape sequences (\\xAB). This
    way, the output is always 7-bit clean and the string can be copied from the
    output and pasted back into C++ sources, if necessary.

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    Output examples:
    \snippet code/src_corelib_io_qdebug.cpp 1

    Note how QDebug needed to close and reopen the string in the way C and C++
    languages concatenate string literals so that the letter 'b' is not
    interpreted as part of the previous hexadecimal escape sequence.
*/

/*!
    \since 6.0
    \fn QDebug &QDebug::operator<<(QByteArrayView t)

    Writes the data of the observed byte array, \a t, to the stream and returns
    a reference to the stream.

    Normally, QDebug prints the data inside quotes and transforms control or
    non-US-ASCII characters to their C escape sequences (\\xAB). This way, the
    output is always 7-bit clean and the string can be copied from the output
    and pasted back into C++ sources, if necessary.

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    See the QByteArray overload for examples.
*/

/*!
    \fn QDebug &QDebug::operator<<(const void *t)

    Writes a pointer, \a t, to the stream and returns a reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(QTextStreamFunction f)
    \internal
*/

/*!
    \fn QDebug &QDebug::operator<<(QTextStreamManipulator m)
    \internal
*/

/*!
    \fn template <typename T, QDebug::if_ordering_type<T>> QDebug::operator<<(QDebug debug, T t)
    \since 6.9
    Prints the Qt or std ordering value \a t to the \a debug object.

    \constraints \c T is one of <Qt/Std>::<weak/partial/strong>_ordering.
*/

/*!
    \since 6.5
    \fn template <typename Char, typename...Args> QDebug &QDebug::operator<<(const std::basic_string<Char, Args...> &s)
    \fn template <typename Char, typename...Args> QDebug &QDebug::operator<<(std::basic_string_view<Char, Args...> s)

    Writes the string or string-view \a s to the stream and returns a reference
    to the stream.

    These operators only participate in overload resolution if \c Char is one of
    \list
    \li char
    \li char8_t (C++20 only)
    \li char16_t
    \li char32_t
    \li wchar_t
    \endlist
*/

/*!
    \since 6.6
    \fn template <typename Rep, typename Period> QDebug &QDebug::operator<<(std::chrono::duration<Rep, Period> duration)

    Prints the time duration \a duration to the stream and returns a reference
    to the stream. The printed string is the numeric representation of the
    period followed by the time unit, similar to what the C++ Standard Library
    would produce with \c{std::ostream}.

    The unit is not localized.
*/

/*!
    \fn template <typename T, QDebug::if_qint128<T>> QDebug::operator<<(T i)
    \fn template <typename T, QDebug::if_quint128<T>> QDebug::operator<<(T i)
    \since 6.7

    Prints the textual representation of the 128-bit integer \a i.

    \note This operator is only available if Qt supports 128-bit integer types.
    If 128-bit integer types are available in your build, but the Qt libraries
    were compiled without, the operator will print a warning instead.

    \note Because the operator is a function template, no implicit conversions
    are performed on its argument. It must be exactly qint128/quint128.

    \sa QT_SUPPORTS_INT128
*/

/*!
    \fn template <class T> QString QDebug::toString(const T &object)
    \since 6.0

    \include qdebug-toString.qdocinc

    \sa toBytes()
*/

/*! \internal */
QString QDebug::toStringImpl(StreamTypeErased s, const void *obj)
{
    QString result;
    {
        QDebug d(&result);
        s(d.nospace(), obj);
    }
    return result;
}

/*!
    \fn template <class T> QByteArray QDebug::toBytes(const T &object)
    \since 6.9

    This is equivalent to passing \a object to
    \c{QDebug::toString(object).toUtf8()}, but more efficient.

    \sa toString()
*/

/*! \internal */
QByteArray QDebug::toBytesImpl(StreamTypeErased s, const void *obj)
{
    QByteArray result;
    {
        QDebug d(&result);
        s(d.nospace(), obj);
    }
    return result;
}

/*!
    \internal
    \since 6.9

    Outputs a heterogeneous product type (pair, tuple, or anything that
    implements the Tuple Protocol). The class name is described by "\a ns
    \c{::} \a what", while the addresses of the \a n elements are stored in the
    array \a data. The formatters are stored in the array \a ops.

    If \a ns is empty, only \a what is used.
*/
QDebug &QDebug::putTupleLikeImplImpl(const char *ns, const char *what,
                                     size_t n, StreamTypeErased *ops, const void **data)
{
    const QDebugStateSaver saver(*this);
    nospace();
    if (ns && *ns)
        *this << ns << "::";
    *this << what << '(';
    while (n--) {
        (*ops++)(*this, *data++);
        if (n)
            *this << ", ";
    }
    return *this << ')';
}

/*!
    \fn template <class T> QDebug operator<<(QDebug debug, const QList<T> &list)
    \relates QDebug

    Writes the contents of \a list to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template <class T, qsizetype P> QDebug operator<<(QDebug debug, const QVarLengthArray<T,P> &array)
    \relates QDebug
    \since 6.3

    Writes the contents of \a array to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template <typename T, typename Alloc> QDebug operator<<(QDebug debug, const std::list<T, Alloc> &vec)
    \relates QDebug
    \since 5.7

    Writes the contents of list \a vec to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template <typename T, typename Alloc> QDebug operator<<(QDebug debug, const std::vector<T, Alloc> &vec)
    \relates QDebug
    \since 5.7

    Writes the contents of vector \a vec to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template <typename T, std::size_t N> QDebug operator<<(QDebug debug, const std::array<T, N> &array)
    \relates QDebug
    \since 6.9

    Writes the contents of \a array to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template <typename T> QDebug operator<<(QDebug debug, const QSet<T> &set)
    \relates QDebug

    Writes the contents of \a set to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template <class Key, class T> QDebug operator<<(QDebug debug, const QMap<Key, T> &map)
    \relates QDebug

    Writes the contents of \a map to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <class Key, class T> QDebug operator<<(QDebug debug, const QMultiMap<Key, T> &map)
    \relates QDebug

    Writes the contents of \a map to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <typename Key, typename T, typename Compare, typename Alloc> QDebug operator<<(QDebug debug, const std::map<Key, T, Compare, Alloc> &map)
    \relates QDebug
    \since 5.7

    Writes the contents of \a map to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <typename Key, typename T, typename Compare, typename Alloc> QDebug operator<<(QDebug debug, const std::multimap<Key, T, Compare, Alloc> &map)
    \relates QDebug
    \since 5.7

    Writes the contents of \a map to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <typename Key, typename Compare, typename Alloc> QDebug operator<<(QDebug debug, const std::multiset<Key, Compare, Alloc> &multiset)
    \relates QDebug
    \since 6.9

    Writes the contents of \a multiset to \a debug. The \c Key type
    needs to support streaming into QDebug.
*/

/*!
    \fn template <typename Key, typename Compare, typename Alloc> QDebug operator<<(QDebug debug, const std::set<Key, Compare, Alloc> &set)
    \relates QDebug
    \since 6.9

    Writes the contents of \a set to \a debug. The \c Key type
    needs to support streaming into QDebug.
*/

/*!
    \fn template <typename Key, typename T, typename Hash, typename KeyEqual, typename Alloc> QDebug operator<<(QDebug debug, const std::unordered_map<Key, T, Hash, KeyEqual, Alloc> &map)
    \relates QDebug
    \since 6.9

    Writes the contents of \a map to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <typename Key, typename Hash, typename KeyEqual, typename Alloc> QDebug operator<<(QDebug debug, const std::unordered_set<Key, Hash, KeyEqual, Alloc> &unordered_set)
    \relates QDebug
    \since 6.9

    Writes the contents of \a unordered_set to \a debug. The \c Key type
    needs to support streaming into QDebug.
*/

/*!
    \fn template <class Key, class T> QDebug operator<<(QDebug debug, const QHash<Key, T> &hash)
    \relates QDebug

    Writes the contents of \a hash to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <class Key, class T> QDebug operator<<(QDebug debug, const QMultiHash<Key, T> &hash)
    \relates QDebug

    Writes the contents of \a hash to \a debug. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn template <class...Ts, QDebug::if_streamable<Ts...>> QDebug &QDebug::operator<<(const std::tuple<Ts...> &tuple)
    \since 6.9

    Writes the contents of \a tuple to the stream. All \c Ts... need to support
    streaming into QDebug.
*/

/*!
    \fn template <class T1, class T2> QDebug operator<<(QDebug debug, const std::pair<T1, T2> &pair)
    \relates QDebug

    Writes the contents of \a pair to \a debug. Both \c T1 and
    \c T2 need to support streaming into QDebug.
*/

/*!
    \since 6.7
    \fn template <class T, QDebug::if_streamable<T>> QDebug::operator<<(const std::optional<T> &opt)

    Writes the contents of \a opt (or \c nullopt if not set) to this stream.
    \c T needs to support streaming into QDebug.
*/

/*!
    \fn template <typename T> QDebug operator<<(QDebug debug, const QContiguousCache<T> &cache)
    \relates QDebug

    Writes the contents of \a cache to \a debug. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn template<typename T> QDebug operator<<(QDebug debug, const QFlags<T> &flags)
    \relates QDebug
    \since 4.7

    Writes \a flags to \a debug.
*/

/*!
    \fn template<typename T> QDebug operator<<(QDebug debug, const QSharedPointer<T> &ptr)
    \relates QSharedPointer
    \since 5.7

    Writes the pointer tracked by \a ptr into the debug object \a debug for
    debugging purposes.

    \sa {Debugging Techniques}
*/

/*!
  \fn QDebug &QDebug::operator<<(std::nullptr_t)
  \internal
 */

/*!
    \since 6.7
    \fn QDebug &QDebug::operator<<(std::nullopt_t)

    Writes nullopt to the stream.
*/

/*!
    \class QDebugStateSaver
    \inmodule QtCore
    \brief Convenience class for custom QDebug operators.

    Saves the settings used by QDebug, and restores them upon destruction,
    then calls \l {QDebug::maybeSpace()}{maybeSpace()}, to separate arguments with a space if
    \l {QDebug::autoInsertSpaces()}{autoInsertSpaces()} was true at the time of constructing the QDebugStateSaver.

    The automatic insertion of spaces between writes is one of the settings
    that QDebugStateSaver stores for the duration of the current block.

    The settings of the internal QTextStream are also saved and restored,
    so that using << Qt::hex in a QDebug operator doesn't affect other QDebug
    operators.

    QDebugStateSaver is typically used in the implementation of an operator<<() for debugging:

    \snippet customtype/customtypeexample.cpp custom type streaming operator

    \since 5.1
*/

class QDebugStateSaverPrivate
{
public:
    QDebugStateSaverPrivate(QDebug::Stream *stream)
        : m_stream(stream),
          m_spaces(stream->space),
          m_noQuotes(stream->noQuotes),
          m_verbosity(stream->verbosity),
          m_streamParams(stream->ts.d_ptr->params)
    {
    }
    void restoreState()
    {
        const bool currentSpaces = m_stream->space;
        if (currentSpaces && !m_spaces)
            if (m_stream->buffer.endsWith(u' '))
                m_stream->buffer.chop(1);

        m_stream->space = m_spaces;
        m_stream->noQuotes = m_noQuotes;
        m_stream->ts.d_ptr->params = m_streamParams;
        m_stream->verbosity = m_verbosity;

        if (!currentSpaces && m_spaces)
            m_stream->ts << ' ';
    }

    QDebug::Stream *m_stream;

    // QDebug state
    const bool m_spaces;
    const bool m_noQuotes;
    const int m_verbosity;

    // QTextStream state
    const QTextStreamPrivate::Params m_streamParams;
};


/*!
    Creates a QDebugStateSaver instance, which saves the settings
    currently used by \a dbg.

    \sa QDebug::setAutoInsertSpaces(), QDebug::autoInsertSpaces()
*/
QDebugStateSaver::QDebugStateSaver(QDebug &dbg)
    : d(new QDebugStateSaverPrivate(dbg.stream))
{
}

/*!
    Destroys a QDebugStateSaver instance, which restores the settings
    used when the QDebugStateSaver instance was created.

    \sa QDebug::setAutoInsertSpaces(), QDebug::autoInsertSpaces()
*/
QDebugStateSaver::~QDebugStateSaver()
{
    d->restoreState();
}

/*!
    \internal

    Specialization of the primary template in qdebug.h to out-of-line
    the common case of QFlags<T>::Int being 32-bit.

    Just call the generic version so the two don't get out of sync.
*/
void qt_QMetaEnum_flagDebugOperator(QDebug &debug, size_t sizeofT, uint value)
{
    qt_QMetaEnum_flagDebugOperator(debug, sizeofT, quint64(value));
}

/*!
    \internal
    Ditto, for 64-bit.
*/
void qt_QMetaEnum_flagDebugOperator(QDebug &debug, size_t sizeofT, quint64 value)
{
    qt_QMetaEnum_flagDebugOperator<quint64>(debug, sizeofT, value);
}


#ifndef QT_NO_QOBJECT
/*!
    \internal

    Formats the given enum \a value for debug output.

    The supported verbosity are:

      0: Just the key, or value with enum name if no key is found:

         MyEnum2
         MyEnum(123)
         MyScopedEnum::Enum3
         MyScopedEnum(456)

      1: Same as 0, but treating all enums as scoped:

         MyEnum::MyEnum2
         MyEnum(123)
         MyScopedEnum::Enum3
         MyScopedEnum(456)

      2: The QDebug default. Same as 0, and includes class/namespace scope:

         MyNamespace::MyClass::MyEnum2
         MyNamespace::MyClass::MyEnum(123)
         MyNamespace::MyClass::MyScopedEnum::Enum3
         MyNamespace::MyClass::MyScopedEnum(456)

      3: Same as 2, but treating all enums as scoped:

         MyNamespace::MyClass::MyEnum::MyEnum2
         MyNamespace::MyClass::MyEnum(123)
         MyNamespace::MyClass::MyScopedEnum::Enum3
         MyNamespace::MyClass::MyScopedEnum(456)
 */
QDebug qt_QMetaEnum_debugOperator(QDebug &dbg, qint64 value, const QMetaObject *meta, const char *name)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));

    const int verbosity = dbg.verbosity();
    if (verbosity >= QDebug::DefaultVerbosity) {
        if (const char *scope = me.scope())
            dbg << scope << u"::";
    }

    const char *key = me.valueToKey(static_cast<int>(value));
    const bool scoped = me.isScoped() || verbosity & 1;
    if (scoped || !key)
        dbg << me.enumName() << (!key ? u"(" : u"::");

    if (key)
        dbg << key;
    else
        dbg << value << ')';

    return dbg;
}

/*!
    \fn QDebug qt_QMetaEnum_flagDebugOperator(QDebug &, quint64 value, const QMetaObject *, const char *name)
    \internal

    Formats the given flag \a value for debug output.

    The supported verbosity are:

      0: Just the key(s):

         MyFlag1
         MyFlag2|MyFlag3
         MyScopedFlag(MyFlag2)
         MyScopedFlag(MyFlag2|MyFlag3)

      1: Same as 0, but treating all flags as scoped:

         MyFlag(MyFlag1)
         MyFlag(MyFlag2|MyFlag3)
         MyScopedFlag(MyFlag2)
         MyScopedFlag(MyFlag2|MyFlag3)

      2: The QDebug default. Same as 1, and includes class/namespace scope:

         QFlags<MyNamespace::MyClass::MyFlag>(MyFlag1)
         QFlags<MyNamespace::MyClass::MyFlag>(MyFlag2|MyFlag3)
         QFlags<MyNamespace::MyClass::MyScopedFlag>(MyFlag2)
         QFlags<MyNamespace::MyClass::MyScopedFlag>(MyFlag2|MyFlag3)
 */
QDebug qt_QMetaEnum_flagDebugOperator(QDebug &debug, quint64 value, const QMetaObject *meta, const char *name)
{
    const int verbosity = debug.verbosity();

    QDebugStateSaver saver(debug);
    debug.resetFormat();
    debug.noquote();
    debug.nospace();

    const QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));

    const bool classScope = verbosity >= QDebug::DefaultVerbosity;
    if (classScope) {
        debug << u"QFlags<";

        if (const char *scope = me.scope())
            debug << scope << u"::";
    }

    const bool enumScope = me.isScoped() || verbosity > QDebug::MinimumVerbosity;
    if (enumScope) {
        debug << me.enumName();
        if (classScope)
            debug << '>';
        debug << '(';
    }

    debug << me.valueToKeys(value);

    if (enumScope)
        debug << ')';

    return debug;
}

/*!
    \macro QDebug qDebug()
    \relates QDebug
    \threadsafe

    Returns a QDebug object that logs a debug message to the central message handler.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 25

    Using qDebug() is an alternative to \l{qDebug(const char *, ...)},
    which follows the printf paradigm.

    Note that QDebug and the type specific stream operators do add various
    formatting to make the debug message easier to read. See the
     \l{Formatting Options}{formatting options} documentation for more details.

    This function does nothing if \c QT_NO_DEBUG_OUTPUT was defined during
    compilation.

    \sa {qDebug(const char *, ...)}, qCDebug()
*/

/*!
    \macro QDebug qInfo()
    \relates QDebug
    \threadsafe

    Returns a QDebug object that logs an informational message to the central message handler.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp qInfo_stream

    Using qInfo() is an alternative to \l{qInfo(const char *, ...)},
    which follows the printf paradigm.

    Note that QDebug and the type specific stream operators do add various
    formatting to make the debug message easier to read. See the
    \l{Formatting Options}{formatting options} documentation for more details.

    This function does nothing if \c QT_NO_INFO_OUTPUT was defined during
    compilation.

    \sa {qInfo(const char *, ...)}, qCInfo()
*/

/*!
    \macro QDebug qWarning()
    \relates QDebug
    \threadsafe

    Returns a QDebug object that logs a warning message to the central message handler.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 27

    Using qWarning() is an alternative to \l{qWarning(const char *, ...)},
    which follows the printf paradigm.

    Note that QDebug and the type specific stream operators do add various
    formatting to make the debug message easier to read. See the
    \l{Formatting Options}{formatting options} documentation for more details.

    This function does nothing if \c QT_NO_WARNING_OUTPUT was defined during
    compilation.

    For debugging purposes, it is sometimes convenient to let the
    program abort for warning messages. This allows you then
    to inspect the core dump, or attach a debugger - see also \l{qFatal()}.
    To enable this, set the environment variable \c{QT_FATAL_WARNINGS}
    to a number \c n. The program terminates then for the n-th warning.
    That is, if the environment variable is set to 1, it will terminate
    on the first call; if it contains the value 10, it will exit on the 10th
    call. Any non-numeric value in the environment variable is equivalent to 1.

    \sa {qWarning(const char *, ...)}, qCWarning()
*/

/*!
    \macro QDebug qCritical()
    \relates QDebug
    \threadsafe

    Returns a QDebug object that logs a critical message to the central message handler.

    Example:

    \snippet code/src_corelib_global_qglobal.cpp 29

    Using qCritical() is an alternative to \l{qCritical(const char *, ...)},
    which follows the printf paradigm.

    Note that QDebug and the type specific stream operators do add various
    formatting to make the debug message easier to read. See the
    \l{Formatting Options}{formatting options} documentation for more details.

    For debugging purposes, it is sometimes convenient to let the
    program abort for critical messages. This allows you then
    to inspect the core dump, or attach a debugger - see also \l{qFatal()}.
    To enable this, set the environment variable \c{QT_FATAL_CRITICALS}
    to a number \c n. The program terminates then for the n-th critical
    message.
    That is, if the environment variable is set to 1, it will terminate
    on the first call; if it contains the value 10, it will exit on the 10th
    call. Any non-numeric value in the environment variable is equivalent to 1.

    \sa {qCritical(const char *, ...)}, qCCritical()
*/

/*!
    \macro QDebug qFatal()
    \relates QDebug
    \threadsafe

    Returns a QDebug object that logs a fatal message to the central message handler.

    Using qFatal() is an alternative to \l{qFatal(const char *, ...)},
    which follows the printf paradigm.

    Note that QDebug and the type specific stream operators do add various
    formatting to make the debug message easier to read. See the
    \l{Formatting Options}{formatting options} documentation for more details.

    If you are using the \b{default message handler}, the returned stream will abort
    to create a core dump. On Windows, for debug builds,
    this function will report a _CRT_ERROR enabling you to connect a debugger
    to the application.

    \sa {qFatal(const char *, ...)}, qCFatal()
*/

#endif // !QT_NO_QOBJECT

QT_END_NAMESPACE
