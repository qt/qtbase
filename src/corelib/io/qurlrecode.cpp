/****************************************************************************
**
** Copyright (C) 2012 Intel Corporation
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qurl.h"

QT_BEGIN_NAMESPACE

// ### move to qurl_p.h
enum EncodingAction {
    DecodeCharacter = 0,
    LeaveCharacter = 1,
    EncodeCharacter = 2
};

// From RFC 3896, Appendix A Collected ABNF for URI
//    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
//    reserved      = gen-delims / sub-delims
//    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                  / "*" / "+" / "," / ";" / "="
static const uchar defaultActionTable[96] = {
    2, // space
    1, // '!' (sub-delim)
    2, // '"'
    1, // '#' (gen-delim)
    1, // '$' (gen-delim)
    2, // '%' (percent)
    1, // '&' (gen-delim)
    1, // "'" (sub-delim)
    1, // '(' (sub-delim)
    1, // ')' (sub-delim)
    1, // '*' (sub-delim)
    1, // '+' (sub-delim)
    1, // ',' (sub-delim)
    0, // '-' (unreserved)
    0, // '.' (unreserved)
    1, // '/' (gen-delim)

    0, 0, 0, 0, 0,  // '0' to '4' (unreserved)
    0, 0, 0, 0, 0,  // '5' to '9' (unreserved)
    1, // ':' (gen-delim)
    1, // ';' (sub-delim)
    2, // '<'
    1, // '=' (sub-delim)
    2, // '>'
    1, // '?' (gen-delim)

    1, // '@' (gen-delim)
    0, 0, 0, 0, 0,  // 'A' to 'E' (unreserved)
    0, 0, 0, 0, 0,  // 'F' to 'J' (unreserved)
    0, 0, 0, 0, 0,  // 'K' to 'O' (unreserved)
    0, 0, 0, 0, 0,  // 'P' to 'T' (unreserved)
    0, 0, 0, 0, 0, 0,  // 'U' to 'Z' (unreserved)
    1, // '[' (gen-delim)
    2, // '\'
    1, // ']' (gen-delim)
    2, // '^'
    0, // '_' (unreserved)

    2, // '`'
    0, 0, 0, 0, 0,  // 'a' to 'e' (unreserved)
    0, 0, 0, 0, 0,  // 'f' to 'j' (unreserved)
    0, 0, 0, 0, 0,  // 'k' to 'o' (unreserved)
    0, 0, 0, 0, 0,  // 'p' to 't' (unreserved)
    0, 0, 0, 0, 0, 0,  // 'u' to 'z' (unreserved)
    2, // '{'
    2, // '|'
    2, // '}'
    0, // '~' (unreserved)

    2  // BSKP
};

static inline bool isHex(ushort c)
{
    return (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            (c >= '0' && c <= '9');
}

static inline bool isUpperHex(ushort c)
{
    // undefined behaviour if c isn't an hex char!
    return c < 0x60;
}

static inline ushort toUpperHex(ushort c)
{
    return isUpperHex(c) ? c : c - 0x20;
}

static inline ushort decodeNibble(ushort c)
{
    return c >= 'a' ? c - 'a' + 0xA :
           c >= 'A' ? c - 'A' + 0xA : c - '0';
}

static inline ushort encodeNibble(ushort c)
{
    static const uchar hexnumbers[] = "0123456789ABCDEF";
    return hexnumbers[c & 0xf];
}

static void ensureDetached(QString &result, ushort *&output, const ushort *input, const ushort *end)
{
    if (!output) {
        // now detach
        // create enough space if the rest of the string needed to be percent-encoded
        int charsProcessed = input - reinterpret_cast<const ushort *>(result.constData()) - 1;
        int charsRemaining = end - input + 1;
        int newSize = result.size() + 2 * charsRemaining;
        result.resize(newSize);

        // set the output variable
        output = reinterpret_cast<ushort *>(result.data()) + charsProcessed;
    }
}

static inline bool isUnicodeNonCharacter(uint ucs4)
{
    // Unicode has a couple of "non-characters" that one can use internally,
    // but are not allowed to be used for text interchange.
    //
    // Those are the last two entries each Unicode Plane (U+FFFE, U+FFFF,
    // U+1FFFE, U+1FFFF, etc.) as well as the entries between U+FDD0 and
    // U+FDEF (inclusive)

    return (ucs4 & 0xfffe) == 0xfffe
            || (ucs4 - 0xfdd0U) < 16;
}

// returns true if we performed an UTF-8 decoding
static uint encodedUtf8ToUcs4(QString &result, ushort *&output, const ushort *&input, const ushort *end, ushort decoded)
{
    if (decoded <= 0xC1) {
        // an UTF-8 first character must be at least 0xC0
        // however, all 0xC0 and 0xC1 first bytes can only produce overlong sequences
        return false;
    }

    int charsNeeded;
    uint min_uc;
    uint uc;
    if (decoded < 0xe0) {
        charsNeeded = 1;
        min_uc = 0x80;
        uc = decoded & 0x1f;
    } else if (decoded < 0xf0) {
        charsNeeded = 2;
        min_uc = 0x800;
        uc = decoded & 0x0f;
    } else if (decoded < 0xf5) {
        charsNeeded = 3;
        min_uc = 0x10000;
        uc = decoded & 0x07;
    } else {
        // the last Unicode character is U+10FFFF
        // it's encoded in UTF-8 as "\xF4\x8F\xBF\xBF"
        // therefore, a byte outside the range 0xC0..0xF4 is not the UTF-8 first byte
        return false;
    }

    // are there enough remaining?
    if (end - input < 3*charsNeeded + 2)
        return false;

    if (input[2] != '%')
        return false;

    // first continuation character
    decoded = (decodeNibble(input[3]) << 4) | decodeNibble(input[4]);
    if ((decoded & 0xc0) != 0x80)
        return false;
    uc <<= 6;
    uc |= decoded & 0x3f;

    if (charsNeeded > 1) {
        if (input[5] != '%')
            return false;

        // second continuation character
        decoded = (decodeNibble(input[6]) << 4) | decodeNibble(input[7]);
        if ((decoded & 0xc0) != 0x80)
            return false;
        uc <<= 6;
        uc |= decoded & 0x3f;

        if (charsNeeded > 2) {
            if (input[8] != '%')
                return false;

            // third continuation character
            decoded = (decodeNibble(input[9]) << 4) | decodeNibble(input[10]);
            if ((decoded & 0xc0) != 0x80)
                return false;
            uc <<= 6;
            uc |= decoded & 0x3f;
        }
    }

    // we've decoded something; safety-check it
    if (uc < min_uc)
        return false;
    if (isUnicodeNonCharacter(uc) || (uc >= 0xD800 && uc <= 0xDFFF) || uc >= 0x110000)
        return false;

    // detach if necessary
    if (!output) {
        // create enough space if the rest of the string needed to be percent-encoded
        int charsProcessed = input - reinterpret_cast<const ushort *>(result.constData()) - 1;
        int charsRemaining = end - input - 2 - 3*charsNeeded;
        int newSize = result.size() + 2 * charsRemaining;
        result.resize(newSize);

        // set the output variable
        output = reinterpret_cast<ushort *>(result.data()) + charsProcessed;
    }

    if (!QChar::requiresSurrogates(uc)) {
        // UTF-8 decoded and no surrogates are required
        *output++ = uc;
    } else {
        // UTF-8 decoded to something that requires a surrogate pair
        *output++ = QChar::highSurrogate(uc);
        *output++ = QChar::lowSurrogate(uc);
    }
    input += charsNeeded * 3 + 2;
    return true;
}

static void unicodeToEncodedUtf8(QString &result, ushort *&output, const ushort *&input, const ushort *end, ushort decoded)
{
    uint uc = decoded;
    if (QChar::isHighSurrogate(uc)) {
        if (QChar::isLowSurrogate(*input))
            uc = QChar::surrogateToUcs4(uc, *input);
    }

    // note: we will encode bad UTF-16 to UTF-8
    // but they don't get decoded back

    // calculate the utf8 length
    int utf8len = uc >= 0x10000 ? 4 : uc >= 0x800 ? 3 : 2;

    // detach
    if (!output) {
        // create enough space if the rest of the string needed to be percent-encoded
        int charsProcessed = input - reinterpret_cast<const ushort *>(result.constData()) - 1;
        int charsRemaining = end - input;
        int newSize = result.size() + 2 * charsRemaining - 1 + 3*utf8len;
        result.resize(newSize);

        // set the output variable
        output = reinterpret_cast<ushort *>(result.data()) + charsProcessed;
    } else {
        // verify that there's enough space or expand
        int charsRemaining = end - input;
        int pos = output - reinterpret_cast<const ushort *>(result.constData());
        int spaceRemaining = result.size() - pos;
        if (spaceRemaining < 3*charsRemaining + 3*utf8len) {
            // must resize
            result.resize(result.size() + 3*utf8len);
            output = reinterpret_cast<ushort *>(result.data()) + pos;
        }
    }

    if (QChar::requiresSurrogates(uc))
        ++input;

    // write the sequence
    if (uc < 0x800) {
        // first of two bytes
        uchar c = 0xc0 | uchar(uc >> 6);
        *output++ = '%';
        *output++ = encodeNibble(c >> 4);
        *output++ = encodeNibble(c & 0xf);
    } else {
        uchar c;
        if (uc > 0xFFFF) {
            // first two of four bytes
            c = 0xf0 | uchar(uc >> 18);
            *output++ = '%';
            *output++ = 'F';
            *output++ = encodeNibble(c & 0xf);

            // continuation byte
            c = 0x80 | (uchar(uc >> 12) & 0x3f);
            *output++ = '%';
            *output++ = encodeNibble(c >> 4);
            *output++ = encodeNibble(c & 0xf);
        } else {
            // first of three bytes
            c = 0xe0 | uchar(uc >> 12);
            *output++ = '%';
            *output++ = 'E';
            *output++ = encodeNibble(c & 0xf);
        }

        // continuation byte
        c = 0x80 | (uchar(uc >> 6) & 0x3f);
        *output++ = '%';
        *output++ = encodeNibble(c >> 4);
        *output++ = encodeNibble(c & 0xf);
    }

    // continuation byte
    uchar c = 0x80 | (uc & 0x3f);
    *output++ = '%';
    *output++ = encodeNibble(c >> 4);
    *output++ = encodeNibble(c & 0xf);
}

Q_AUTOTEST_EXPORT QString
qt_urlRecode(const QString &component, QUrl::ComponentFormattingOptions encoding,
             const uchar *tableModifications)
{
    uchar actionTable[sizeof defaultActionTable];
    memcpy(actionTable, defaultActionTable, sizeof actionTable);
    if (encoding & QUrl::DecodeSpaces)
        actionTable[0] = DecodeCharacter; // decode

    if (tableModifications) {
        for (const ushort *p = tableModifications; *p; ++p)
            actionTable[uchar(*p) - ' '] = *p >> 8;
    }

    QString result = component;
    const ushort *input = reinterpret_cast<const ushort *>(component.constData());
    const ushort * const end = input + component.length();
    ushort *output = 0;

    while (input != end) {
        register ushort c = *input++;
        register ushort decoded;
        if (c == '%') {
            // our input is always valid, so there are two hex characters for us to read here
            decoded = (decodeNibble(input[0]) << 4) | decodeNibble(input[1]);
        } else {
            decoded = c;
        }

        EncodingAction action;
        if (decoded < 0x20) {
            // always encode control characters
            action = EncodeCharacter;
        } else if (decoded < 0x80) {
            // use the table
            action = EncodingAction(actionTable[decoded - ' ']);
        } else {
            // non-ASCII
            bool decodeUnicode = encoding & QUrl::DecodeUnicode;

            // should we leave it like this?
            if ((c != '%' && decodeUnicode) || (c == '%' && !decodeUnicode)) {
                action = LeaveCharacter;
            } else if (decodeUnicode) {
                // c == '%': decode the UTF-8 sequence
                if (encodedUtf8ToUcs4(result, output, input, end, decoded))
                    continue;
                action = LeaveCharacter;
            } else {
                // c != '%': encode the UTF-8 sequence
                unicodeToEncodedUtf8(result, output, input, end, decoded);
                continue;
            }
        }

        // there are six possibilities:
        //  current \ action  | DecodeCharacter | LeaveCharacter | EncodeCharacter
        //      decoded       |    1:leave      |    2:leave     |    3:encode
        //      encoded       |    4:decode     |    5:leave     |    6:leave

        if (c != '%' && (action == LeaveCharacter || action == DecodeCharacter)) {
            // cases 1 and 2: it's decoded and we're leaving it as is
            // there's always enough memory allocated for a single character
            if (output)
                *output++ = c;
        } else if (c == '%' && (action == LeaveCharacter || action == EncodeCharacter)) {
            // cases 5 and 6: it's encoded and we're leaving it as it is
            // except we're pedantic and we'll uppercase the hex
            if (output || !isUpperHex(input[0]) || !isUpperHex(input[1])) {
                ensureDetached(result, output, input, end);
                *output++ = '%';
                *output++ = toUpperHex(*input++);
                *output++ = toUpperHex(*input++);
            }
        } else if (c == '%' && action == DecodeCharacter) {
            // case 4: we need to decode
            ensureDetached(result, output, input, end);
            *output++ = decoded;
            input += 2;
        } else {
            // must be case 3: we need to encode
            ensureDetached(result, output, input, end);
            *output++ = '%';
            *output++ = encodeNibble(c >> 4);
            *output++ = encodeNibble(c & 0xf);
        }
    }

    if (output)
        result.truncate(output - reinterpret_cast<const ushort *>(result.constData()));
    return result;
}

Q_AUTOTEST_EXPORT QString
qt_tolerantParsePercentEncoding(const QString &url)
{
    // are there any '%'
    int firstPercent = url.indexOf(QLatin1Char('%'));
    if (firstPercent == -1) {
        // none found, the string is fine
        return url;
    }

    // are there any invalid percents?
    int nextPercent = firstPercent;
    int percentCount = 0;

    {
        int len = url.length();
        bool ok = true;
        do {
            ++percentCount;
            if (nextPercent + 2 >= len ||
                    !isHex(url.at(nextPercent + 1).unicode()) ||
                    !isHex(url.at(nextPercent + 2).unicode())) {
                ok = false;
            }

            nextPercent = url.indexOf(QLatin1Char('%'), nextPercent + 1);
        } while (nextPercent != -1);

        if (ok)
            return url;
    }

    // we've found at least one invalid percent
    // that means all of them are invalid
    QString corrected(url.size() + percentCount * 2, Qt::Uninitialized);
    ushort *output = reinterpret_cast<ushort *>(corrected.data());
    const ushort *input = reinterpret_cast<const ushort *>(url.constData());
    for (int i = 0; i <= firstPercent; ++i)
        output[i] = input[i];

    const ushort *const end = input + url.length();
    output += firstPercent + 1;
    input += firstPercent + 1;

    // we've copied up to the first percent
    // correct this one and all others
    *output++ = '2';
    *output++ = '5';
    while (input != end) {
        // copy verbatim until the next percent, inclusive
        *output++ = *input;
        if (*input == '%') {
            *output++ = '2';
            *output++ = '5';
        }
        ++input;
    }
    return corrected;
}

QT_END_NAMESPACE
