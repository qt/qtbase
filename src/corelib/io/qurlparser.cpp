/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qurl_p.h"

QT_BEGIN_NAMESPACE

static bool QT_FASTCALL _HEXDIG(const char **ptr)
{
    char ch = **ptr;
    if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F')) {
        ++(*ptr);
        return true;
    }

    return false;
}

// pct-encoded = "%" HEXDIG HEXDIG
static bool QT_FASTCALL _pctEncoded(const char **ptr)
{
    const char *ptrBackup = *ptr;

    if (**ptr != '%')
        return false;
    ++(*ptr);

    if (!_HEXDIG(ptr)) {
        *ptr = ptrBackup;
        return false;
    }
    if (!_HEXDIG(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    return true;
}

#if 0
// gen-delims  = ":" / "/" / "?" / "#" / "[" / "]" / "@"
static bool QT_FASTCALL _genDelims(const char **ptr, char *c)
{
    char ch = **ptr;
    switch (ch) {
    case ':': case '/': case '?': case '#':
    case '[': case ']': case '@':
        *c = ch;
        ++(*ptr);
        return true;
    default:
        return false;
    }
}
#endif

// sub-delims  = "!" / "$" / "&" / "'" / "(" / ")"
//             / "*" / "+" / "," / ";" / "="
static bool QT_FASTCALL _subDelims(const char **ptr)
{
    char ch = **ptr;
    switch (ch) {
    case '!': case '$': case '&': case '\'':
    case '(': case ')': case '*': case '+':
    case ',': case ';': case '=':
        ++(*ptr);
        return true;
    default:
        return false;
    }
}

// unreserved  = ALPHA / DIGIT / "-" / "." / "_" / "~"
static bool QT_FASTCALL _unreserved(const char **ptr)
{
    char ch = **ptr;
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9')
        || ch == '-' || ch == '.' || ch == '_' || ch == '~') {
        ++(*ptr);
        return true;
    }
    return false;
}

// scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
static bool QT_FASTCALL _scheme(const char **ptr, QUrlParseData *parseData)
{
    bool first = true;
    bool isSchemeValid = true;

    parseData->scheme = *ptr;
    for (;;) {
        char ch = **ptr;
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
            ;
        } else if ((ch >= '0' && ch <= '9') || ch == '+' || ch == '-' || ch == '.') {
            if (first)
                isSchemeValid = false;
        } else {
            break;
        }

        ++(*ptr);
        first = false;
    }

    if (**ptr != ':') {
        isSchemeValid = true;
        *ptr = parseData->scheme;
    } else {
        parseData->schemeLength = *ptr - parseData->scheme;
        ++(*ptr); // skip ':'
    }

    return isSchemeValid;
}

// IPvFuture  = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
static bool QT_FASTCALL _IPvFuture(const char **ptr)
{
    if (**ptr != 'v')
        return false;

    const char *ptrBackup = *ptr;
    ++(*ptr);

    if (!_HEXDIG(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    while (_HEXDIG(ptr))
        ;

    if (**ptr != '.') {
        *ptr = ptrBackup;
        return false;
    }
    ++(*ptr);

    if (!_unreserved(ptr) && !_subDelims(ptr) && *((*ptr)++) != ':') {
        *ptr = ptrBackup;
        return false;
    }


    while (_unreserved(ptr) || _subDelims(ptr) || *((*ptr)++) == ':')
        ;

    return true;
}

// h16         = 1*4HEXDIG
//             ; 16 bits of address represented in hexadecimal
static bool QT_FASTCALL _h16(const char **ptr)
{
    int i = 0;
    for (; i < 4; ++i) {
        if (!_HEXDIG(ptr))
            break;
    }
    return (i != 0);
}

// dec-octet   = DIGIT                 ; 0-9
//             / %x31-39 DIGIT         ; 10-99
//             / "1" 2DIGIT            ; 100-199
//             / "2" %x30-34 DIGIT     ; 200-249
//             / "25" %x30-35          ; 250-255
static bool QT_FASTCALL _decOctet(const char **ptr)
{
    const char *ptrBackup = *ptr;
    char c1 = **ptr;

    if (c1 < '0' || c1 > '9')
        return false;

    ++(*ptr);

    if (c1 == '0')
        return true;

    char c2 = **ptr;

    if (c2 < '0' || c2 > '9')
        return true;

    ++(*ptr);

    char c3 = **ptr;
    if (c3 < '0' || c3 > '9')
        return true;

    // If there is a three digit number larger than 255, reject the
    // whole token.
    if (c1 >= '2' && c2 >= '5' && c3 > '5') {
        *ptr = ptrBackup;
        return false;
    }

    ++(*ptr);

    return true;
}

// IPv4address = dec-octet "." dec-octet "." dec-octet "." dec-octet
static bool QT_FASTCALL _IPv4Address(const char **ptr)
{
    const char *ptrBackup = *ptr;

    if (!_decOctet(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    for (int i = 0; i < 3; ++i) {
        char ch = *((*ptr)++);
        if (ch != '.') {
            *ptr = ptrBackup;
            return false;
        }

        if (!_decOctet(ptr)) {
            *ptr = ptrBackup;
            return false;
        }
    }

    return true;
}

// ls32        = ( h16 ":" h16 ) / IPv4address
//             ; least-significant 32 bits of address
static bool QT_FASTCALL _ls32(const char **ptr)
{
    const char *ptrBackup = *ptr;
    if (_h16(ptr) && *((*ptr)++) == ':' && _h16(ptr))
        return true;

    *ptr = ptrBackup;
    return _IPv4Address(ptr);
}

// IPv6address =                            6( h16 ":" ) ls32 // case 1
//             /                       "::" 5( h16 ":" ) ls32 // case 2
//             / [               h16 ] "::" 4( h16 ":" ) ls32 // case 3
//             / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32 // case 4
//             / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32 // case 5
//             / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32 // case 6
//             / [ *4( h16 ":" ) h16 ] "::"              ls32 // case 7
//             / [ *5( h16 ":" ) h16 ] "::"              h16  // case 8
//             / [ *6( h16 ":" ) h16 ] "::"                   // case 9
static bool QT_FASTCALL _IPv6Address(const char **ptr)
{
    const char *ptrBackup = *ptr;

    // count of (h16 ":") to the left of and including ::
    int leftHexColons = 0;
    // count of (h16 ":") to the right of ::
    int rightHexColons = 0;

    // first count the number of (h16 ":") on the left of ::
    while (_h16(ptr)) {

        // an h16 not followed by a colon is considered an
        // error.
        if (**ptr != ':') {
            *ptr = ptrBackup;
            return false;
        }
        ++(*ptr);
        ++leftHexColons;

        // check for case 1, the only time when there can be no ::
        if (leftHexColons == 6 && _ls32(ptr)) {
            return true;
        }
    }

    // check for case 2 where the address starts with a :
    if (leftHexColons == 0 && *((*ptr)++) != ':') {
        *ptr = ptrBackup;
        return false;
    }

    // check for the second colon in ::
    if (*((*ptr)++) != ':') {
        *ptr = ptrBackup;
        return false;
    }

    int canBeCase = -1;
    bool ls32WasRead = false;

    const char *tmpBackup = *ptr;

    // count the number of (h16 ":") on the right of ::
    for (;;) {
        tmpBackup = *ptr;
        if (!_h16(ptr)) {
            if (!_ls32(ptr)) {
                if (rightHexColons != 0) {
                    *ptr = ptrBackup;
                    return false;
                }

                // the address ended with :: (case 9)
                // only valid if 1 <= leftHexColons <= 7
                canBeCase = 9;
            } else {
                ls32WasRead = true;
            }
            break;
        }
        ++rightHexColons;
        if (**ptr != ':') {
            // no colon could mean that what was read as an h16
            // was in fact the first part of an ls32. we backtrack
            // and retry.
            const char *pb = *ptr;
            *ptr = tmpBackup;
            if (_ls32(ptr)) {
                ls32WasRead = true;
                --rightHexColons;
            } else {
                *ptr = pb;
                // address ends with only 1 h16 after :: (case 8)
                if (rightHexColons == 1)
                    canBeCase = 8;
            }
            break;
        }
        ++(*ptr);
    }

    // determine which case it is based on the number of rightHexColons
    if (canBeCase == -1) {

        // check if a ls32 was read. If it wasn't and rightHexColons >= 2 then the
        // last 2 HexColons are in fact a ls32
        if (!ls32WasRead && rightHexColons >= 2)
            rightHexColons -= 2;

        canBeCase = 7 - rightHexColons;
    }

    // based on the case we need to check that the number of leftHexColons is valid
    if (leftHexColons > (canBeCase - 2)) {
        *ptr = ptrBackup;
        return false;
    }

    return true;
}

// IP-literal = "[" ( IPv6address / IPvFuture  ) "]"
static bool QT_FASTCALL _IPLiteral(const char **ptr)
{
    const char *ptrBackup = *ptr;
    if (**ptr != '[')
        return false;
    ++(*ptr);

    if (!_IPv6Address(ptr) && !_IPvFuture(ptr)) {
        *ptr = ptrBackup;
        return false;
    }

    if (**ptr != ']') {
        *ptr = ptrBackup;
        return false;
    }
    ++(*ptr);

    return true;
}

// reg-name    = *( unreserved / pct-encoded / sub-delims )
static void QT_FASTCALL _regName(const char **ptr)
{
    for (;;) {
        if (!_unreserved(ptr) && !_subDelims(ptr)) {
            if (!_pctEncoded(ptr))
                break;
        }
    }
}

// host        = IP-literal / IPv4address / reg-name
static void QT_FASTCALL _host(const char **ptr, QUrlParseData *parseData)
{
    parseData->host = *ptr;
    if (!_IPLiteral(ptr)) {
        if (_IPv4Address(ptr)) {
            char ch = **ptr;
            if (ch && ch != ':' && ch != '/') {
                // reset
                *ptr = parseData->host;
                _regName(ptr);
            }
        } else {
            _regName(ptr);
        }
    }
    parseData->hostLength = *ptr - parseData->host;
}

// userinfo    = *( unreserved / pct-encoded / sub-delims / ":" )
static void QT_FASTCALL _userInfo(const char **ptr, QUrlParseData *parseData)
{
    parseData->userInfo = *ptr;
    for (;;) {
        if (_unreserved(ptr) || _subDelims(ptr)) {
            ;
        } else {
            if (_pctEncoded(ptr)) {
                ;
            } else if (**ptr == ':') {
                parseData->userInfoDelimIndex = *ptr - parseData->userInfo;
                ++(*ptr);
            } else {
                break;
            }
        }
    }
    if (**ptr != '@') {
        *ptr = parseData->userInfo;
        parseData->userInfoDelimIndex = -1;
        return;
    }
    parseData->userInfoLength = *ptr - parseData->userInfo;
    ++(*ptr);
}

// port        = *DIGIT
static void QT_FASTCALL _port(const char **ptr, int *port)
{
    bool first = true;

    for (;;) {
        const char *ptrBackup = *ptr;
        char ch = *((*ptr)++);
        if (ch < '0' || ch > '9') {
            *ptr = ptrBackup;
            break;
        }

        if (first) {
            first = false;
            *port = 0;
        }

        *port *= 10;
        *port += ch - '0';
    }
}

// authority   = [ userinfo "@" ] host [ ":" port ]
static void QT_FASTCALL _authority(const char **ptr, QUrlParseData *parseData)
{
    _userInfo(ptr, parseData);
    _host(ptr, parseData);

    if (**ptr != ':')
        return;

    ++(*ptr);
    _port(ptr, &parseData->port);
}

// pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
static bool QT_FASTCALL _pchar(const char **ptr)
{
    char c = *(*ptr);

    switch (c) {
    case '!': case '$': case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case ';': case '=': case ':': case '@':
    case '-': case '.': case '_': case '~':
        ++(*ptr);
        return true;
    default:
        break;
    };

    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
        ++(*ptr);
        return true;
    }

    if (_pctEncoded(ptr))
        return true;

    return false;
}

// segment       = *pchar
static bool QT_FASTCALL _segmentNZ(const char **ptr)
{
    if (!_pchar(ptr))
        return false;

    while(_pchar(ptr))
        ;

    return true;
}

// path-abempty  = *( "/" segment )
static void QT_FASTCALL _pathAbEmpty(const char **ptr)
{
    for (;;) {
        if (**ptr != '/')
            break;
        ++(*ptr);

        while (_pchar(ptr))
            ;
    }
}

// path-abs      = "/" [ segment-nz *( "/" segment ) ]
static bool QT_FASTCALL _pathAbs(const char **ptr)
{
    // **ptr == '/' already checked in caller
    ++(*ptr);

    // we might be able to unnest this to gain some performance.
    if (!_segmentNZ(ptr))
        return true;

    _pathAbEmpty(ptr);

    return true;
}

// path-rootless = segment-nz *( "/" segment )
static bool QT_FASTCALL _pathRootless(const char **ptr)
{
    // we might be able to unnest this to gain some performance.
    if (!_segmentNZ(ptr))
        return false;

    _pathAbEmpty(ptr);

    return true;
}


// hier-part   = "//" authority path-abempty
//             / path-abs
//             / path-rootless
//             / path-empty
static void QT_FASTCALL _hierPart(const char **ptr, QUrlParseData *parseData)
{
    const char *ptrBackup = *ptr;
    const char *pathStart = 0;
    if (*((*ptr)++) == '/' && *((*ptr)++) == '/') {
        _authority(ptr, parseData);
        pathStart = *ptr;
        _pathAbEmpty(ptr);
    } else {
        *ptr = ptrBackup;
        pathStart = *ptr;
        if (**ptr == '/')
            _pathAbs(ptr);
        else
            _pathRootless(ptr);
    }
    parseData->path = pathStart;
    parseData->pathLength = *ptr - pathStart;
}

// query       = *( pchar / "/" / "?" )
static void QT_FASTCALL _query(const char **ptr, QUrlParseData *parseData)
{
    parseData->query = *ptr;
    for (;;) {
        if (_pchar(ptr)) {
            ;
        } else if (**ptr == '/' || **ptr == '?') {
            ++(*ptr);
        } else {
            break;
        }
    }
    parseData->queryLength = *ptr - parseData->query;
}

// fragment    = *( pchar / "/" / "?" )
static void QT_FASTCALL _fragment(const char **ptr, QUrlParseData *parseData)
{
    parseData->fragment = *ptr;
    for (;;) {
        if (_pchar(ptr)) {
            ;
        } else if (**ptr == '/' || **ptr == '?' || **ptr == '#') {
            ++(*ptr);
        } else {
            break;
        }
    }
    parseData->fragmentLength = *ptr - parseData->fragment;
}

bool qt_urlParse(const char *pptr, QUrlParseData &parseData)
{
    const char **ptr = &pptr;

#if defined (QURL_DEBUG)
    qDebug("QUrlPrivate::parse(), parsing \"%s\"", pptr);
#endif

    // optional scheme
    bool isSchemeValid = _scheme(ptr, &parseData);

    if (isSchemeValid == false) {
        char ch = *((*ptr)++);
        parseData.errorInfo->setParams(*ptr, QT_TRANSLATE_NOOP(QUrl, "unexpected URL scheme"),
                                       0, ch);
#if defined (QURL_DEBUG)
        qDebug("QUrlPrivate::parse(), unrecognized: %c%s", ch, *ptr);
#endif
        return false;
    }

    // hierpart
    _hierPart(ptr, &parseData);

    // optional query
    char ch = *((*ptr)++);
    if (ch == '?') {
        _query(ptr, &parseData);
        ch = *((*ptr)++);
    }

    // optional fragment
    if (ch == '#') {
        _fragment(ptr, &parseData);
    } else if (ch != '\0') {
        parseData.errorInfo->setParams(*ptr, QT_TRANSLATE_NOOP(QUrl, "expected end of URL"),
                                       0, ch);
#if defined (QURL_DEBUG)
        qDebug("QUrlPrivate::parse(), unrecognized: %c%s", ch, *ptr);
#endif
        return false;
    }

    return true;
}

bool qt_isValidUrlIP(const char *ptr)
{
    // returns true if it matches IP-Literal or IPv4Address
    // see _host above
    return (_IPLiteral(&ptr) || _IPv4Address(&ptr)) && !*ptr;
}

QT_END_NAMESPACE
