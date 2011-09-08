/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

/*!
    \class QUrl

    \brief The QUrl class provides a convenient interface for working
    with URLs.

    \reentrant
    \ingroup io
    \ingroup network
    \ingroup shared


    It can parse and construct URLs in both encoded and unencoded
    form. QUrl also has support for internationalized domain names
    (IDNs).

    The most common way to use QUrl is to initialize it via the
    constructor by passing a QString. Otherwise, setUrl() and
    setEncodedUrl() can also be used.

    URLs can be represented in two forms: encoded or unencoded. The
    unencoded representation is suitable for showing to users, but
    the encoded representation is typically what you would send to
    a web server. For example, the unencoded URL
    "http://b\uuml\c{}hler.example.com" would be sent to the server as
    "http://xn--bhler-kva.example.com/List%20of%20applicants.xml".

    A URL can also be constructed piece by piece by calling
    setScheme(), setUserName(), setPassword(), setHost(), setPort(),
    setPath(), setEncodedQuery() and setFragment(). Some convenience
    functions are also available: setAuthority() sets the user name,
    password, host and port. setUserInfo() sets the user name and
    password at once.

    Call isValid() to check if the URL is valid. This can be done at
    any point during the constructing of a URL.

    Constructing a query is particularly convenient through the use
    of setQueryItems(), addQueryItem() and removeQueryItem(). Use
    setQueryDelimiters() to customize the delimiters used for
    generating the query string.

    For the convenience of generating encoded URL strings or query
    strings, there are two static functions called
    fromPercentEncoding() and toPercentEncoding() which deal with
    percent encoding and decoding of QStrings.

    Calling isRelative() will tell whether or not the URL is
    relative. A relative URL can be resolved by passing it as argument
    to resolved(), which returns an absolute URL. isParentOf() is used
    for determining whether one URL is a parent of another.

    fromLocalFile() constructs a QUrl by parsing a local
    file path. toLocalFile() converts a URL to a local file path.

    The human readable representation of the URL is fetched with
    toString(). This representation is appropriate for displaying a
    URL to a user in unencoded form. The encoded form however, as
    returned by toEncoded(), is for internal use, passing to web
    servers, mail clients and so on.

    QUrl conforms to the URI specification from
    \l{RFC 3986} (Uniform Resource Identifier: Generic Syntax), and includes
    scheme extensions from \l{RFC 1738} (Uniform Resource Locators). Case
    folding rules in QUrl conform to \l{RFC 3491} (Nameprep: A Stringprep
    Profile for Internationalized Domain Names (IDN)).

    \section2 Character Conversions

    Follow these rules to avoid erroneous character conversion when
    dealing with URLs and strings:

    \list
    \li When creating an QString to contain a URL from a QByteArray or a
       char*, always use QString::fromUtf8().
    \endlist

    \sa QUrlInfo
*/

/*!
    \enum QUrl::ParsingMode

    The parsing mode controls the way QUrl parses strings.

    \value TolerantMode QUrl will try to correct some common errors in URLs.
                        This mode is useful when processing URLs entered by
                        users.

    \value StrictMode Only valid URLs are accepted. This mode is useful for
                      general URL validation.

    In TolerantMode, the parser corrects the following invalid input:

    \list

    \li Spaces and "%20": If an encoded URL contains a space, this will be
    replaced with "%20". If a decoded URL contains "%20", this will be
    replaced with a single space before the URL is parsed.

    \li Single "%" characters: Any occurrences of a percent character "%" not
    followed by exactly two hexadecimal characters (e.g., "13% coverage.html")
    will be replaced by "%25".

    \li Reserved and unreserved characters: An encoded URL should only
    contain a few characters as literals; all other characters should
    be percent-encoded. In TolerantMode, these characters will be
    automatically percent-encoded where they are not allowed:
            space / double-quote / "<" / ">" / "[" / "\" /
            "]" / "^" / "`" / "{" / "|" / "}"

    \endlist
*/

/*!
    \enum QUrl::FormattingOption

    The formatting options define how the URL is formatted when written out
    as text.

    \value None The format of the URL is unchanged.
    \value RemoveScheme  The scheme is removed from the URL.
    \value RemovePassword  Any password in the URL is removed.
    \value RemoveUserInfo  Any user information in the URL is removed.
    \value RemovePort      Any specified port is removed from the URL.
    \value RemoveAuthority
    \value RemovePath   The URL's path is removed, leaving only the scheme,
                        host address, and port (if present).
    \value RemoveQuery  The query part of the URL (following a '?' character)
                        is removed.
    \value RemoveFragment
    \value PreferLocalFile If the URL is a local file according to isLocalFile()
     and contains no query or fragment, a local file path is returned.
    \value StripTrailingSlash  The trailing slash is removed if one is present.

    Note that the case folding rules in \l{RFC 3491}{Nameprep}, which QUrl
    conforms to, require host names to always be converted to lower case,
    regardless of the Qt::FormattingOptions used.
*/

/*!
 \fn uint qHash(const QUrl &url)
 \since 4.7
 \relates QUrl

 Computes a hash key from the normalized version of \a url.
 */
#include "qurl.h"
#include "qurl_p.h"
#include "qplatformdefs.h"
#include "qatomic.h"
#include "qbytearray.h"
#include "qdir.h"
#include "qfile.h"
#include "qlist.h"
#ifndef QT_NO_REGEXP
#include "qregexp.h"
#endif
#include "qstring.h"
#include "qstringlist.h"
#include "qstack.h"
#include "qvarlengtharray.h"
#include "qdebug.h"
#include "qtldurl_p.h"
#if defined(Q_OS_WINCE_WM)
#pragma optimize("g", off)
#endif

QT_BEGIN_NAMESPACE

extern void q_normalizePercentEncoding(QByteArray *ba, const char *exclude);
extern void q_toPercentEncoding(QByteArray *ba, const char *exclude, const char *include = 0);
extern void q_fromPercentEncoding(QByteArray *ba);

static QByteArray toPercentEncodingHelper(const QString &s, const char *exclude, const char *include = 0)
{
    if (s.isNull())
        return QByteArray();    // null
    QByteArray ba = s.toUtf8();
    q_toPercentEncoding(&ba, exclude, include);
    return ba;
}

static QString fromPercentEncodingHelper(const QByteArray &ba)
{
    if (ba.isNull())
        return QString();       // null
    QByteArray copy = ba;
    q_fromPercentEncoding(&copy);
    return QString::fromUtf8(copy.constData(), copy.length());
}

static QString fromPercentEncodingMutable(QByteArray *ba)
{
    if (ba->isNull())
        return QString();       // null
    q_fromPercentEncoding(ba);
    return QString::fromUtf8(ba->constData(), ba->length());
}

// ### Qt 5: Consider accepting empty strings as valid. See task 144227.

//#define QURL_DEBUG

// implemented in qvsnprintf.cpp
Q_CORE_EXPORT int qsnprintf(char *str, size_t n, const char *fmt, ...);

#define QURL_SETFLAG(a, b) { (a) |= (b); }
#define QURL_UNSETFLAG(a, b) { (a) &= ~(b); }
#define QURL_HASFLAG(a, b) (((a) & (b)) == (b))

class QUrlPrivate
{
public:
    QUrlPrivate();
    QUrlPrivate(const QUrlPrivate &other);

    bool setUrl(const QString &url);

    QString canonicalHost() const;
    void ensureEncodedParts() const;
    QString authority(QUrl::FormattingOptions options = QUrl::None) const;
    void setAuthority(const QString &auth);
    void setUserInfo(const QString &userInfo);
    QString userInfo(QUrl::FormattingOptions options = QUrl::None) const;
    void setEncodedAuthority(const QByteArray &authority);
    void setEncodedUserInfo(const QUrlParseData *parseData);
    void setEncodedUrl(const QByteArray&, QUrl::ParsingMode);

    QByteArray mergePaths(const QByteArray &relativePath) const;

    void queryItem(int pos, int *value, int *end);

    enum ParseOptions {
        ParseAndSet,
        ParseOnly
    };

    void validate() const;
    void parse(ParseOptions parseOptions = ParseAndSet) const;
    void clear();

    QByteArray toEncoded(QUrl::FormattingOptions options = QUrl::None) const;
    bool isLocalFile() const;

    QAtomicInt ref;

    QString scheme;
    QString userName;
    QString password;
    QString host;
    QString path;
    QByteArray query;
    QString fragment;

    QByteArray encodedOriginal;
    QByteArray encodedUserName;
    QByteArray encodedPassword;
    QByteArray encodedPath;
    QByteArray encodedFragment;

    int port;
    QUrl::ParsingMode parsingMode;

    bool hasQuery;
    bool hasFragment;
    bool isValid;
    bool isHostValid;

    char valueDelimiter;
    char pairDelimiter;

    enum State {
        Parsed = 0x1,
        Validated = 0x2,
        Normalized = 0x4,
        HostCanonicalized = 0x8
    };
    int stateFlags;

    mutable QByteArray encodedNormalized;
    const QByteArray & normalized() const;

    mutable QUrlErrorInfo errorInfo;
    QString createErrorString();
};

QUrlPrivate::QUrlPrivate() : ref(1), port(-1), parsingMode(QUrl::TolerantMode),
    hasQuery(false), hasFragment(false), isValid(false), isHostValid(true),
    valueDelimiter('='), pairDelimiter('&'),
    stateFlags(0)
{
}

QUrlPrivate::QUrlPrivate(const QUrlPrivate &copy)
    : ref(1), scheme(copy.scheme),
      userName(copy.userName),
      password(copy.password),
      host(copy.host),
      path(copy.path),
      query(copy.query),
      fragment(copy.fragment),
      encodedOriginal(copy.encodedOriginal),
      encodedUserName(copy.encodedUserName),
      encodedPassword(copy.encodedPassword),
      encodedPath(copy.encodedPath),
      encodedFragment(copy.encodedFragment),
      port(copy.port),
      parsingMode(copy.parsingMode),
      hasQuery(copy.hasQuery),
      hasFragment(copy.hasFragment),
      isValid(copy.isValid),
      isHostValid(copy.isHostValid),
      valueDelimiter(copy.valueDelimiter),
      pairDelimiter(copy.pairDelimiter),
      stateFlags(copy.stateFlags),
      encodedNormalized(copy.encodedNormalized)
{
}

QString QUrlPrivate::canonicalHost() const
{
    if (QURL_HASFLAG(stateFlags, HostCanonicalized) || host.isEmpty())
        return host;

    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    QURL_SETFLAG(that->stateFlags, HostCanonicalized);
    if (host.contains(QLatin1Char(':'))) {
        // This is an IP Literal, use _IPLiteral to validate
        QByteArray ba = host.toLatin1();
        bool needsBraces = false;
        if (!ba.startsWith('[')) {
            // surround the IP Literal with [ ] if it's not already done so
            ba.reserve(ba.length() + 2);
            ba.prepend('[');
            ba.append(']');
            needsBraces = true;
        }

        const char *ptr = ba.constData();
        if (!qt_isValidUrlIP(ptr))
            that->host.clear();
        else if (needsBraces)
            that->host = QString::fromLatin1(ba.toLower());
        else
            that->host = host.toLower();
    } else {
        that->host = qt_ACE_do(host, NormalizeAce);
    }
    that->isHostValid = !that->host.isNull();
    return that->host;
}

// From RFC 3896, Appendix A Collected ABNF for URI
//    authority     = [ userinfo "@" ] host [ ":" port ]
//    userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
//    host          = IP-literal / IPv4address / reg-name
//    port          = *DIGIT
//[...]
//    pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
//
//    query         = *( pchar / "/" / "?" )
//
//    fragment      = *( pchar / "/" / "?" )
//
//    pct-encoded   = "%" HEXDIG HEXDIG
//
//    unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
//    reserved      = gen-delims / sub-delims
//    gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//    sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                  / "*" / "+" / "," / ";" / "="

// use defines for concatenation:
#define ABNF_sub_delims         "!$&'()*+,;="
#define ABNF_gen_delims         ":/?#[]@"
#define ABNF_pchar              ABNF_sub_delims ":@"
#define ABNF_reserved           ABNF_sub_delims ABNF_gen_delims

// list the characters that don't have to be converted according to the list above.
// "unreserved" is already automatically not encoded, so we don't have to list it.
// the path component has a complex ABNF that basically boils down to
// slash-separated segments of "pchar"

static const char userNameExcludeChars[] = ABNF_sub_delims;
static const char passwordExcludeChars[] = ABNF_sub_delims ":";
static const char pathExcludeChars[]     = ABNF_pchar "/";
static const char queryExcludeChars[]    = ABNF_pchar "/?";
static const char fragmentExcludeChars[] = ABNF_pchar "/?";

void QUrlPrivate::ensureEncodedParts() const
{
    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);

    if (encodedUserName.isNull())
        // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
        that->encodedUserName = toPercentEncodingHelper(userName, userNameExcludeChars);
    if (encodedPassword.isNull())
        // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
        that->encodedPassword = toPercentEncodingHelper(password, passwordExcludeChars);
    if (encodedPath.isNull())
        // pchar = unreserved / pct-encoded / sub-delims / ":" / "@" ... also "/"
        that->encodedPath = toPercentEncodingHelper(path, pathExcludeChars);
    if (encodedFragment.isNull())
        // fragment      = *( pchar / "/" / "?" )
        that->encodedFragment = toPercentEncodingHelper(fragment, fragmentExcludeChars);
}

QString QUrlPrivate::authority(QUrl::FormattingOptions options) const
{
    if ((options & QUrl::RemoveAuthority) == QUrl::RemoveAuthority)
        return QString();

    QString tmp = userInfo(options);
    if (!tmp.isEmpty())
        tmp += QLatin1Char('@');
    tmp += canonicalHost();
    if (!(options & QUrl::RemovePort) && port != -1)
        tmp += QLatin1Char(':') + QString::number(port);

    return tmp;
}

void QUrlPrivate::setAuthority(const QString &auth)
{
    isHostValid = true;
    if (auth.isEmpty()) {
        setUserInfo(QString());
        host.clear();
        port = -1;
        return;
    }

    // find the port section of the authority by searching from the
    // end towards the beginning for numbers until a ':' is reached.
    int portIndex = auth.length() - 1;
    if (portIndex == 0) {
        portIndex = -1;
    } else {
        short c = auth.at(portIndex--).unicode();
        if (c < '0' || c > '9') {
            portIndex = -1;
        } else while (portIndex >= 0) {
            c = auth.at(portIndex).unicode();
            if (c == ':') {
                break;
            } else if (c == '.') {
                portIndex = -1;
                break;
            }
            --portIndex;
        }
    }

    if (portIndex != -1) {
        port = 0;
        for (int i = portIndex + 1; i < auth.length(); ++i) 
            port = (port * 10) + (auth.at(i).unicode() - '0');
    } else {
        port = -1;
    }

    int userInfoIndex = auth.indexOf(QLatin1Char('@'));
    if (userInfoIndex != -1 && (portIndex == -1 || userInfoIndex < portIndex))
        setUserInfo(auth.left(userInfoIndex));

    int hostIndex = 0;
    if (userInfoIndex != -1)
        hostIndex = userInfoIndex + 1;
    int hostLength = auth.length() - hostIndex;
    if (portIndex != -1)
        hostLength -= (auth.length() - portIndex);

    host = auth.mid(hostIndex, hostLength).trimmed();
}

void QUrlPrivate::setUserInfo(const QString &userInfo)
{
    encodedUserName.clear();
    encodedPassword.clear();

    int delimIndex = userInfo.indexOf(QLatin1Char(':'));
    if (delimIndex == -1) {
        userName = userInfo;
        password.clear();
        return;
    }
    userName = userInfo.left(delimIndex);
    password = userInfo.right(userInfo.length() - delimIndex - 1);
}

void QUrlPrivate::setEncodedUserInfo(const QUrlParseData *parseData)
{
    userName.clear();
    password.clear();
    if (!parseData->userInfoLength) {
        encodedUserName.clear();
        encodedPassword.clear();
    } else if (parseData->userInfoDelimIndex == -1) {
        encodedUserName = QByteArray(parseData->userInfo, parseData->userInfoLength);
        encodedPassword.clear();
    } else {
        encodedUserName = QByteArray(parseData->userInfo, parseData->userInfoDelimIndex);
        encodedPassword = QByteArray(parseData->userInfo + parseData->userInfoDelimIndex + 1,
                                     parseData->userInfoLength - parseData->userInfoDelimIndex - 1);
    }
}

QString QUrlPrivate::userInfo(QUrl::FormattingOptions options) const
{
    if ((options & QUrl::RemoveUserInfo) == QUrl::RemoveUserInfo)
        return QString();

    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    if (userName.isNull())
        that->userName = fromPercentEncodingHelper(encodedUserName);
    if (password.isNull())
        that->password = fromPercentEncodingHelper(encodedPassword);

    QString tmp = userName;

    if (!(options & QUrl::RemovePassword) && !password.isEmpty()) {
        tmp += QLatin1Char(':');
        tmp += password;
    }
    
    return tmp;
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.3: Merge paths

    Returns a merge of the current path with the relative path passed
    as argument.
*/
QByteArray QUrlPrivate::mergePaths(const QByteArray &relativePath) const
{
    if (encodedPath.isNull())
        ensureEncodedParts();

    // If the base URI has a defined authority component and an empty
    // path, then return a string consisting of "/" concatenated with
    // the reference's path; otherwise,
    if (!authority().isEmpty() && encodedPath.isEmpty())
        return '/' + relativePath;

    // Return a string consisting of the reference's path component
    // appended to all but the last segment of the base URI's path
    // (i.e., excluding any characters after the right-most "/" in the
    // base URI path, or excluding the entire base URI path if it does
    // not contain any "/" characters).
    QByteArray newPath;
    if (!encodedPath.contains('/'))
        newPath = relativePath;
    else
        newPath = encodedPath.left(encodedPath.lastIndexOf('/') + 1) + relativePath;

    return newPath;
}

void QUrlPrivate::queryItem(int pos, int *value, int *end)
{
    *end = query.indexOf(pairDelimiter, pos);
    if (*end == -1)
        *end = query.size();
    *value = pos;
    while (*value < *end) {
        if (query[*value] == valueDelimiter)
            break;
        ++*value;
    }
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.4: Remove dot segments

    Removes unnecessary ../ and ./ from the path. Used for normalizing
    the URL.
*/
static void removeDotsFromPath(QByteArray *path)
{
    // The input buffer is initialized with the now-appended path
    // components and the output buffer is initialized to the empty
    // string.
    char *out = path->data();
    const char *in = out;
    const char *end = out + path->size();

    // If the input buffer consists only of
    // "." or "..", then remove that from the input
    // buffer;
    if (path->size() == 1 && in[0] == '.')
        ++in;
    else if (path->size() == 2 && in[0] == '.' && in[1] == '.')
        in += 2;
    // While the input buffer is not empty, loop:
    while (in < end) {

        // otherwise, if the input buffer begins with a prefix of "../" or "./",
        // then remove that prefix from the input buffer;
        if (path->size() >= 2 && in[0] == '.' && in[1] == '/')
            in += 2;
        else if (path->size() >= 3 && in[0] == '.' && in[1] == '.' && in[2] == '/')
            in += 3;

        // otherwise, if the input buffer begins with a prefix of
        // "/./" or "/.", where "." is a complete path segment,
        // then replace that prefix with "/" in the input buffer;
        if (in <= end - 3 && in[0] == '/' && in[1] == '.' && in[2] == '/') {
            in += 2;
            continue;
        } else if (in == end - 2 && in[0] == '/' && in[1] == '.') {
            *out++ = '/';
            in += 2;
            break;
        }
        
        // otherwise, if the input buffer begins with a prefix
        // of "/../" or "/..", where ".." is a complete path
        // segment, then replace that prefix with "/" in the
        // input buffer and remove the last //segment and its
        // preceding "/" (if any) from the output buffer;
        if (in <= end - 4 && in[0] == '/' && in[1] == '.' && in[2] == '.' && in[3] == '/') {
            while (out > path->constData() && *(--out) != '/')
                ;
            if (out == path->constData() && *out != '/')
                ++in;
            in += 3;
            continue;
        } else if (in == end - 3 && in[0] == '/' && in[1] == '.' && in[2] == '.') {
            while (out > path->constData() && *(--out) != '/')
                ;
            if (*out == '/')
                ++out;
            in += 3;
            break;
        }
        
        // otherwise move the first path segment in
        // the input buffer to the end of the output
        // buffer, including the initial "/" character
        // (if any) and any subsequent characters up
        // to, but not including, the next "/"
        // character or the end of the input buffer.
        *out++ = *in++;
        while (in < end && *in != '/')
            *out++ = *in++;
    }
    path->truncate(out - path->constData());
}

void QUrlPrivate::validate() const
{
    QUrlPrivate *that = (QUrlPrivate *)this;
    that->encodedOriginal = that->toEncoded(); // may detach
    parse(ParseOnly);

    QURL_SETFLAG(that->stateFlags, Validated);

    if (!isValid)
        return;

    QString auth = authority(); // causes the non-encoded forms to be valid

    // authority() calls canonicalHost() which sets this
    if (!isHostValid)
        return;

    if (scheme == QLatin1String("mailto")) {
        if (!host.isEmpty() || port != -1 || !userName.isEmpty() || !password.isEmpty()) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "expected empty host, username,"
                                                           "port and password"),
                                      0, 0);
        }
    } else if (scheme == QLatin1String("ftp") || scheme == QLatin1String("http")) {
        if (host.isEmpty() && !(path.isEmpty() && encodedPath.isEmpty())) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "the host is empty, but not the path"),
                                      0, 0);
        }
    }
}

void QUrlPrivate::parse(ParseOptions parseOptions) const
{
    QUrlPrivate *that = (QUrlPrivate *)this;
    that->errorInfo.setParams(0, 0, 0, 0);
    if (encodedOriginal.isEmpty()) {
        that->isValid = false;
        that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "empty"),
                                  0, 0);
        QURL_SETFLAG(that->stateFlags, Validated | Parsed);
        return;
    }


    QUrlParseData parseData;
    memset(&parseData, 0, sizeof(parseData));
    parseData.userInfoDelimIndex = -1;
    parseData.port = -1;
    parseData.errorInfo = &that->errorInfo;

    const char *pptr = (char *) encodedOriginal.constData();
    if (!qt_urlParse(pptr, parseData)) {
        that->isValid = false;
        QURL_SETFLAG(that->stateFlags, Validated | Parsed);
        return;
    }
    that->hasQuery = parseData.query;
    that->hasFragment = parseData.fragment;

    // when doing lazy validation, this function is called after
    // encodedOriginal has been constructed from the individual parts,
    // only to see if the constructed URL can be parsed. in that case,
    // parse() is called in ParseOnly mode; we don't want to set all
    // the members over again.
    if (parseOptions == ParseAndSet) {
        QURL_UNSETFLAG(that->stateFlags, HostCanonicalized);

        if (parseData.scheme) {
            QByteArray s(parseData.scheme, parseData.schemeLength);
            that->scheme = fromPercentEncodingMutable(&s).toLower();
        }

        that->setEncodedUserInfo(&parseData);

        QByteArray h(parseData.host, parseData.hostLength);
        that->host = fromPercentEncodingMutable(&h);
        that->port = parseData.port;

        that->path.clear();
        that->encodedPath = QByteArray(parseData.path, parseData.pathLength);

        if (that->hasQuery)
            that->query = QByteArray(parseData.query, parseData.queryLength);
        else
            that->query.clear();

        that->fragment.clear();
        if (that->hasFragment) {
            that->encodedFragment = QByteArray(parseData.fragment, parseData.fragmentLength);
        } else {
            that->encodedFragment.clear();
        }
    }

    that->isValid = true;
    QURL_SETFLAG(that->stateFlags, Parsed);

#if defined (QURL_DEBUG)
    qDebug("QUrl::setUrl(), scheme = %s", that->scheme.toLatin1().constData());
    qDebug("QUrl::setUrl(), userInfo = %s", that->userInfo.toLatin1().constData());
    qDebug("QUrl::setUrl(), host = %s", that->host.toLatin1().constData());
    qDebug("QUrl::setUrl(), port = %i", that->port);
    qDebug("QUrl::setUrl(), path = %s", fromPercentEncodingHelper(__path).toLatin1().constData());
    qDebug("QUrl::setUrl(), query = %s", __query.constData());
    qDebug("QUrl::setUrl(), fragment = %s", fromPercentEncodingHelper(__fragment).toLatin1().constData());
#endif
}

void QUrlPrivate::clear()
{
    scheme.clear();
    userName.clear();
    password.clear();
    host.clear();
    port = -1;
    path.clear();
    query.clear();
    fragment.clear();

    encodedOriginal.clear();
    encodedUserName.clear();
    encodedPassword.clear();
    encodedPath.clear();
    encodedFragment.clear();
    encodedNormalized.clear();

    isValid = false;
    hasQuery = false;
    hasFragment = false;

    valueDelimiter = '=';
    pairDelimiter = '&';

    QURL_UNSETFLAG(stateFlags, Parsed | Validated | Normalized | HostCanonicalized);
}

QByteArray QUrlPrivate::toEncoded(QUrl::FormattingOptions options) const
{
    if (!QURL_HASFLAG(stateFlags, Parsed)) parse();
    else ensureEncodedParts();

    if (options==0x100) // private - see qHash(QUrl)
        return normalized();

    if ((options & QUrl::PreferLocalFile) && isLocalFile() && !hasQuery && !hasFragment)
        return encodedPath;

    QByteArray url;

    if (!(options & QUrl::RemoveScheme) && !scheme.isEmpty()) {
        url += scheme.toLatin1();
        url += ':';
    }
    QString savedHost = host;  // pre-validation, may be invalid!
    QString auth = authority();
    bool doFileScheme = scheme == QLatin1String("file") && encodedPath.startsWith('/');
    if ((options & QUrl::RemoveAuthority) != QUrl::RemoveAuthority && (!auth.isEmpty() || doFileScheme || !savedHost.isEmpty())) {
        if (doFileScheme && !encodedPath.startsWith('/'))
            url += '/';
        url += "//";

        if ((options & QUrl::RemoveUserInfo) != QUrl::RemoveUserInfo) {
            bool hasUserOrPass = false;
            if (!userName.isEmpty()) {
                url += encodedUserName;
                hasUserOrPass = true;
            }
            if (!(options & QUrl::RemovePassword) && !password.isEmpty()) {
                url += ':';
                url += encodedPassword;
                hasUserOrPass = true;
            }
            if (hasUserOrPass)
                url += '@';
        }

        if (host.startsWith(QLatin1Char('['))) {
            url += host.toLatin1();
        } else if (host.contains(QLatin1Char(':'))) {
            url += '[';
            url += host.toLatin1();
            url += ']';
        } else if (host.isEmpty() && !savedHost.isEmpty()) {
            // this case is only possible with an invalid URL
            // it's here only so that we can keep the original, invalid hostname
            // in encodedOriginal.
            // QUrl::isValid() will return false, so toEncoded() can be anything (it's not valid)
            url += savedHost.toUtf8();
        } else {
            url += qt_ACE_do(host, ToAceOnly).toLatin1();
        }
        if (!(options & QUrl::RemovePort) && port != -1) {
            url += ':';
            url += QString::number(port).toAscii();
        }
    }

    if (!(options & QUrl::RemovePath)) {
        // check if we need to insert a slash
        if (!encodedPath.isEmpty() && !auth.isEmpty()) {
            if (!encodedPath.startsWith('/'))
                url += '/';
        }
        url += encodedPath;

        // check if we need to remove trailing slashes
        while ((options & QUrl::StripTrailingSlash) && url.endsWith('/'))
            url.chop(1);
    }

    if (!(options & QUrl::RemoveQuery) && hasQuery) {
        url += '?';
        url += query;
    }
    if (!(options & QUrl::RemoveFragment) && hasFragment) {
        url += '#';
        url += encodedFragment;
    }

    return url;
}

#define qToLower(ch) (((ch|32) >= 'a' && (ch|32) <= 'z') ? (ch|32) : ch)

const QByteArray &QUrlPrivate::normalized() const
{
    if (QURL_HASFLAG(stateFlags, QUrlPrivate::Normalized))
        return encodedNormalized;

    QUrlPrivate *that = const_cast<QUrlPrivate *>(this);
    QURL_SETFLAG(that->stateFlags, QUrlPrivate::Normalized);

    QUrlPrivate tmp = *this;
    tmp.host = tmp.canonicalHost();

    // ensure the encoded and normalized parts of the URL
    tmp.ensureEncodedParts();
    if (tmp.encodedUserName.contains('%'))
        q_normalizePercentEncoding(&tmp.encodedUserName, userNameExcludeChars);
    if (tmp.encodedPassword.contains('%'))
        q_normalizePercentEncoding(&tmp.encodedPassword, passwordExcludeChars);
    if (tmp.encodedFragment.contains('%'))
        q_normalizePercentEncoding(&tmp.encodedFragment, fragmentExcludeChars);

    if (tmp.encodedPath.contains('%')) {
        // the path is a bit special:
        // the slashes shouldn't be encoded or decoded.
        // They should remain exactly like they are right now
        //
        // treat the path as a slash-separated sequence of pchar
        QByteArray result;
        result.reserve(tmp.encodedPath.length());
        if (tmp.encodedPath.startsWith('/'))
            result.append('/');

        const char *data = tmp.encodedPath.constData();
        int lastSlash = 0;
        int nextSlash;
        do {
            ++lastSlash;
            nextSlash = tmp.encodedPath.indexOf('/', lastSlash);
            int len;
            if (nextSlash == -1)
                len = tmp.encodedPath.length() - lastSlash;
            else
                len = nextSlash - lastSlash;

            if (memchr(data + lastSlash, '%', len)) {
                // there's at least one percent before the next slash
                QByteArray block = QByteArray(data + lastSlash, len);
                q_normalizePercentEncoding(&block, pathExcludeChars);
                result.append(block);
            } else {
                // no percents in this path segment, append wholesale
                result.append(data + lastSlash, len);
            }

            // append the slash too, if it's there
            if (nextSlash != -1)
                result.append('/');

            lastSlash = nextSlash;
        } while (lastSlash != -1);

        tmp.encodedPath = result;
    }

    if (!tmp.scheme.isEmpty()) // relative test
        removeDotsFromPath(&tmp.encodedPath);

    int qLen = tmp.query.length();
    for (int i = 0; i < qLen; i++) {
        if (qLen - i > 2 && tmp.query.at(i) == '%') {
            ++i;
            tmp.query[i] = qToLower(tmp.query.at(i));
            ++i;
            tmp.query[i] = qToLower(tmp.query.at(i));
        }
    }
    encodedNormalized = tmp.toEncoded();

    return encodedNormalized;
}

QString QUrlPrivate::createErrorString()
{
    if (isValid && isHostValid)
        return QString();

    QString errorString(QLatin1String(QT_TRANSLATE_NOOP(QUrl, "Invalid URL \"")));
    errorString += QLatin1String(encodedOriginal.constData());
    errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "\""));

    if (errorInfo._source) {
        int position = encodedOriginal.indexOf(errorInfo._source) - 1;
        if (position > 0) {
            errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": error at position "));
            errorString += QString::number(position);
        } else {
            errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": "));
            errorString += QLatin1String(errorInfo._source);
        }
    }

    if (errorInfo._expected) {
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": expected \'"));
        errorString += QLatin1Char(errorInfo._expected);
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "\'"));
    } else {
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ": "));
        if (isHostValid)
            errorString += QLatin1String(errorInfo._message);
        else
            errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "invalid hostname"));
    }
    if (errorInfo._found) {
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, ", but found \'"));
        errorString += QLatin1Char(errorInfo._found);
        errorString += QLatin1String(QT_TRANSLATE_NOOP(QUrl, "\'"));
    }
    return errorString;
}

/*!
    \macro QT_NO_URL_CAST_FROM_STRING
    \relates QUrl

    Disables automatic conversions from QString (or char *) to QUrl.

    Compiling your code with this define is useful when you have a lot of
    code that uses QString for file names and you wish to convert it to
    use QUrl for network transparency. In any code that uses QUrl, it can
    help avoid missing QUrl::resolved() calls, and other misuses of
    QString to QUrl conversions.

    \oldcode
        url = filename; // probably not what you want
    \newcode
        url = QUrl::fromLocalFile(filename);
        url = baseurl.resolved(QUrl(filename));
    \endcode

    \sa QT_NO_CAST_FROM_ASCII
*/


/*!
    Constructs a URL by parsing \a url. \a url is assumed to be in human
    readable representation, with no percent encoding. QUrl will automatically
    percent encode all characters that are not allowed in a URL.
    The default parsing mode is TolerantMode.

    The parsing mode \a parsingMode is used for parsing \a url.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 0

    \sa setUrl(), TolerantMode
*/
QUrl::QUrl(const QString &url, ParsingMode parsingMode) : d(0)
{
    if (!url.isEmpty())
        setUrl(url, parsingMode);
    else {
        d = new QUrlPrivate;
        d->parsingMode = parsingMode;
    }
}

/*!
    Constructs an empty QUrl object.
*/
QUrl::QUrl() : d(0)
{
}

/*!
    Constructs a copy of \a other.
*/
QUrl::QUrl(const QUrl &other) : d(other.d)
{
    if (d)
        d->ref.ref();
}

/*!
    Destructor; called immediately before the object is deleted.
*/
QUrl::~QUrl()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Returns true if the URL is valid; otherwise returns false.

    The URL is run through a conformance test. Every part of the URL
    must conform to the standard encoding rules of the URI standard
    for the URL to be reported as valid.

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 2
*/
bool QUrl::isValid() const
{
    if (!d) return false;

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Validated)) d->validate();

    return d->isValid && d->isHostValid;
}

/*!
    Returns true if the URL has no data; otherwise returns false.
*/
bool QUrl::isEmpty() const
{
    if (!d) return true;

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed))
        return d->encodedOriginal.isEmpty();
    else
        return d->scheme.isEmpty()   // no encodedScheme
        && d->userName.isEmpty() && d->encodedUserName.isEmpty()
        && d->password.isEmpty() && d->encodedPassword.isEmpty()
        && d->host.isEmpty()   // no encodedHost
        && d->port == -1
        && d->path.isEmpty() && d->encodedPath.isEmpty()
        && d->query.isEmpty()
        && d->fragment.isEmpty() && d->encodedFragment.isEmpty();
}

/*!
    Resets the content of the QUrl. After calling this function, the
    QUrl is equal to one that has been constructed with the default
    empty constructor.
*/
void QUrl::clear()
{
    if (d && !d->ref.deref())
        delete d;
    d = 0;
}

/*!
    Constructs a URL by parsing the contents of \a url.

    \a url is assumed to be in unicode format, and encoded,
    such as URLs produced by url().

    The parsing mode \a parsingMode is used for parsing \a url.

    Calling isValid() will tell whether or not a valid URL was
    constructed.

    \sa setEncodedUrl()
*/
void QUrl::setUrl(const QString &url, ParsingMode parsingMode)
{
    detach();

    d->setEncodedUrl(url.toUtf8(), parsingMode);
    if (isValid() || parsingMode == StrictMode)
        return;

    // Tolerant preprocessing
    QString tmp = url;

    // Allow %20 in the QString variant
    tmp.replace(QLatin1String("%20"), QLatin1String(" "));

    // Percent-encode unsafe ASCII characters after host part
    int start = tmp.indexOf(QLatin1String("//"));
    if (start != -1) {
        // Has host part, find delimiter
        start += 2; // skip "//"
        const char delims[] = "/#?";
        const char *d = delims;
        int hostEnd = -1;
        while (*d && (hostEnd = tmp.indexOf(QLatin1Char(*d), start)) == -1)
            ++d;
        start = (hostEnd == -1) ? -1 : hostEnd + 1;
    } else {
        start = 0; // Has no host part
    }
    QByteArray encodedUrl;
    if (start != -1) {
        QString hostPart = tmp.left(start);
        QString otherPart = tmp.mid(start);
        encodedUrl = toPercentEncodingHelper(hostPart, ":/?#[]@!$&'()*+,;=")
                   + toPercentEncodingHelper(otherPart, ":/?#@!$&'()*+,;=");
    } else {
        encodedUrl = toPercentEncodingHelper(tmp, ABNF_reserved);
    }
    d->setEncodedUrl(encodedUrl, StrictMode);
}

inline static bool isHex(char c)
{
    c |= 0x20;
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static inline char toHex(quint8 c)
{
    return c > 9 ? c - 10 + 'A' : c + '0';
}

/*!
    \fn void QUrl::setEncodedUrl(const QByteArray &encodedUrl, ParsingMode parsingMode)
    Constructs a URL by parsing the contents of \a encodedUrl.

    \a encodedUrl is assumed to be a URL string in percent encoded
    form, containing only ASCII characters.

    The parsing mode \a parsingMode is used for parsing \a encodedUrl.

    \obsolete Use setUrl(QString::fromUtf8(encodedUrl), parsingMode)

    \sa setUrl()
*/


void QUrlPrivate::setEncodedUrl(const QByteArray &encodedUrl, QUrl::ParsingMode mode)
{
    QByteArray tmp = encodedUrl;
    clear();
    parsingMode = mode;
    if (parsingMode == QUrl::TolerantMode) {
        // Replace stray % with %25
        QByteArray copy = tmp;
        for (int i = 0, j = 0; i < copy.size(); ++i, ++j) {
            if (copy.at(i) == '%') {
                if (i + 2 >= copy.size() || !isHex(copy.at(i + 1)) || !isHex(copy.at(i + 2))) {
                    tmp.replace(j, 1, "%25");
                    j += 2;
                }
            }
        }

        // Find the host part
        int hostStart = tmp.indexOf("//");
        int hostEnd = -1;
        if (hostStart != -1) {
            // Has host part, find delimiter
            hostStart += 2; // skip "//"
            hostEnd = tmp.indexOf('/', hostStart);
            if (hostEnd == -1)
                hostEnd = tmp.indexOf('#', hostStart);
            if (hostEnd == -1)
                hostEnd = tmp.indexOf('?');
            if (hostEnd == -1)
                hostEnd = tmp.length() - 1;
        }

        // Reserved and unreserved characters are fine
//         unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
//         reserved      = gen-delims / sub-delims
//         gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
//         sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//                         / "*" / "+" / "," / ";" / "="
        // Replace everything else with percent encoding
        static const char doEncode[] = " \"<>[\\]^`{|}";
        static const char doEncodeHost[] = " \"<>\\^`{|}";
        for (int i = 0; i < tmp.size(); ++i) {
            quint8 c = quint8(tmp.at(i));
            if (c < 32 || c > 127 ||
                strchr(hostStart <= i && i <= hostEnd ? doEncodeHost : doEncode, c)) {
                char buf[4];
                buf[0] = '%';
                buf[1] = toHex(c >> 4);
                buf[2] = toHex(c & 0xf);
                buf[3] = '\0';
                tmp.replace(i, 1, buf);
                i += 2;
            }
        }
    }

    encodedOriginal = tmp;
}

/*!
    Sets the scheme of the URL to \a scheme. As a scheme can only
    contain ASCII characters, no conversion or encoding is done on the
    input.

    The scheme describes the type (or protocol) of the URL. It's
    represented by one or more ASCII characters at the start the URL,
    and is followed by a ':'. The following example shows a URL where
    the scheme is "ftp":

    \img qurl-authority2.png

    The scheme can also be empty, in which case the URL is interpreted
    as relative.

    \sa scheme(), isRelative()
*/
void QUrl::setScheme(const QString &scheme)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->scheme = scheme.toLower();
}

/*!
    Returns the scheme of the URL. If an empty string is returned,
    this means the scheme is undefined and the URL is then relative.

    The returned scheme is always lowercase, for convenience.

    \sa setScheme(), isRelative()
*/
QString QUrl::scheme() const
{
    if (!d) return QString();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->scheme;
}

/*!
    Sets the authority of the URL to \a authority.

    The authority of a URL is the combination of user info, a host
    name and a port. All of these elements are optional; an empty
    authority is therefore valid.

    The user info and host are separated by a '@', and the host and
    port are separated by a ':'. If the user info is empty, the '@'
    must be omitted; although a stray ':' is permitted if the port is
    empty.

    The following example shows a valid authority string:

    \img qurl-authority.png
*/
void QUrl::setAuthority(const QString &authority)
{
    if (!d) d = new QUrlPrivate;

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized | QUrlPrivate::HostCanonicalized);
    d->setAuthority(authority);
}

/*!
    Returns the authority of the URL if it is defined; otherwise
    an empty string is returned.

    \sa setAuthority()
*/
QString QUrl::authority() const
{
    if (!d) return QString();

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->authority();
}

/*!
    Sets the user info of the URL to \a userInfo. The user info is an
    optional part of the authority of the URL, as described in
    setAuthority().

    The user info consists of a user name and optionally a password,
    separated by a ':'. If the password is empty, the colon must be
    omitted. The following example shows a valid user info string:

    \img qurl-authority3.png

    \sa userInfo(), setUserName(), setPassword(), setAuthority()
*/
void QUrl::setUserInfo(const QString &userInfo)
{
    if (!d) d = new QUrlPrivate;

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->setUserInfo(userInfo.trimmed());
}

/*!
    Returns the user info of the URL, or an empty string if the user
    info is undefined.
*/
QString QUrl::userInfo() const
{
    if (!d) return QString();

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->userInfo();
}

/*!
    Sets the URL's user name to \a userName. The \a userName is part
    of the user info element in the authority of the URL, as described
    in setUserInfo().

    \sa setEncodedUserName(), userName(), setUserInfo()
*/
void QUrl::setUserName(const QString &userName)
{
    if (!d) d = new QUrlPrivate;

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->userName = userName;
    d->encodedUserName.clear();
}

/*!
    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned.

    \sa setUserName(), encodedUserName()
*/
QString QUrl::userName() const
{
    if (!d) return QString();

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->userInfo();              // causes the unencoded form to be set
    return d->userName;
}

/*!
    \since 4.4

    Sets the URL's user name to the percent-encoded \a userName. The \a
    userName is part of the user info element in the authority of the
    URL, as described in setUserInfo().

    Note: this function does not verify that \a userName is properly
    encoded. It is the caller's responsibility to ensure that the any
    delimiters (such as colons or slashes) are properly encoded.

    \sa setUserName(), encodedUserName(), setUserInfo()
*/
void QUrl::setEncodedUserName(const QByteArray &userName)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedUserName = userName;
    d->userName.clear();
}

/*!
    \since 4.4

    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedUserName()
*/
QByteArray QUrl::encodedUserName() const
{
    if (!d) return QByteArray();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedUserName;
}

/*!
    Sets the URL's password to \a password. The \a password is part of
    the user info element in the authority of the URL, as described in
    setUserInfo().

    \sa password(), setUserInfo()
*/
void QUrl::setPassword(const QString &password)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->password = password;
    d->encodedPassword.clear();
}

/*!
    Returns the password of the URL if it is defined; otherwise
    an empty string is returned.

    \sa setPassword()
*/
QString QUrl::password() const
{
    if (!d) return QString();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->userInfo();              // causes the unencoded form to be set
    return d->password;
}

/*!
    \since 4.4

    Sets the URL's password to the percent-encoded \a password. The \a
    password is part of the user info element in the authority of the
    URL, as described in setUserInfo().

    Note: this function does not verify that \a password is properly
    encoded. It is the caller's responsibility to ensure that the any
    delimiters (such as colons or slashes) are properly encoded.

    \sa setPassword(), encodedPassword(), setUserInfo()
*/
void QUrl::setEncodedPassword(const QByteArray &password)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedPassword = password;
    d->password.clear();
}

/*!
    \since 4.4

    Returns the password of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedPassword(), toEncoded()
*/
QByteArray QUrl::encodedPassword() const
{
    if (!d) return QByteArray();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedPassword;
}

/*!
    Sets the host of the URL to \a host. The host is part of the
    authority.

    \sa host(), setAuthority()
*/
void QUrl::setHost(const QString &host)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    d->isHostValid = true;
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized | QUrlPrivate::HostCanonicalized);

    d->host = host;
}

/*!
    Returns the host of the URL if it is defined; otherwise
    an empty string is returned.
*/
QString QUrl::host() const
{
    if (!d) return QString();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    if (d->host.isEmpty() || d->host.at(0) != QLatin1Char('['))
        return d->canonicalHost();
    QString tmp = d->host.mid(1);
    tmp.truncate(tmp.length() - 1);
    return tmp;
}

/*!
    \since 4.4

    Sets the URL's host to the ACE- or percent-encoded \a host. The \a
    host is part of the user info element in the authority of the
    URL, as described in setAuthority().

    \sa setHost(), encodedHost(), setAuthority(), fromAce()
*/
void QUrl::setEncodedHost(const QByteArray &host)
{
    setHost(fromPercentEncodingHelper(host));
}

/*!
    \since 4.4

    Returns the host part of the URL if it is defined; otherwise
    an empty string is returned.

    Note: encodedHost() does not return percent-encoded hostnames. Instead,
    the ACE-encoded (bare ASCII in Punycode encoding) form will be
    returned for any non-ASCII hostname.

    This function is equivalent to calling QUrl::toAce() on the return
    value of host().

    \sa setEncodedHost()
*/
QByteArray QUrl::encodedHost() const
{
    // should we cache this in d->encodedHost?
    return qt_ACE_do(host(), ToAceOnly).toLatin1();
}

/*!
    Sets the port of the URL to \a port. The port is part of the
    authority of the URL, as described in setAuthority().

    \a port must be between 0 and 65535 inclusive. Setting the
    port to -1 indicates that the port is unspecified.
*/
void QUrl::setPort(int port)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    if (port < -1 || port > 65535) {
        qWarning("QUrl::setPort: Out of range");
        port = -1;
    }

    d->port = port;
}

/*!
    \since 4.1

    Returns the port of the URL, or \a defaultPort if the port is
    unspecified.

    Example:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 3
*/
int QUrl::port(int defaultPort) const
{
    if (!d) return defaultPort;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    return d->port == -1 ? defaultPort : d->port;
}

/*!
    Sets the path of the URL to \a path. The path is the part of the
    URL that comes after the authority but before the query string.

    \img qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \img qurl-mailtopath.png

    \sa path()
*/
void QUrl::setPath(const QString &path)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->path = path;
    d->encodedPath.clear();
}

/*!
    Returns the path of the URL.

    \sa setPath()
*/
QString QUrl::path() const
{
    if (!d) return QString();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    if (d->path.isNull()) {
        QUrlPrivate *that = const_cast<QUrlPrivate *>(d);
        that->path = fromPercentEncodingHelper(d->encodedPath);
    }
    return d->path;
}

/*!
    \since 4.4

    Sets the URL's path to the percent-encoded \a path.  The path is
    the part of the URL that comes after the authority but before the
    query string.

    \img qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \img qurl-mailtopath.png

    Note: this function does not verify that \a path is properly
    encoded. It is the caller's responsibility to ensure that the any
    delimiters (such as '?' and '#') are properly encoded.

    \sa setPath(), encodedPath(), setUserInfo()
*/
void QUrl::setEncodedPath(const QByteArray &path)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedPath = path;
    d->path.clear();
}

/*!
    \since 4.4

    Returns the path of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedPath(), toEncoded()
*/
QByteArray QUrl::encodedPath() const
{
    if (!d) return QByteArray();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedPath;
}

/*!
    \since 4.2

    Returns true if this URL contains a Query (i.e., if ? was seen on it).

    \sa hasQueryItem(), encodedQuery()
*/
bool QUrl::hasQuery() const
{
    if (!d) return false;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->hasQuery;
}

/*!
    Sets the query string of the URL to \a query. The string is
    inserted as-is, and no further encoding is performed when calling
    toEncoded().

    This function is useful if you need to pass a query string that
    does not fit into the key-value pattern, or that uses a different
    scheme for encoding special characters than what is suggested by
    QUrl.

    Passing a value of QByteArray() to \a query (a null QByteArray) unsets
    the query completely. However, passing a value of QByteArray("")
    will set the query to an empty value, as if the original URL
    had a lone "?".

    \sa encodedQuery(), hasQuery()
*/
void QUrl::setEncodedQuery(const QByteArray &query)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->query = query;
    d->hasQuery = !query.isNull();
}

/*!
    Returns the query string of the URL in percent encoded form.
*/
QByteArray QUrl::encodedQuery() const
{
    if (!d) return QByteArray();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->query;
}

/*!
    Sets the fragment of the URL to \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \img qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QString() (a null QString) will unset the fragment.
    Passing an argument of QString("") (an empty but not null QString)
    will set the fragment to an empty string (as if the original URL
    had a lone "#").

    \sa fragment(), hasFragment()
*/
void QUrl::setFragment(const QString &fragment)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->fragment = fragment;
    d->hasFragment = !fragment.isNull();
    d->encodedFragment.clear();
}

/*!
    Returns the fragment of the URL.

    \sa setFragment()
*/
QString QUrl::fragment() const
{
    if (!d) return QString();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    if (d->fragment.isNull() && !d->encodedFragment.isNull()) {
        QUrlPrivate *that = const_cast<QUrlPrivate *>(d);
        that->fragment = fromPercentEncodingHelper(d->encodedFragment);
    }
    return d->fragment;
}

/*!
    \since 4.4

    Sets the URL's fragment to the percent-encoded \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \img qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QByteArray() (a null QByteArray) will unset
    the fragment.  Passing an argument of QByteArray("") (an empty but
    not null QByteArray) will set the fragment to an empty string (as
    if the original URL had a lone "#").

    \sa setFragment(), encodedFragment()
*/
void QUrl::setEncodedFragment(const QByteArray &fragment)
{
    if (!d) d = new QUrlPrivate;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    detach();
    QURL_UNSETFLAG(d->stateFlags, QUrlPrivate::Validated | QUrlPrivate::Normalized);

    d->encodedFragment = fragment;
    d->hasFragment = !fragment.isNull();
    d->fragment.clear();
}

/*!
    \since 4.4

    Returns the fragment of the URL if it is defined; otherwise an
    empty string is returned. The returned value will have its
    non-ASCII and other control characters percent-encoded, as in
    toEncoded().

    \sa setEncodedFragment(), toEncoded()
*/
QByteArray QUrl::encodedFragment() const
{
    if (!d) return QByteArray();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    d->ensureEncodedParts();
    return d->encodedFragment;
}

/*!
    \since 4.2

    Returns true if this URL contains a fragment (i.e., if # was seen on it).

    \sa fragment(), setFragment()
*/
bool QUrl::hasFragment() const
{
    if (!d) return false;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->hasFragment;
}

/*!
    \since 4.8

    Returns the TLD (Top-Level Domain) of the URL, (e.g. .co.uk, .net).
    Note that the return value is prefixed with a '.' unless the
    URL does not contain a valid TLD, in which case the function returns
    an empty string.
*/
QString QUrl::topLevelDomain() const
{
    return qTopLevelDomain(host());
}

/*!
    Returns the result of the merge of this URL with \a relative. This
    URL is used as a base to convert \a relative to an absolute URL.

    If \a relative is not a relative URL, this function will return \a
    relative directly. Otherwise, the paths of the two URLs are
    merged, and the new URL returned has the scheme and authority of
    the base URL, but with the merged path, as in the following
    example:

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 5

    Calling resolved() with ".." returns a QUrl whose directory is
    one level higher than the original. Similarly, calling resolved()
    with "../.." removes two levels from the path. If \a relative is
    "/", the path becomes "/".

    \sa isRelative()
*/
QUrl QUrl::resolved(const QUrl &relative) const
{
    if (!d) return relative;
    if (!relative.d) return *this;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    if (!QURL_HASFLAG(relative.d->stateFlags, QUrlPrivate::Parsed))
        relative.d->parse();

    d->ensureEncodedParts();
    relative.d->ensureEncodedParts();

    QUrl t;
    // be non strict and allow scheme in relative url
    if (!relative.d->scheme.isEmpty() && relative.d->scheme != d->scheme) {
        t = relative;
    } else {
        if (!relative.authority().isEmpty()) {
            t = relative;
        } else {
            t.d = new QUrlPrivate;
            if (relative.d->encodedPath.isEmpty()) {
                t.d->encodedPath = d->encodedPath;
                t.setEncodedQuery(relative.d->hasQuery ? relative.d->query : d->query);
            } else {
                t.d->encodedPath = relative.d->encodedPath.at(0) == '/'
                                       ? relative.d->encodedPath
                                       : d->mergePaths(relative.d->encodedPath);
                t.setEncodedQuery(relative.d->query);
            }
            t.d->encodedUserName = d->encodedUserName;
            t.d->encodedPassword = d->encodedPassword;
            t.d->host = d->host;
            t.d->port = d->port;
        }
        t.setScheme(d->scheme);
    }
    t.setFragment(relative.fragment());
    removeDotsFromPath(&t.d->encodedPath);
    t.d->path.clear();

#if defined(QURL_DEBUG)
    qDebug("QUrl(\"%s\").resolved(\"%s\") = \"%s\"",
           toEncoded().constData(),
           relative.toEncoded().constData(),
           t.toEncoded().constData());
#endif
    return t;
}

/*!
    Returns true if the URL is relative; otherwise returns false. A
    URL is relative if its scheme is undefined; this function is
    therefore equivalent to calling scheme().isEmpty().
*/
bool QUrl::isRelative() const
{
    if (!d) return true;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    return d->scheme.isEmpty();
}

// Encodes only what really needs to be encoded.
// \a input must be decoded.
static QString toPrettyPercentEncoding(const QString &input, bool forFragment)
{
    const int len = input.length();
    QString result;
    result.reserve(len);
    for (int i = 0; i < len; ++i) {
        const QChar c = input.at(i);
        register ushort u = c.unicode();
        if (u < 0x20
        || (!forFragment && u == '?') // don't escape '?' in fragments
        || u == '#' || u == '%'
        || (u == ' ' && (i+1 == len|| input.at(i+1).unicode() == ' '))) {
            static const char hexdigits[] = "0123456789ABCDEF";
            result += QLatin1Char('%');
            result += QLatin1Char(hexdigits[(u & 0xf0) >> 4]);
            result += QLatin1Char(hexdigits[u & 0xf]);
        } else {
            result += c;
        }
    }

    return result;
}

/*!
    Returns a string representation of the URL.
    The output can be customized by passing flags with \a options.

    The resulting QString can be passed back to a QUrl later on.

    Synonym for url(options).

    \sa FormattingOptions, toEncoded(), url()
*/
QString QUrl::toString(FormattingOptions options) const
{
    if (!d) return QString();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QString url;

    const QString ourPath = path();
    if ((options & QUrl::PreferLocalFile) && isLocalFile() && !d->hasQuery && !d->hasFragment)
        return ourPath;

    if (!(options & QUrl::RemoveScheme) && !d->scheme.isEmpty())
        url += d->scheme + QLatin1Char(':');
    if ((options & QUrl::RemoveAuthority) != QUrl::RemoveAuthority) {
        bool doFileScheme = d->scheme == QLatin1String("file") && ourPath.startsWith(QLatin1Char('/'));
        QString tmp = d->authority(options);
        if (!tmp.isNull() || doFileScheme) {
            if (doFileScheme && !ourPath.startsWith(QLatin1Char('/')))
                url += QLatin1Char('/');
            url += QLatin1String("//");
            url += tmp;
        }
    }
    if (!(options & QUrl::RemovePath)) {
        // check if we need to insert a slash
        if ((options & QUrl::RemoveAuthority) != QUrl::RemoveAuthority
            && !d->authority(options).isEmpty() && !ourPath.isEmpty() && ourPath.at(0) != QLatin1Char('/'))
            url += QLatin1Char('/');
        url += toPrettyPercentEncoding(ourPath, false);
        // check if we need to remove trailing slashes
        while ((options & StripTrailingSlash) && url.endsWith(QLatin1Char('/')))
            url.chop(1);
    }

    if (!(options & QUrl::RemoveQuery) && d->hasQuery) {
        url += QLatin1Char('?');
        url += QString::fromUtf8(QByteArray::fromPercentEncoding(d->query));
    }
    if (!(options & QUrl::RemoveFragment) && d->hasFragment) {
        url += QLatin1Char('#');
        url += fragment();
    }

    return url;
}

/*!
    \since 5.0
    Returns a string representation of the URL.
    The output can be customized by passing flags with \a options.

    The resulting QString can be passed back to a QUrl later on.

    Synonym for toString(options).

    \sa FormattingOptions, toEncoded(), toString()
*/
QString QUrl::url(FormattingOptions options) const
{
    return toString(options);
}

/*!
    \since 5.0

    Returns a human-displayable string representation of the URL.
    The output can be customized by passing flags with \a options.
    The option RemovePassword is always enabled, since passwords
    should never be shown back to users.

    With the default options, the resulting QString can be passed back
    to a QUrl later on, but any password that was present initially will
    be lost.

    \sa FormattingOptions, toEncoded(), toString()
*/

QString QUrl::toDisplayString(FormattingOptions options) const
{
    return toString(options | RemovePassword);
}

/*!
    Returns the encoded representation of the URL if it's valid;
    otherwise an empty QByteArray is returned. The output can be
    customized by passing flags with \a options.

    The user info, path and fragment are all converted to UTF-8, and
    all non-ASCII characters are then percent encoded. The host name
    is encoded using Punycode.
*/
QByteArray QUrl::toEncoded(FormattingOptions options) const
{
    if (!d) return QByteArray();
    return d->toEncoded(options);
}

/*!
    \fn QUrl QUrl::fromEncoded(const QByteArray &input, ParsingMode parsingMode)
    \obsolete

    Parses \a input and returns the corresponding QUrl. \a input is
    assumed to be in encoded form, containing only ASCII characters.

    The URL is parsed using \a parsingMode.

    Use QUrl(QString::fromUtf8(input), parsingMode) instead.

    \sa toEncoded(), setUrl()
*/

/*!
    Returns a decoded copy of \a input. \a input is first decoded from
    percent encoding, then converted from UTF-8 to unicode.
*/
QString QUrl::fromPercentEncoding(const QByteArray &input)
{
    return QString::fromUtf8(QByteArray::fromPercentEncoding(input));
}

/*!
    Returns an encoded copy of \a input. \a input is first converted
    to UTF-8, and all ASCII-characters that are not in the unreserved group
    are percent encoded. To prevent characters from being percent encoded
    pass them to \a exclude. To force characters to be percent encoded pass
    them to \a include.

    Unreserved is defined as:
       ALPHA / DIGIT / "-" / "." / "_" / "~"

    \snippet doc/src/snippets/code/src_corelib_io_qurl.cpp 6
*/
QByteArray QUrl::toPercentEncoding(const QString &input, const QByteArray &exclude, const QByteArray &include)
{
    return input.toUtf8().toPercentEncoding(exclude, include);
}

/*!
    \fn QByteArray QUrl::toPunycode(const QString &uc)
    \obsolete
    Returns a \a uc in Punycode encoding.

    Punycode is a Unicode encoding used for internationalized domain
    names, as defined in RFC3492. If you want to convert a domain name from
    Unicode to its ASCII-compatible representation, use toAce().
*/

/*!
    \fn QString QUrl::fromPunycode(const QByteArray &pc)
    \obsolete
    Returns the Punycode decoded representation of \a pc.

    Punycode is a Unicode encoding used for internationalized domain
    names, as defined in RFC3492. If you want to convert a domain from
    its ASCII-compatible encoding to the Unicode representation, use
    fromAce().
*/

/*!
    \since 4.2

    Returns the Unicode form of the given domain name
    \a domain, which is encoded in the ASCII Compatible Encoding (ACE).
    The result of this function is considered equivalent to \a domain.

    If the value in \a domain cannot be encoded, it will be converted
    to QString and returned.

    The ASCII Compatible Encoding (ACE) is defined by RFC 3490, RFC 3491
    and RFC 3492. It is part of the Internationalizing Domain Names in
    Applications (IDNA) specification, which allows for domain names
    (like \c "example.com") to be written using international
    characters.
*/
QString QUrl::fromAce(const QByteArray &domain)
{
    return qt_ACE_do(QString::fromLatin1(domain), NormalizeAce);
}

/*!
    \since 4.2

    Returns the ASCII Compatible Encoding of the given domain name \a domain.
    The result of this function is considered equivalent to \a domain.

    The ASCII-Compatible Encoding (ACE) is defined by RFC 3490, RFC 3491
    and RFC 3492. It is part of the Internationalizing Domain Names in
    Applications (IDNA) specification, which allows for domain names
    (like \c "example.com") to be written using international
    characters.

    This function return an empty QByteArra if \a domain is not a valid
    hostname. Note, in particular, that IPv6 literals are not valid domain
    names.
*/
QByteArray QUrl::toAce(const QString &domain)
{
    QString result = qt_ACE_do(domain, ToAceOnly);
    return result.toLatin1();
}

/*!
    \internal

    Returns true if this URL is "less than" the given \a url. This
    provides a means of ordering URLs.
*/
bool QUrl::operator <(const QUrl &url) const
{
    if (!d) return url.d ? QByteArray() < url.d->normalized() : false;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!url.d) return d->normalized() < QByteArray();
    if (!QURL_HASFLAG(url.d->stateFlags, QUrlPrivate::Parsed)) url.d->parse();
    return d->normalized() < url.d->normalized();
}

/*!
    Returns true if this URL and the given \a url are equal;
    otherwise returns false.
*/
bool QUrl::operator ==(const QUrl &url) const
{
    if (!d) return url.isEmpty();
    if (!url.d) return isEmpty();
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    if (!QURL_HASFLAG(url.d->stateFlags, QUrlPrivate::Parsed)) url.d->parse();
    return d->normalized() == url.d->normalized();
}

/*!
    Returns true if this URL and the given \a url are not equal;
    otherwise returns false.
*/
bool QUrl::operator !=(const QUrl &url) const
{
    return !(*this == url);
}

/*!
    Assigns the specified \a url to this object.
*/
QUrl &QUrl::operator =(const QUrl &url)
{
    if (!d) {
        if (url.d) {
            url.d->ref.ref();
            d = url.d;
        }
    } else {
        if (url.d)
            qAtomicAssign(d, url.d);
        else
            clear();
    }
    return *this;
}

/*!
    Assigns the specified \a url to this object.
*/
QUrl &QUrl::operator =(const QString &url)
{
    if (url.isEmpty()) {
        clear();
    } else {
        QUrl tmp(url);
        if (!d) d = new QUrlPrivate;
        qAtomicAssign(d, tmp.d);
    }
    return *this;
}

/*!
    \fn void QUrl::swap(QUrl &other)
    \since 4.8

    Swaps URL \a other with this URL. This operation is very
    fast and never fails.
*/

/*! \internal

    Forces a detach.
*/
void QUrl::detach()
{
    if (!d)
        d = new QUrlPrivate;
    else
        qAtomicDetach(d);
}

/*!
    \internal
*/
bool QUrl::isDetached() const
{
    return !d || d->ref.load() == 1;
}


/*!
    Returns a QUrl representation of \a localFile, interpreted as a local
    file. This function accepts paths separated by slashes as well as the
    native separator for this platform.

    This function also accepts paths with a doubled leading slash (or
    backslash) to indicate a remote file, as in
    "//servername/path/to/file.txt". Note that only certain platforms can
    actually open this file using QFile::open().

    \sa toLocalFile(), isLocalFile(), QDir::toNativeSeparators()
*/
QUrl QUrl::fromLocalFile(const QString &localFile)
{
    QUrl url;
    url.setScheme(QLatin1String("file"));
    QString deslashified = QDir::fromNativeSeparators(localFile);

    // magic for drives on windows
    if (deslashified.length() > 1 && deslashified.at(1) == QLatin1Char(':') && deslashified.at(0) != QLatin1Char('/')) {
        url.setPath(QLatin1Char('/') + deslashified);
    // magic for shared drive on windows
    } else if (deslashified.startsWith(QLatin1String("//"))) {
        int indexOfPath = deslashified.indexOf(QLatin1Char('/'), 2);
        url.setHost(deslashified.mid(2, indexOfPath - 2));
        if (indexOfPath > 2)
            url.setPath(deslashified.right(deslashified.length() - indexOfPath));
    } else {
        url.setPath(deslashified);
    }

    return url;
}

/*!
    Returns the path of this URL formatted as a local file path. The path
    returned will use forward slashes, even if it was originally created
    from one with backslashes.

    If this URL contains a non-empty hostname, it will be encoded in the
    returned value in the form found on SMB networks (for example,
    "//servername/path/to/file.txt").

    \sa fromLocalFile(), isLocalFile()
*/
QString QUrl::toLocalFile() const
{
    // the call to isLocalFile() also ensures that we're parsed
    if (!isLocalFile())
        return QString();

    QString tmp;
    QString ourPath = path();

    // magic for shared drive on windows
    if (!d->host.isEmpty()) {
        tmp = QLatin1String("//") + d->host + (ourPath.length() > 0 && ourPath.at(0) != QLatin1Char('/')
                                               ? QLatin1Char('/') + ourPath :  ourPath);
    } else {
        tmp = ourPath;
        // magic for drives on windows
        if (ourPath.length() > 2 && ourPath.at(0) == QLatin1Char('/') && ourPath.at(2) == QLatin1Char(':'))
            tmp.remove(0, 1);
    }

    return tmp;
}

bool QUrlPrivate::isLocalFile() const
{
    if (scheme.compare(QLatin1String("file"), Qt::CaseInsensitive) != 0)
        return false;   // not file
    return true;
}

/*!
    \since 4.7
    Returns true if this URL is pointing to a local file path. A URL is a
    local file path if the scheme is "file".

    Note that this function considers URLs with hostnames to be local file
    paths, even if the eventual file path cannot be opened with
    QFile::open().

    \sa fromLocalFile(), toLocalFile()
*/
bool QUrl::isLocalFile() const
{
    if (!d) return false;
    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();
    return d->isLocalFile();
}

/*!
    Returns true if this URL is a parent of \a childUrl. \a childUrl is a child
    of this URL if the two URLs share the same scheme and authority,
    and this URL's path is a parent of the path of \a childUrl.
*/
bool QUrl::isParentOf(const QUrl &childUrl) const
{
    QString childPath = childUrl.path();

    if (!d)
        return ((childUrl.scheme().isEmpty())
            && (childUrl.authority().isEmpty())
            && childPath.length() > 0 && childPath.at(0) == QLatin1Char('/'));

    if (!QURL_HASFLAG(d->stateFlags, QUrlPrivate::Parsed)) d->parse();

    QString ourPath = path();

    return ((childUrl.scheme().isEmpty() || d->scheme == childUrl.scheme())
            && (childUrl.authority().isEmpty() || d->authority() == childUrl.authority())
            &&  childPath.startsWith(ourPath)
            && ((ourPath.endsWith(QLatin1Char('/')) && childPath.length() > ourPath.length())
                || (!ourPath.endsWith(QLatin1Char('/'))
                    && childPath.length() > ourPath.length() && childPath.at(ourPath.length()) == QLatin1Char('/'))));
}


#ifndef QT_NO_DATASTREAM
/*! \relates QUrl

    Writes url \a url to the stream \a out and returns a reference
    to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator<<(QDataStream &out, const QUrl &url)
{
    QByteArray u = url.toEncoded();
    out << u;
    return out;
}

/*! \relates QUrl

    Reads a url into \a url from the stream \a in and returns a
    reference to the stream.

    \sa \link datastreamformat.html Format of the QDataStream operators \endlink
*/
QDataStream &operator>>(QDataStream &in, QUrl &url)
{
    QByteArray u;
    in >> u;
    url = QUrl(QString::fromUtf8(u));
    return in;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QUrl &url)
{
    d.maybeSpace() << "QUrl(" << url.toDisplayString() << ')';
    return d.space();
}
#endif

/*!
    \since 4.2

    Returns a text string that explains why an URL is invalid in the case being;
    otherwise returns an empty string.
*/
QString QUrl::errorString() const
{
    if (!d)
        return QLatin1String(QT_TRANSLATE_NOOP(QUrl, "Invalid URL \"\": ")); // XXX not a good message, but the one an empty URL produces
    return d->createErrorString();
}

/*!
    \typedef QUrl::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QUrl::data_ptr()
    \internal
*/

// The following code has the following copyright:
/*
   Copyright (C) Research In Motion Limited 2009. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Research In Motion Limited nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY Research In Motion Limited ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Research In Motion Limited BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


/*!
    Returns a valid URL from a user supplied \a userInput string if one can be
    deducted. In the case that is not possible, an invalid QUrl() is returned.

    \since 4.6

    Most applications that can browse the web, allow the user to input a URL
    in the form of a plain string. This string can be manually typed into
    a location bar, obtained from the clipboard, or passed in via command
    line arguments.

    When the string is not already a valid URL, a best guess is performed,
    making various web related assumptions.

    In the case the string corresponds to a valid file path on the system,
    a file:// URL is constructed, using QUrl::fromLocalFile().

    If that is not the case, an attempt is made to turn the string into a
    http:// or ftp:// URL. The latter in the case the string starts with
    'ftp'. The result is then passed through QUrl's tolerant parser, and
    in the case or success, a valid QUrl is returned, or else a QUrl().

    \section1 Examples:

    \list
    \li qt.nokia.com becomes http://qt.nokia.com
    \li ftp.qt.nokia.com becomes ftp://ftp.qt.nokia.com
    \li hostname becomes http://hostname
    \li /home/user/test.html becomes file:///home/user/test.html
    \endlist
*/
QUrl QUrl::fromUserInput(const QString &userInput)
{
    QString trimmedString = userInput.trimmed();

    // Check first for files, since on Windows drive letters can be interpretted as schemes
    if (QDir::isAbsolutePath(trimmedString))
        return QUrl::fromLocalFile(trimmedString);

    QUrl url(trimmedString, QUrl::TolerantMode);
    QUrl urlPrepended(QString::fromLatin1("http://") + trimmedString, QUrl::TolerantMode);

    // Check the most common case of a valid url with scheme and host
    // We check if the port would be valid by adding the scheme to handle the case host:port
    // where the host would be interpretted as the scheme
    if (url.isValid()
        && !url.scheme().isEmpty()
        && (!url.host().isEmpty() || !url.path().isEmpty())
        && urlPrepended.port() == -1)
        return url;

    // Else, try the prepended one and adjust the scheme from the host name
    if (urlPrepended.isValid() && (!urlPrepended.host().isEmpty() || !urlPrepended.path().isEmpty()))
    {
        int dotIndex = trimmedString.indexOf(QLatin1Char('.'));
        const QString hostscheme = trimmedString.left(dotIndex).toLower();
        if (hostscheme == QLatin1String("ftp"))
            urlPrepended.setScheme(QLatin1String("ftp"));
        return urlPrepended;
    }

    return QUrl();
}
// end of BSD code

QT_END_NAMESPACE
