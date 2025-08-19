// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2021 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#ifndef QT_BOOTSTRAPPED
#include <qcoreapplication.h>
#endif
#include <qdebug.h>
#include "qjsonparser_p.h"
#include "qjson_p.h"
#include "private/qstringconverter_p.h"
#include "private/qcborvalue_p.h"
#include "private/qnumeric_p.h"
#include <private/qtools_p.h>

static const int nestingLimit = 1024;

QT_BEGIN_NAMESPACE

using namespace QtMiscUtils;

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
    \ingroup qtserialization
    \reentrant
    \since 5.0

    \brief The QJsonParseError class is used to report errors during JSON parsing.

    \sa {JSON Support in Qt}, {Saving and Loading a Game}
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
    \value TerminationByNumber      The input stream ended while parsing a number (as of 6.9, this is no longer returned)
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

    Contains the byte offset in the UTF-8 byte array where the parse error occurred.

    \sa error, errorString(), QJsonDocument::fromJson()
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
    return QLatin1StringView(sz);
#endif
}

using namespace QJsonPrivate;

class StashedContainer
{
    Q_DISABLE_COPY_MOVE(StashedContainer)
public:
    StashedContainer(QExplicitlySharedDataPointer<QCborContainerPrivate> *container,
                     QCborValue::Type type)
        : type(type), stashed(std::move(*container))
    {
    }

    QCborValue intoValue(QExplicitlySharedDataPointer<QCborContainerPrivate> *parent)
    {
        std::swap(stashed, *parent);
        return QCborContainerPrivate::makeValue(type, -1, stashed.take(),
                                                QCborContainerPrivate::MoveContainer);
    }

private:
    QCborValue::Type type;
    QExplicitlySharedDataPointer<QCborContainerPrivate> stashed;
};

Parser::Parser(QUtf8StringView json)
    : head(json.data()), json(head), end(json.end())
    , nestingLevel(0)
    , lastError(QJsonParseError::NoError)
{
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
    eatBOM();

    char token;
    QCborValue value;

    if (!eatSpace()) {
        lastError = QJsonParseError::IllegalValue;
        goto error;
    }

    token = *json;
    if (token == Quote) {
        container = new QCborContainerPrivate;
        json++;
        if (!parseString())
            goto error;
        value = QCborContainerPrivate::makeValue(QCborValue::String, 0, container.take(),
                                                 QCborContainerPrivate::MoveContainer);
    } else {
        value = parseValue();
        if (value.isUndefined())
            goto error;
    }

    eatSpace();
    if (json < end) {
        lastError = QJsonParseError::GarbageAtEnd;
        goto error;
    }

    {
        if (error) {
            error->offset = 0;
            error->error = QJsonParseError::NoError;
        }

        return value;
    }

error:
    container.reset();
    if (error) {
        error->offset = json - head;
        error->error  = lastError;
    }
    return QCborValue();
}

// We need to retain the _last_ value for any duplicate keys and we need to deref containers.
// Therefore the manual implementation of std::unique().
template<typename Iterator, typename Compare, typename Assign>
static Iterator customAssigningUniqueLast(Iterator first, Iterator last,
                                          Compare compare, Assign assign)
{
    first = std::adjacent_find(first, last, compare);
    if (first == last)
        return last;

    // After adjacent_find, we know that *first and *(first+1) compare equal,
    // and that first+1 != last.
    Iterator result = first++;
    Q_ASSERT(compare(*result, *first));
    assign(*result, *first);
    Q_ASSERT(first != last);

    while (++first != last) {
        if (!compare(*result, *first))
            ++result;

        // Due to adjacent_find above, we know that we've at least eliminated one element.
        // Therefore we have to move each further element across the gap.
        Q_ASSERT(result != first);

        // We have to overwrite each element we want to eliminate, to deref() the container.
        // Therefore we don't try to optimize the number of assignments here.
        assign(*result, *first);
    }

    return ++result;
}

static void sortContainer(QCborContainerPrivate *container)
{
    using Forward = QJsonPrivate::KeyIterator;
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

            return QtPrivate::compareStrings(aData->asUtf8StringView(), bData->asUtf8StringView());
        }
    };

    // The elements' containers are owned by the outer container, not by the elements themselves.
    auto move = [](Forward::reference target, Forward::reference source)
    {
        QtCbor::Element &targetValue = target.value();

        // If the target has a container, deref it before overwriting, so that we don't leak.
        if (targetValue.flags & QtCbor::Element::IsContainer)
            targetValue.container->deref();

        // Do not move, so that we can clear the value afterwards.
        target = source;

        // Clear the source value, so that we don't store the same container twice.
        source.value() = QtCbor::Element();
    };

    std::stable_sort(
                Forward(container->elements.begin()), Forward(container->elements.end()),
                [&compare](const Value &a, const Value &b) { return compare(a, b) < 0; });

    Forward result = customAssigningUniqueLast(
                Forward(container->elements.begin()),  Forward(container->elements.end()),
                [&compare](const Value &a, const Value &b) { return compare(a, b) == 0; }, move);

    container->elements.erase(result.elementsIterator(), container->elements.end());
}

bool Parser::parseValueIntoContainer()
{
    QCborValue value = parseValue();
    switch (value.type()) {
    case QCborValue::Undefined:
        return false; // error while parsing
    case QCborValue::String:
        break; // strings were already added
    default:
        container->append(std::move(value));
    }

    return true;
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

    if (token != EndObject) {
        lastError = QJsonParseError::UnterminatedObject;
        return false;
    }

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

    return parseValueIntoContainer();
}

/*
    array = begin-array [ value *( value-separator value ) ] end-array
*/
bool Parser::parseArray()
{
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

            if (!parseValueIntoContainer())
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

    --nestingLevel;

    return true;
}

/*
value = false / null / true / object / array / number / string

*/

QCborValue Parser::parseValue()
{
    switch (*json++) {
    case 'n':
        if (end - json < 3) {
            lastError = QJsonParseError::IllegalValue;
            return QCborValue();
        }
        if (*json++ == 'u' &&
            *json++ == 'l' &&
            *json++ == 'l') {
            return QCborValue(QCborValue::Null);
        }
        lastError = QJsonParseError::IllegalValue;
        return QCborValue();
    case 't':
        if (end - json < 3) {
            lastError = QJsonParseError::IllegalValue;
            return QCborValue();
        }
        if (*json++ == 'r' &&
            *json++ == 'u' &&
            *json++ == 'e') {
            return QCborValue(true);
        }
        lastError = QJsonParseError::IllegalValue;
        return QCborValue();
    case 'f':
        if (end - json < 4) {
            lastError = QJsonParseError::IllegalValue;
            return QCborValue();
        }
        if (*json++ == 'a' &&
            *json++ == 'l' &&
            *json++ == 's' &&
            *json++ == 'e') {
            return QCborValue(false);
        }
        lastError = QJsonParseError::IllegalValue;
        return QCborValue();
    case Quote: {
        if (parseString())
            // strings are already added to the container
            // callers must check for this type
            return QCborValue(QCborValue::String);

        return QCborValue();
    }
    case BeginArray: {
        StashedContainer stashedContainer(&container, QCborValue::Array);
        if (parseArray())
            return stashedContainer.intoValue(&container);

        return QCborValue();
    }
    case BeginObject: {
        StashedContainer stashedContainer(&container, QCborValue::Map);
        if (parseObject())
            return stashedContainer.intoValue(&container);

        return QCborValue();
    }
    case ValueSeparator:
        // Essentially missing value, but after a colon, not after a comma
        // like the other MissingObject errors.
        lastError = QJsonParseError::IllegalValue;
        return QCborValue();
    case EndObject:
    case EndArray:
        lastError = QJsonParseError::MissingObject;
        return QCborValue();
    default:
        --json;
        return parseNumber();
    }
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

QCborValue Parser::parseNumber()
{
    const char *start = json;
    bool isInt = true;

    // minus
    if (json < end && *json == '-')
        ++json;

    // int = zero / ( digit1-9 *DIGIT )
    if (json < end && *json == '0') {
        ++json;
    } else {
        while (json < end && isAsciiDigit(*json))
            ++json;
    }

    // frac = decimal-point 1*DIGIT
    if (json < end && *json == '.') {
        ++json;
        while (json < end && isAsciiDigit(*json)) {
            isInt = isInt && *json == '0';
            ++json;
        }
    }

    // exp = e [ minus / plus ] 1*DIGIT
    if (json < end && (*json == 'e' || *json == 'E')) {
        isInt = false;
        ++json;
        if (json < end && (*json == '-' || *json == '+'))
            ++json;
        while (json < end && isAsciiDigit(*json))
            ++json;
    }

    const QByteArray number = QByteArray::fromRawData(start, json - start);

    if (isInt) {
        bool ok;
        qlonglong n = number.toLongLong(&ok);
        if (ok) {
            return QCborValue(n);
        }
    }

    bool ok;
    double d = number.toDouble(&ok);

    if (!ok) {
        lastError = QJsonParseError::IllegalNumber;
        return QCborValue();
    }

    qint64 n;
    if (convertDoubleTo(d, &n))
        return QCborValue(n);
    return QCborValue(d);
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
static inline bool addHexDigit(char digit, char32_t *result)
{
    *result <<= 4;
    const int h = fromHex(digit);
    if (h != -1) {
        *result |= h;
        return true;
    }

    return false;
}

static inline bool scanEscapeSequence(const char *&json, const char *end, char32_t *ch)
{
    ++json;
    if (json >= end)
        return false;

    uchar escaped = *json++;
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

static inline bool scanUtf8Char(const char *&json, const char *end, char32_t *result)
{
    auto usrc = reinterpret_cast<const qchar8_t*>(json);
    const auto uend = reinterpret_cast<const qchar8_t*>(end);
    constexpr char32_t Invalid = ~U'\0';
    const char32_t ch = QUtf8Functions::nextUcs4FromUtf8(usrc, uend, Invalid);
    if (ch == Invalid)
        return false;
    *result = ch;
    json = reinterpret_cast<const char *>(usrc);
    return true;
}

bool Parser::parseString()
{
    const char *start = json;

    // try to parse a utf-8 string without escape sequences, and note whether it's 7bit ASCII.

    bool isUtf8 = true;
    bool isAscii = true;
    while (json < end) {
        char32_t ch = 0;
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
    }
    ++json;
    if (json > end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    // no escape sequences, we are done
    if (isUtf8) {
        if (isAscii)
            container->appendAsciiString(start, json - start - 1);
        else
            container->appendUtf8String(start, json - start - 1);
        return true;
    }

    json = start;

    QString ucs4;
    while (json < end) {
        char32_t ch = 0;
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
        ucs4.append(QChar::fromUcs4(ch));
    }
    ++json;

    if (json > end) {
        lastError = QJsonParseError::UnterminatedString;
        return false;
    }

    container->appendByteData(reinterpret_cast<const char *>(ucs4.constData()), ucs4.size() * 2,
                              QCborValue::String, QtCbor::Element::StringIsUtf16);
    return true;
}

QT_END_NAMESPACE
