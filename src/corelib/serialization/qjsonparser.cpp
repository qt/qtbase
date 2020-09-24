/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif
#include <qdebug.h>
#include "qjsonparser_p.h"
#include "qjson_p.h"
#include "private/qutfcodec_p.h"
#include "private/qcborvalue_p.h"
#include "private/qnumeric_p.h"

//#define PARSER_DEBUG
#ifdef PARSER_DEBUG
static int indent = 0;
#define BEGIN qDebug() << QByteArray(4*indent++, ' ').constData() << "pos=" << current
#define END --indent
#define DEBUG qDebug() << QByteArray(4*indent, ' ').constData()
#else
#define BEGIN if (1) ; else qDebug()
#define END do {} while (0)
#define DEBUG if (1) ; else qDebug()
#endif

static const int nestingLimit = 1024;

QT_BEGIN_NAMESPACE

// error strings for the JSON parser
#define JSONERR_OK          QT_TRANSLATE_NOOP("QJsonParseError", "no error occurred")
#define JSONERR_UNTERM_OBJ  QT_TRANSLATE_NOOP("QJsonParseError", "unterminated object")
#define JSONERR_MISS_NSEP   QT_TRANSLATE_NOOP("QJsonParseError", "missing name separator")
#define JSONERR_UNTERM_AR   QT_TRANSLATE_NOOP("QJsonParseError", "unterminated array")
#define JSONERR_MISS_VSEP   QT_TRANSLATE_NOOP("QJsonParseError", "missing value separator")
#define JSONERR_ILLEGAL_VAL QT_TRANSLATE_NOOP("QJsonParseError", "illegal value")
#define JSONERR_END_OF_NUM  QT_TRANSLATE_NOOP("QJsonParseError", "invalid termination by number")
#define JSONERR_ILLEGAL_NUM QT_TRANSLATE_NOOP("QJsonParseError", "illegal number")
#define JSONERR_STR_ESC_SEQ QT_TRANSLATE_NOOP("QJsonParseError", "invalid escape sequence")
#define JSONERR_STR_UTF8    QT_TRANSLATE_NOOP("QJsonParseError", "invalid UTF8 string")
#define JSONERR_UTERM_STR   QT_TRANSLATE_NOOP("QJsonParseError", "unterminated string")
#define JSONERR_MISS_OBJ    QT_TRANSLATE_NOOP("QJsonParseError", "object is missing after a comma")
#define JSONERR_DEEP_NEST   QT_TRANSLATE_NOOP("QJsonParseError", "too deeply nested document")
#define JSONERR_DOC_LARGE   QT_TRANSLATE_NOOP("QJsonParseError", "too large document")
#define JSONERR_GARBAGEEND  QT_TRANSLATE_NOOP("QJsonParseError", "garbage at the end of the document")

/*!
    \class QJsonParseError
    \inmodule QtCore
    \ingroup json
    \ingroup shared
    \reentrant
    \since 5.0

    \brief The QJsonParseError class is used to report errors during JSON parsing.

    \sa {JSON Support in Qt}, {JSON Save Game Example}
*/

/*!
    \enum QJsonParseError::ParseError

    This enum describes the type of error that occurred during the parsing of a JSON document.

    \value NoError                  No error occurred
    \value UnterminatedObject       An object is not correctly terminated with a closing curly bracket
    \value MissingNameSeparator     A comma separating different items is missing
    \value UnterminatedArray        The array is not correctly terminated with a closing square bracket
    \value MissingValueSeparator    A colon separating keys from values inside objects is missing
    \value IllegalValue             The value is illegal
    \value TerminationByNumber      The input stream ended while parsing a number
    \value IllegalNumber            The number is not well formed
    \value IllegalEscapeSequence    An illegal escape sequence occurred in the input
    \value IllegalUTF8String        An illegal UTF8 sequence occurred in the input
    \value UnterminatedString       A string wasn't terminated with a quote
    \value MissingObject            An object was expected but couldn't be found
    \value DeepNesting              The JSON document is too deeply nested for the parser to parse it
    \value DocumentTooLarge         The JSON document is too large for the parser to parse it
    \value GarbageAtEnd             The parsed document contains additional garbage characters at the end

*/

/*!
    \variable QJsonParseError::error

    Contains the type of the parse error. Is equal to QJsonParseError::NoError if the document
    was parsed correctly.

    \sa ParseError, errorString()
*/


/*!
    \variable QJsonParseError::offset

    Contains the offset in the input string where the parse error occurred.

    \sa error, errorString()
*/

/*!
  Returns the human-readable message appropriate to the reported JSON parsing error.

  \sa error
 */
QString QJsonParseError::errorString() const
{
    const char *sz = "";
    switch (error) {
    case NoError:
        sz = JSONERR_OK;
        break;
    case UnterminatedObject:
        sz = JSONERR_UNTERM_OBJ;
        break;
    case MissingNameSeparator:
        sz = JSONERR_MISS_NSEP;
        break;
    case UnterminatedArray:
        sz = JSONERR_UNTERM_AR;
        break;
    case MissingValueSeparator:
        sz = JSONERR_MISS_VSEP;
        break;
    case IllegalValue:
        sz = JSONERR_ILLEGAL_VAL;
        break;
    case TerminationByNumber:
        sz = JSONERR_END_OF_NUM;
        break;
    case IllegalNumber:
        sz = JSONERR_ILLEGAL_NUM;
        break;
    case IllegalEscapeSequence:
        sz = JSONERR_STR_ESC_SEQ;
        break;
    case IllegalUTF8String:
        sz = JSONERR_STR_UTF8;
        break;
    case UnterminatedString:
        sz = JSONERR_UTERM_STR;
        break;
    case MissingObject:
        sz = JSONERR_MISS_OBJ;
        break;
    case DeepNesting:
        sz = JSONERR_DEEP_NEST;
        break;
    case DocumentTooLarge:
        sz = JSONERR_DOC_LARGE;
        break;
    case GarbageAtEnd:
        sz = JSONERR_GARBAGEEND;
        break;
    }
#ifndef QT_BOOTSTRAPPED
    return QCoreApplication::translate("QJsonParseError", sz);
#else
    return QLatin1String(sz);
#endif
}

using namespace QJsonPrivate;

class StashedContainer
{
    Q_DISABLE_COPY_MOVE(StashedContainer)
public:
    StashedContainer(QExplicitlySharedDataPointer<QCborContainerPrivate> *container,
                     QCborValue::Type type)
        : type(type), stashed(std::move(*container)), current(container)
    {
    }

    ~StashedContainer()
    {
        stashed->append(QCborContainerPrivate::makeValue(type, -1, current->take(),
                                                         QCborContainerPrivate::MoveContainer));
        *current = std::move(stashed);
    }

private:
    QCborValue::Type type;
    QExplicitlySharedDataPointer<QCborContainerPrivate> stashed;
    QExplicitlySharedDataPointer<QCborContainerPrivate> *current;
};

Parser::Parser(const char *json, int length)
    : head(json), json(json)
    , nestingLevel(0)
    , lastError(QJsonParseError::NoError)
{
    end = json + length;
}



/*

begin-array     = ws %x5B ws  ; [ left square bracket

begin-object    = ws %x7B ws  ; { left curly bracket

end-array       = ws %x5D ws  ; ] right square bracket

end-object      = ws %x7D ws  ; } right curly bracket

name-separator  = ws %x3A ws  ; : colon

value-separator = ws %x2C ws  ; , comma

Insignificant whitespace is allowed before or after any of the six
structural characters.

ws = *(
          %x20 /              ; Space
          %x09 /              ; Horizontal tab
          %x0A /              ; Line feed or New line
          %x0D                ; Carriage return
      )

*/

enum {
    Space = 0x20,
    Tab = 0x09,
    LineFeed = 0x0a,
    Return = 0x0d,
    BeginArray = 0x5b,
    BeginObject = 0x7b,
    EndArray = 0x5d,
    EndObject = 0x7d,
    NameSeparator = 0x3a,
    ValueSeparator = 0x2c,
    Quote = 0x22
};

void Parser::eatBOM()
{
    // eat UTF-8 byte order mark
    uchar utf8bom[3] = { 0xef, 0xbb, 0xbf };
    if (end - json > 3 &&
        (uchar)json[0] == utf8bom[0] &&
        (uchar)json[1] == utf8bom[1] &&
        (uchar)json[2] == utf8bom[2])
        json += 3;
}

bool Parser::eatSpace()
{
    while (json < end) {
        if (*json > Space)
            break;
        if (*json != Space &&
            *json != Tab &&
            *json != LineFeed &&
            *json != Return)
            break;
        ++json;
    }
    return (json < end);
}

char Parser::nextToken()
{
    if (!eatSpace())
        return 0;
    char token = *json++;
    switch (token) {
    case BeginArray:
    case BeginObject:
    case NameSeparator:
    case ValueSeparator:
    case EndArray:
    case EndObject:
    case Quote:
        break;
    default:
        token = 0;
        break;
    }
    return token;
}

/*
    JSON-text = object / array
*/
QCborValue Parser::parse(QJsonParseError *error)
{
#ifdef PARSER_DEBUG
    indent = 0;
    qDebug(">>>>> parser begin");
#endif
    eatBOM();
    char token = nextToken();

    QCborValue data;

    DEBUG << Qt::hex << (uint)token;
    if (token == BeginArray) {
        container = new QCborContainerPrivate;
        if (!parseArray())
            goto error;
        data = QCborContainerPrivate::makeValue(QCborValue::Array, -1, container.take(),
                                                QCborContainerPrivate::MoveContainer);
    } else if (token == BeginObject) {
        container = new QCborContainerPrivate;
        if (!parseObject())
            goto error;
        data = QCborContainerPrivate::makeValue(QCborValue::Map, -1, container.take(),
                                                QCborContainerPrivate::MoveContainer);
    } else {
        lastError = QJsonParseError::IllegalValue;
        goto error;
    }

    eatSpace();
    if (json < end) {
        lastError = QJsonParseError::GarbageAtEnd;
        goto error;
    }

    END;
    {
        if (error) {
            error->offset = 0;
            error->error = QJsonParseError::NoError;
        }

        return data;
    }

error:
#ifdef PARSER_DEBUG
    qDebug(">>>>> parser error");
#endif
    container.reset();
    if (error) {
        error->offset = json - head;
        error->error  = lastError;
    }
    return QCborValue();
}

static void sortContainer(QCborContainerPrivate *container)
{
    using Forward = QJsonPrivate::KeyIterator;
    using Reverse = std::reverse_iterator<Forward>;
    using Value = Forward::value_type;

    auto compare = [container](const Value &a, const Value &b)
    {
        const auto &aKey = a.key();
        const auto &bKey = b.key();

        Q_ASSERT(aKey.flags & QtCbor::Element::HasByteData);
        Q_ASSERT(bKey.flags & QtCbor::Element::HasByteData);

        const QtCbor::ByteData *aData = container->byteData(aKey);
        const QtCbor::ByteData *bData = container->byteData(bKey);

        if (!aData)
            return bData ? -1 : 0;
        if (!bData)
            return 1;

        // US-ASCII (StringIsAscii flag) is just a special case of UTF-8
        // string, so we can safely ignore the flag.

        if (aKey.flags & QtCbor::Element::StringIsUtf16) {
            if (bKey.flags & QtCbor::Element::StringIsUtf16)
                return QtPrivate::compareStrings(aData->asStringView(), bData->asStringView());

            return -QCborContainerPrivate::compareUtf8(bData, aData->asStringView());
        } else {
            if (bKey.flags & QtCbor::Element::StringIsUtf16)
                return QCborContainerPrivate::compareUtf8(aData, bData->asStringView());

            // We're missing an explicit UTF-8 to UTF-8 comparison in Qt, but
            // UTF-8 to UTF-8 comparison retains simple byte ordering, so we'll
            // abuse the Latin-1 comparison function.
            return QtPrivate::compareStrings(aData->asLatin1(), bData->asLatin1());
        }
    };

    std::sort(Forward(container->elements.begin()), Forward(container->elements.end()),
              [&compare](const Value &a, const Value &b) { return compare(a, b) < 0; });

    // We need to retain the _last_ value for any duplicate keys. Therefore the reverse dance here.
    auto it = std::unique(Reverse(container->elements.end()), Reverse(container->elements.begin()),
                          [&compare](const Value &a, const Value &b) {
        return compare(a, b) == 0;
    }).base().elementsIterator();

    // The erase from beginning is expensive but hopefully rare.
    container->elements.erase(container->elements.begin(), it);
}


/*
    object = begin-object [ member *( value-separator member ) ]
    end-object
*/

bool Parser::parseObject()
{
    if (++nestingLevel > nestingLimit) {
        lastError = QJsonParseError::DeepNesting;
        return false;
    }

    BEGIN << "parseObject" << json;

    char token = nextToken();
    while (token == Quote) {
        if (!container)
            container = new QCborContainerPrivate;
        if (!parseMember())
            return false;
        token = nextToken();
        if (token != ValueSeparator)
            break;
        token = nextToken();
        if (token == EndObject) {
            lastError = QJsonParseError::MissingObject;
            return false;
        }
    }

    DEBUG << "end token=" << token;
    if (token != EndObject) {
        lastError = QJsonParseError::UnterminatedObject;
        return false;
    }

    END;

    --nestingLevel;

    if (container)
        sortContainer(container.data());
    return true;
}

/*
    member = string name-separator value
*/
bool Parser::parseMember()
{
    BEGIN << "parseMember";

    if (!parseString())
        return false;
    char token = nextToken();
    if (token != NameSeparator) {
        lastError = QJsonParseError::MissingNameSeparator;
        return false;
    }
    if (!eatSpace()) {
        lastError = QJsonParseError::UnterminatedObject;
        return false;
    }
    if (!parseValue())
        return false;

    END;
    return true;
}

/*
    array = begin-array [ value *( value-separator value ) ] end-array
*/
bool Parser::parseArray()
{
    BEGIN << "parseArray";

    if (++nestingLevel > nestingLimit) {
        lastError = QJsonParseError::DeepNesting;
        return false;
    }

    if (!eatSpace()) {
        lastError = QJsonParseError::UnterminatedArray;
        return false;
    }
    if (*json == EndArray) {
        nextToken();
    } else {
        while (1) {
            if (!eatSpace()) {
                lastError = QJsonParseError::UnterminatedArray;
                return false;
            }
            if (!container)
                container = new QCborContainerPrivate;
            if (!parseValue())
                return false;
            char token = nextToken();
            if (token == EndArray)
                break;
            else if (token != ValueSeparator) {
                if (!eatSpace())
                    lastError = QJsonParseError::UnterminatedArray;
                else
                    lastError = QJsonParseError::MissingValueSeparator;
                return false;
            }
        }
    }

    DEBUG << "size =" << (container ? container->elements.length() : 0);
    END;

    --nestingLevel;

    return true;
}

/*
value = false / null / true / object / array / number / string

*/

bool Parser::parseValue()
{
    BEGIN << "parse Value" << json;

    switch (*json++) {
    case 'n':
        if (end - json < 4) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'u' &&
            *json++ == 'l' &&
            *json++ == 'l') {
            container->append(QCborValue(QCborValue::Null));
            DEBUG << "value: null";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case 't':
        if (end - json < 4) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'r' &&
            *json++ == 'u' &&
            *json++ == 'e') {
            container->append(QCborValue(true));
            DEBUG << "value: true";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case 'f':
        if (end - json < 5) {
            lastError = QJsonParseError::IllegalValue;
            return false;
        }
        if (*json++ == 'a' &&
            *json++ == 'l' &&
            *json++ == 's' &&
            *json++ == 'e') {
            container->append(QCborValue(false));
            DEBUG << "value: false";
            END;
            return true;
        }
        lastError = QJsonParseError::IllegalValue;
        return false;
    case Quote: {
        if (!parseString())
            return false;
        DEBUG << "value: string";
        END;
        return true;
    }
    case BeginArray: {
        StashedContainer stashedContainer(&container, QCborValue::Array);
        if (!parseArray())
            return false;
        DEBUG << "value: array";
        END;
        return true;
    }
    case BeginObject: {
        StashedContainer stashedContainer(&container, QCborValue::Map);
        if (!parseObject())
            return false;
        DEBUG << "value: object";
        END;
        return true;
    }
    case ValueSeparator:
        // Essentially missing value, but after a colon, not after a comma
        // like the other MissingObject errors.
        lastError = QJsonParseError::IllegalValue;
        return false;
    case EndObject:
    case EndArray:
        lastError = QJsonParseError::MissingObject;
        return false;
    default:
        --json;
        if (!parseNumber())
            return false;
        DEBUG << "value: number";
        END;
    }

    return true;
}





/*
        number = [ minus ] int [ frac ] [ exp ]
        decimal-point = %x2E       ; .
        digit1-9 = %x31-39         ; 1-9
        e = %x65 / %x45            ; e E
        exp = e [ minus / plus ] 1*DIGIT
        frac = decimal-point 1*DIGIT
        int = zero / ( digit1-9 *DIGIT )
        minus = %x2D               ; -
        plus = %x2B                ; +
        zero = %x30                ; 0

*/

bool Parser::parseNumber()
{
    BEGIN << "parseNumber" << json;

    const char *start = json;
    bool isInt = true;

    // minus
    if (json < end && *json == '-')
        ++json;

    // int = zero / ( digit1-9 *DIGIT )
    if (json < end && *json == '0') {
        ++json;
    } else {
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    // frac = decimal-point 1*DIGIT
    if (json < end && *json == '.') {
        isInt = false;
        ++json;
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    // exp = e [ minus / plus ] 1*DIGIT
    if (json < end && (*json == 'e' || *json == 'E')) {
        isInt = false;
        ++json;
        if (json < end && (*json == '-' || *json == '+'))
            ++json;
        while (json < end && *json >= '0' && *json <= '9')
            ++json;
    }

    if (json >= end) {
        lastError = QJsonParseError::TerminationByNumber;
        return false;
    }

    const QByteArray number = QByteArray::fromRawData(start, json - start);
    DEBUG << "numberstring" << number;

    if (isInt) {
        bool ok;
        qlonglong n = number.toLongLong(&ok);
        if (ok) {
            container->append(QCborValue(n));
            END;
            return true;
        }
    }

    bool ok;
    double d = number.toDouble(&ok);

    if (!ok) {
        lastError = QJsonParseError::IllegalNumber;
        return false;
    }

    qint64 n;
    if (convertDoubleTo(d, &n))
        container->append(QCborValue(n));
    else
        container->append(QCborValue(d));

    END;
    return true;
}

/*

        string = quotation-mark *char quotation-mark

        char = unescaped /
               escape (
                   %x22 /          ; "    quotation mark  U+0022
                   %x5C /          ; \    reverse solidus U+005C
                   %x2F /          ; /    solidus         U+002F
                   %x62 /          ; b    backspace       U+0008
                   %x66 /          ; f    form feed       U+000C
                   %x6E /          ; n    line feed       U+000A
                   %x72 /          ; r    carriage return U+000D
                   %x74 /          ; t    tab             U+0009
                   %x75 4HEXDIG )  ; uXXXX                U+XXXX

        escape = %x5C              ; \

        quotation-mark = %x22      ; "

        unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
 */
static inline bool addHexDigit(char digit, uint *result)
{
    *result <<= 4;
    if (digit >= '0' && digit <= '9')
        *result |= (digit - '0');
    else if (digit >= 'a' && digit <= 'f')
        *result |= (digit - 'a') + 10;
    else if (digit >= 'A' && digit <= 'F')
        *result |= (digit - 'A') + 10;
    else
        return false;
    return true;
}

static inline bool scanEscapeSequence(const char *&json, const char *end, uint *ch)
{
    ++json;
    if (json >= end)
        return false;

    DEBUG << "scan escape" << (char)*json;
    uint escaped = *json++;
    switch (escaped) {
    case '"':
        *ch = '"'; break;
    case '\\':
        *ch = '\\'; break;
    case '/':
        *ch = '/'; break;
    case 'b':
        *ch = 0x8; break;
    case 'f':
        *ch = 0xc; break;
    case 'n':
        *ch = 0xa; break;
    case 'r':
        *ch = 0xd; break;
    case 't':
        *ch = 0x9; break;
    case 'u': {
        *ch = 0;
        if (json > end - 4)
            return false;
        for (int i = 0; i < 4; ++i) {
            if (!addHexDigit(*json, ch))
                return false;
            ++json;
        }
        return true;
    }
    default:
        // this is not as strict as one could be, but allows for more Json files
        // to be parsed correctly.
        *ch = escaped;
        return true;
    }
    return true;
}

static inline bool scanUtf8Char(const char *&json, const char *end, uint *result)
{
    const auto *usrc = reinterpret_cast<const uchar *>(json);
    const auto *uend = reinterpret_cast<const uchar *>(end);
    const uchar b = *usrc++;
    int res = QUtf8Functions::fromUtf8<QUtf8BaseTraits>(b, result, usrc, uend);
    if (res < 0)
        return false;

    json = reinterpret_cast<const char *>(usrc);
    return true;
}

bool Parser::parseString()
{
    const char *start = json;

    // try to parse a utf-8 string without escape sequences, and note whether it's 7bit ASCII.

    BEGIN << "parse string" << json;
    bool isUtf8 = true;
    bool isAscii = true;
    while (json < end) {
        uint ch = 0;
        if (*json == '"')
            break;
        if (*json == '\\') {
            isAscii = false;
            // If we find escape sequences, we store UTF-16 as there are some
            // escape sequences which are hard to represent in UTF-8.
            // (plain "\\ud800" for example)
            isUtf8 = false;
            break;
        }
        if (!scanUtf8Char(json, end, &ch)) {
            lastError = QJsonParseError::IllegalUTF8String;
            return false;
        }
        if (ch > 0x7f)
            isAscii = false;
        DEBUG << "  " << ch << char(ch);
    }
    ++json;
    DEBUG << "end of string";
    if (json >= end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    // no escape sequences, we are done
    if (isUtf8) {
        container->appendByteData(start, json - start - 1, QCborValue::String,
                                  isAscii ? QtCbor::Element::StringIsAscii
                                          : QtCbor::Element::ValueFlags {});
        END;
        return true;
    }

    DEBUG << "has escape sequences";

    json = start;

    QString ucs4;
    while (json < end) {
        uint ch = 0;
        if (*json == '"')
            break;
        else if (*json == '\\') {
            if (!scanEscapeSequence(json, end, &ch)) {
                lastError = QJsonParseError::IllegalEscapeSequence;
                return false;
            }
        } else {
            if (!scanUtf8Char(json, end, &ch)) {
                lastError = QJsonParseError::IllegalUTF8String;
                return false;
            }
        }
        if (QChar::requiresSurrogates(ch)) {
            ucs4.append(QChar::highSurrogate(ch));
            ucs4.append(QChar::lowSurrogate(ch));
        } else {
            ucs4.append(QChar(ushort(ch)));
        }
    }
    ++json;

    if (json >= end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    container->appendByteData(reinterpret_cast<const char *>(ucs4.utf16()), ucs4.size() * 2,
                              QCborValue::String, QtCbor::Element::StringIsUtf16);
    END;
    return true;
}

QT_END_NAMESPACE
