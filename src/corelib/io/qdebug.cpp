/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2014 Intel Corporation.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifdef QT_NO_DEBUG
#undef QT_NO_DEBUG
#endif
#ifdef qDebug
#undef qDebug
#endif

#include "qdebug.h"
#include "qmetaobject.h"
#include <private/qtextstream_p.h>
#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE

using QtMiscUtils::toHexUpper;
using QtMiscUtils::fromHex;

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
    value of QtDebugMsg. Similarly, the qWarning(), qCritical() and qFatal()
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
    \fn QDebug::QDebug(QtMsgType type)

    Constructs a debug stream that writes to the handler for the message type specified by \a type.
*/

/*!
    \fn QDebug::QDebug(const QDebug &other)

    Constructs a copy of the \a other debug stream.
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
// Has been defined in the header / inlined before Qt 5.4
QDebug::~QDebug()
{
    if (!--stream->ref) {
        if (stream->space && stream->buffer.endsWith(QLatin1Char(' ')))
            stream->buffer.chop(1);
        if (stream->message_output) {
            qt_message_output(stream->type,
                              stream->context,
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
        stream->ts << hex << "\\x" << ucs4 << reset;
    } else if (ucs4 < 0x80) {
        stream->ts << char(ucs4);
    } else {
        stream->ts << hex << qSetPadChar(QLatin1Char('0'));
        if (ucs4 < 0x10000)
            stream->ts << qSetFieldWidth(4) << "\\u";
        else
            stream->ts << qSetFieldWidth(8) << "\\U";
        stream->ts << ucs4 << reset;
    }
    maybeQuote('\'');
}

// These two functions return true if the character should be printed by QDebug.
// For QByteArray, this is technically identical to US-ASCII isprint();
// for QString, we use QChar::isPrint, which requires a full UCS-4 decode.
static inline bool isPrintable(uint ucs4)
{ return QChar::isPrint(ucs4); }
static inline bool isPrintable(ushort uc)
{ return QChar::isPrint(uc); }
static inline bool isPrintable(uchar c)
{ return c >= ' ' && c < 0x7f; }

template <typename Char>
static inline void putEscapedString(QTextStreamPrivate *d, const Char *begin, int length, bool isUnicode = true)
{
    QChar quote(QLatin1Char('"'));
    d->write(&quote, 1);

    bool lastWasHexEscape = false;
    const Char *end = begin + length;
    for (const Char *p = begin; p != end; ++p) {
        // check if we need to insert "" to break an hex escape sequence
        if (Q_UNLIKELY(lastWasHexEscape)) {
            if (fromHex(*p) != -1) {
                // yes, insert it
                QChar quotes[] = { QLatin1Char('"'), QLatin1Char('"') };
                d->write(quotes, 2);
            }
            lastWasHexEscape = false;
        }

        if (sizeof(Char) == sizeof(QChar)) {
            // Surrogate characters are category Cs (Other_Surrogate), so isPrintable = false for them
            int runLength = 0;
            while (p + runLength != end &&
                   isPrintable(p[runLength]) && p[runLength] != '\\' && p[runLength] != '"')
                ++runLength;
            if (runLength) {
                d->write(reinterpret_cast<const QChar *>(p), runLength);
                p += runLength - 1;
                continue;
            }
        } else if (isPrintable(*p) && *p != '\\' && *p != '"') {
            QChar c = QLatin1Char(*p);
            d->write(&c, 1);
            continue;
        }

        // print as an escape sequence (maybe, see below for surrogate pairs)
        int buflen = 2;
        ushort buf[sizeof "\\U12345678" - 1];
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
                    uint ucs4 = QChar::surrogateToUcs4(*p, p[1]);
                    if (isPrintable(ucs4)) {
                        buf[0] = *p;
                        buf[1] = p[1];
                        buflen = 2;
                    } else {
                        buf[1] = 'U';
                        buf[2] = '0'; // toHexUpper(ucs4 >> 32);
                        buf[3] = '0'; // toHexUpper(ucs4 >> 28);
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
            buf[2] = toHexUpper(ushort(*p) >> 12);
            buf[3] = toHexUpper(ushort(*p) >> 8);
            buf[4] = toHexUpper(*p >> 4);
            buf[5] = toHexUpper(*p);
            buflen = 6;
        }
        d->write(reinterpret_cast<QChar *>(buf), buflen);
    }

    d->write(&quote, 1);
}

/*!
    \internal
    Duplicated from QtTest::toPrettyUnicode().
*/
void QDebug::putString(const QChar *begin, size_t length)
{
    if (stream->testFlag(Stream::NoQuotes)) {
        // no quotes, write the string directly too (no pretty-printing)
        // this respects the QTextStream state, though
        stream->ts.d_ptr->putString(begin, int(length));
    } else {
        // we'll reset the QTextStream formatting mechanisms, so save the state
        QDebugStateSaver saver(*this);
        stream->ts.d_ptr->params.reset();
        putEscapedString(stream->ts.d_ptr.data(), reinterpret_cast<const ushort *>(begin), int(length));
    }
}

/*!
    \internal
    Duplicated from QtTest::toPrettyCString().
*/
void QDebug::putByteArray(const char *begin, size_t length, Latin1Content content)
{
    if (stream->testFlag(Stream::NoQuotes)) {
        // no quotes, write the string directly too (no pretty-printing)
        // this respects the QTextStream state, though
        QString string = content == ContainsLatin1 ? QString::fromLatin1(begin, int(length)) : QString::fromUtf8(begin, int(length));
        stream->ts.d_ptr->putString(string);
    } else {
        // we'll reset the QTextStream formatting mechanisms, so save the state
        QDebugStateSaver saver(*this);
        stream->ts.d_ptr->params.reset();
        putEscapedString(stream->ts.d_ptr.data(), reinterpret_cast<const uchar *>(begin),
                         int(length), content == ContainsLatin1);
    }
}

/*!
    \fn QDebug::swap(QDebug &other)
    \since 5.0

    Swaps this debug stream instance with \a other. This function is
    very fast and never fails.
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
    if (stream->context.version > 1)
        stream->flags = 0;
    stream->setVerbosity(Stream::defaultVerbosity);
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

    \sa setVerbosity()
*/

/*!
    \fn void QDebug::setVerbosity(int verbosityLevel)
    \since 5.6

    Sets the verbosity of the stream to \a verbosityLevel.

    The allowed range is from 0 to 7. The default value is 2.

    \sa verbosity()
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
    \fn QDebug &QDebug::operator<<(signed short i)

    Writes the signed short integer, \a i, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned short i)

    Writes then unsigned short integer, \a i, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed int i)

    Writes the signed integer, \a i, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned int i)

    Writes then unsigned integer, \a i, to the stream and returns a reference to
    the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(signed long l)

    Writes the signed long integer, \a l, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(unsigned long l)

    Writes then unsigned long integer, \a l, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(qint64 i)

    Writes the signed 64-bit integer, \a i, to the stream and returns a reference
    to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(quint64 i)

    Writes then unsigned 64-bit integer, \a i, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(float f)

    Writes the 32-bit floating point number, \a f, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(double f)

    Writes the 64-bit floating point number, \a f, to the stream and returns a
    reference to the stream.
*/

/*!
    \fn QDebug &QDebug::operator<<(const char *s)

    Writes the '\\0'-terminated string, \a s, to the stream and returns a
    reference to the stream. The string is never quoted nor transformed to the
    output, but note that some QDebug backends might not be 8-bit clean.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QString &s)

    Writes the string, \a s, to the stream and returns a reference to the
    stream. Normally, QDebug prints the string inside quotes and transforms
    non-printable characters to their Unicode values (\\u1234).

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    Output examples:
    \code
        QString s;

        s = "a";
        qDebug().noquote() << s;    // prints: a
        qDebug() << s;              // prints: "a"

        s = "\"a\r\n\"";
        qDebug() << s;              // prints: "\"a\r\n\""

        s = "\033";                 // escape character
        qDebug() << s;              // prints: "\u001B"

        s = "\u00AD";               // SOFT HYPHEN
        qDebug() << s;              // prints: "\u00AD"

        s = "\u00E1";               // LATIN SMALL LETTER A WITH ACUTE
        qDebug() << s;              // prints: "á"

        s = "a\u0301";              // "a" followed by COMBINING ACUTE ACCENT
        qDebug() << s;              // prints: "á";

        s = "\u0430\u0301";         // CYRILLIC SMALL LETTER A followed by COMBINING ACUTE ACCENT
        qDebug() << s;              // prints: "а́"
    \endcode
*/

/*!
    \fn QDebug &QDebug::operator<<(const QStringRef &s)

    Writes the string, \a s, to the stream and returns a reference to the
    stream. Normally, QDebug prints the string inside quotes and transforms
    non-printable characters to their Unicode values (\\u1234).

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    See the QString overload for examples.
*/

/*!
    \fn QDebug &QDebug::operator<<(QLatin1String s)

    Writes the string, \a s, to the stream and returns a reference to the
    stream. Normally, QDebug prints the string inside quotes and transforms
    non-printable characters to their Unicode values (\\u1234).

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    See the QString overload for examples.
*/

/*!
    \fn QDebug &QDebug::operator<<(const QByteArray &b)

    Writes the byte array, \a b, to the stream and returns a reference to the
    stream. Normally, QDebug prints the array inside quotes and transforms
    control or non-US-ASCII characters to their C escape sequences (\\xAB). This
    way, the output is always 7-bit clean and the string can be copied from the
    output and pasted back into C++ sources, if necessary.

    To print non-printable characters without transformation, enable the
    noquote() functionality. Note that some QDebug backends might not be 8-bit
    clean.

    Output examples:
    \code
        QByteArray ba;

        ba = "a";
        qDebug().noquote() << ba;    // prints: a
        qDebug() << ba;              // prints: "a"

        ba = "\"a\r\n\"";
        qDebug() << ba;              // prints: "\"a\r\n\""

        ba = "\033";                 // escape character
        qDebug() << ba;              // prints: "\x1B"

        ba = "\xC3\xA1";
        qDebug() << ba;              // prints: "\xC3\xA1"

        ba = QByteArray("a\0b", 3);
        qDebug() << ba               // prints: "\a\x00""b"
    \endcode

    Note how QDebug needed to close and reopen the string in the way C and C++
    languages concatenate string literals so that the letter 'b' is not
    interpreted as part of the previous hexadecimal escape sequence.
*/

/*!
    \fn QDebug &QDebug::operator<<(const void *p)

    Writes a pointer, \a p, to the stream and returns a reference to the stream.
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
    \fn QDebug operator<<(QDebug stream, const QList<T> &list)
    \relates QDebug

    Writes the contents of \a list to \a stream. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn QDebug operator<<(QDebug stream, const QVector<T> &vector)
    \relates QDebug

    Writes the contents of \a vector to \a stream. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn QDebug operator<<(QDebug stream, const QSet<T> &set)
    \relates QDebug

    Writes the contents of \a set to \a stream. \c T needs to
    support streaming into QDebug.
*/

/*!
    \fn QDebug operator<<(QDebug stream, const QMap<Key, T> &map)
    \relates QDebug

    Writes the contents of \a map to \a stream. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn QDebug operator<<(QDebug stream, const QHash<Key, T> &hash)
    \relates QDebug

    Writes the contents of \a hash to \a stream. Both \c Key and
    \c T need to support streaming into QDebug.
*/

/*!
    \fn QDebug operator<<(QDebug stream, const QPair<T1, T2> &pair)
    \relates QDebug

    Writes the contents of \a pair to \a stream. Both \c T1 and
    \c T2 need to support streaming into QDebug.
*/

/*!
    \fn QDebug operator<<(QDebug stream, const QFlags<T> &flag)
    \relates QDebug
    \since 4.7

    Writes \a flag to \a stream.
*/

/*!
    \class QDebugStateSaver
    \inmodule QtCore
    \brief Convenience class for custom QDebug operators

    Saves the settings used by QDebug, and restores them upon destruction,
    then calls \l {QDebug::maybeSpace()}{maybeSpace()}, to separate arguments with a space if
    \l {QDebug::autoInsertSpaces()}{autoInsertSpaces()} was true at the time of constructing the QDebugStateSaver.

    The automatic insertion of spaces between writes is one of the settings
    that QDebugStateSaver stores for the duration of the current block.

    The settings of the internal QTextStream are also saved and restored,
    so that using << hex in a QDebug operator doesn't affect other QDebug
    operators.

    \since 5.1
*/

class QDebugStateSaverPrivate
{
public:
    QDebugStateSaverPrivate(QDebug &dbg)
        : m_dbg(dbg),
          m_spaces(dbg.autoInsertSpaces()),
          m_flags(0),
          m_streamParams(dbg.stream->ts.d_ptr->params)
    {
        if (m_dbg.stream->context.version > 1)
            m_flags = m_dbg.stream->flags;
    }
    void restoreState()
    {
        const bool currentSpaces = m_dbg.autoInsertSpaces();
        if (currentSpaces && !m_spaces)
            if (m_dbg.stream->buffer.endsWith(QLatin1Char(' ')))
                m_dbg.stream->buffer.chop(1);

        m_dbg.setAutoInsertSpaces(m_spaces);
        m_dbg.stream->ts.d_ptr->params = m_streamParams;
        if (m_dbg.stream->context.version > 1)
            m_dbg.stream->flags = m_flags;

        if (!currentSpaces && m_spaces)
            m_dbg.stream->ts << ' ';
    }

    QDebug &m_dbg;

    // QDebug state
    const bool m_spaces;
    int m_flags;

    // QTextStream state
    const QTextStreamPrivate::Params m_streamParams;
};


/*!
    Creates a QDebugStateSaver instance, which saves the settings
    currently used by \a dbg.

    \sa QDebug::setAutoInsertSpaces(), QDebug::autoInsertSpaces()
*/
QDebugStateSaver::QDebugStateSaver(QDebug &dbg)
    : d(new QDebugStateSaverPrivate(dbg))
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

#ifndef QT_NO_QOBJECT
/*!
    \internal
 */
QDebug qt_QMetaEnum_debugOperator(QDebug &dbg, int value, const QMetaObject *meta, const char *name)
{
    QDebugStateSaver saver(dbg);
    QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));
    const char *key = me.valueToKey(value);
    dbg.nospace() << meta->className() << "::" << name << '(';
    if (key)
        dbg << key;
    else
        dbg << value;
    dbg << ')';
    return dbg;
}

QDebug qt_QMetaEnum_flagDebugOperator(QDebug &debug, quint64 value, const QMetaObject *meta, const char *name)
{
    QDebugStateSaver saver(debug);
    debug.resetFormat();
    debug.noquote();
    debug.nospace();
    debug << "QFlags<";
    const QMetaEnum me = meta->enumerator(meta->indexOfEnumerator(name));
    if (const char *scope = me.scope())
        debug << scope << "::";
    debug << me.name() << ">(" << me.valueToKeys(value) << ')';
    return debug;
}
#endif // !QT_NO_QOBJECT

QT_END_NAMESPACE
