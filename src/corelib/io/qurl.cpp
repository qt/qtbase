// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

/*!
    \class QUrl
    \inmodule QtCore

    \brief The QUrl class provides a convenient interface for working
    with URLs.

    \reentrant
    \ingroup io
    \ingroup network
    \ingroup shared

    \compares weak

    It can parse and construct URLs in both encoded and unencoded
    form. QUrl also has support for internationalized domain names
    (IDNs).

    The most common way to use QUrl is to initialize it via the constructor by
    passing a QString containing a full URL. QUrl objects can also be created
    from a QByteArray containing a full URL using QUrl::fromEncoded(), or
    heuristically from incomplete URLs using QUrl::fromUserInput(). The URL
    representation can be obtained from a QUrl using either QUrl::toString() or
    QUrl::toEncoded().

    URLs can be represented in two forms: encoded or unencoded. The
    unencoded representation is suitable for showing to users, but
    the encoded representation is typically what you would send to
    a web server. For example, the unencoded URL
    "http://bühler.example.com/List of applicants.xml"
    would be sent to the server as
    "http://xn--bhler-kva.example.com/List%20of%20applicants.xml".

    A URL can also be constructed piece by piece by calling
    setScheme(), setUserName(), setPassword(), setHost(), setPort(),
    setPath(), setQuery() and setFragment(). Some convenience
    functions are also available: setAuthority() sets the user name,
    password, host and port. setUserInfo() sets the user name and
    password at once.

    Call isValid() to check if the URL is valid. This can be done at any point
    during the constructing of a URL. If isValid() returns \c false, you should
    clear() the URL before proceeding, or start over by parsing a new URL with
    setUrl().

    Constructing a query is particularly convenient through the use of the \l
    QUrlQuery class and its methods QUrlQuery::setQueryItems(),
    QUrlQuery::addQueryItem() and QUrlQuery::removeQueryItem(). Use
    QUrlQuery::setQueryDelimiters() to customize the delimiters used for
    generating the query string.

    For the convenience of generating encoded URL strings or query
    strings, there are two static functions called
    fromPercentEncoding() and toPercentEncoding() which deal with
    percent encoding and decoding of QString objects.

    fromLocalFile() constructs a QUrl by parsing a local
    file path. toLocalFile() converts a URL to a local file path.

    The human readable representation of the URL is fetched with
    toString(). This representation is appropriate for displaying a
    URL to a user in unencoded form. The encoded form however, as
    returned by toEncoded(), is for internal use, passing to web
    servers, mail clients and so on. Both forms are technically correct
    and represent the same URL unambiguously -- in fact, passing either
    form to QUrl's constructor or to setUrl() will yield the same QUrl
    object.

    QUrl conforms to the URI specification from
    \l{RFC 3986} (Uniform Resource Identifier: Generic Syntax), and includes
    scheme extensions from \l{RFC 1738} (Uniform Resource Locators). Case
    folding rules in QUrl conform to \l{RFC 3491} (Nameprep: A Stringprep
    Profile for Internationalized Domain Names (IDN)). It is also compatible with the
    \l{http://freedesktop.org/wiki/Specifications/file-uri-spec/}{file URI specification}
    from freedesktop.org, provided that the locale encodes file names using
    UTF-8 (required by IDN).

    \section2 Relative URLs vs Relative Paths

    Calling isRelative() will return whether or not the URL is relative.
    A relative URL has no \l {scheme}. For example:

    \snippet code/src_corelib_io_qurl.cpp 8

    Notice that a URL can be absolute while containing a relative path, and
    vice versa:

    \snippet code/src_corelib_io_qurl.cpp 9

    A relative URL can be resolved by passing it as an argument to resolved(),
    which returns an absolute URL. isParentOf() is used for determining whether
    one URL is a parent of another.

    \section2 Error checking

    QUrl is capable of detecting many errors in URLs while parsing it or when
    components of the URL are set with individual setter methods (like
    setScheme(), setHost() or setPath()). If the parsing or setter function is
    successful, any previously recorded error conditions will be discarded.

    By default, QUrl setter methods operate in QUrl::TolerantMode, which means
    they accept some common mistakes and mis-representation of data. An
    alternate method of parsing is QUrl::StrictMode, which applies further
    checks. See QUrl::ParsingMode for a description of the difference of the
    parsing modes.

    QUrl only checks for conformance with the URL specification. It does not
    try to verify that high-level protocol URLs are in the format they are
    expected to be by handlers elsewhere. For example, the following URIs are
    all considered valid by QUrl, even if they do not make sense when used:

    \list
      \li "http:/filename.html"
      \li "mailto://example.com"
    \endlist

    When the parser encounters an error, it signals the event by making
    isValid() return false and toString() / toEncoded() return an empty string.
    If it is necessary to show the user the reason why the URL failed to parse,
    the error condition can be obtained from QUrl by calling errorString().
    Note that this message is highly technical and may not make sense to
    end-users.

    QUrl is capable of recording only one error condition. If more than one
    error is found, it is undefined which error is reported.

    \section2 Character Conversions

    Follow these rules to avoid erroneous character conversion when
    dealing with URLs and strings:

    \list
    \li When creating a QString to contain a URL from a QByteArray or a
       char*, always use QString::fromUtf8().
    \endlist
*/

/*!
    \enum QUrl::ParsingMode

    The parsing mode controls the way QUrl parses strings.

    \value TolerantMode QUrl will try to correct some common errors in URLs.
                        This mode is useful for parsing URLs coming from sources
                        not known to be strictly standards-conforming.

    \value StrictMode Only valid URLs are accepted. This mode is useful for
                      general URL validation.

    \value DecodedMode QUrl will interpret the URL component in the fully-decoded form,
                       where percent characters stand for themselves, not as the beginning
                       of a percent-encoded sequence. This mode is only valid for the
                       setters setting components of a URL; it is not permitted in
                       the QUrl constructor, in fromEncoded() or in setUrl().
                       For more information on this mode, see the documentation for
                       \l {QUrl::ComponentFormattingOption}{QUrl::FullyDecoded}.

    In TolerantMode, the parser has the following behaviour:

    \list

    \li Spaces and "%20": unencoded space characters will be accepted and will
    be treated as equivalent to "%20".

    \li Single "%" characters: Any occurrences of a percent character "%" not
    followed by exactly two hexadecimal characters (e.g., "13% coverage.html")
    will be replaced by "%25". Note that one lone "%" character will trigger
    the correction mode for all percent characters.

    \li Reserved and unreserved characters: An encoded URL should only
    contain a few characters as literals; all other characters should
    be percent-encoded. In TolerantMode, these characters will be
    accepted if they are found in the URL:
            space / double-quote / "<" / ">" / "\" /
            "^" / "`" / "{" / "|" / "}"
    Those same characters can be decoded again by passing QUrl::DecodeReserved
    to toString() or toEncoded(). In the getters of individual components,
    those characters are often returned in decoded form.

    \endlist

    When in StrictMode, if a parsing error is found, isValid() will return \c
    false and errorString() will return a message describing the error.
    If more than one error is detected, it is undefined which error gets
    reported.

    Note that TolerantMode is not usually enough for parsing user input, which
    often contains more errors and expectations than the parser can deal with.
    When dealing with data coming directly from the user -- as opposed to data
    coming from data-transfer sources, such as other programs -- it is
    recommended to use fromUserInput().

    \sa fromUserInput(), setUrl(), toString(), toEncoded(), QUrl::FormattingOptions
*/

/*!
    \enum QUrl::UrlFormattingOption

    The formatting options define how the URL is formatted when written out
    as text.

    \value None The format of the URL is unchanged.
    \value RemoveScheme  The scheme is removed from the URL.
    \value RemovePassword  Any password in the URL is removed.
    \value RemoveUserInfo  Any user information in the URL is removed.
    \value RemovePort      Any specified port is removed from the URL.
    \value RemoveAuthority  Remove user name, password, host and port.
    \value RemovePath   The URL's path is removed, leaving only the scheme,
                        host address, and port (if present).
    \value RemoveQuery  The query part of the URL (following a '?' character)
                        is removed.
    \value RemoveFragment The fragment part of the URL (including the '#' character) is removed.
    \value RemoveFilename The filename (i.e. everything after the last '/' in the path) is removed.
            The trailing '/' is kept, unless StripTrailingSlash is set.
            Only valid if RemovePath is not set.
    \value PreferLocalFile If the URL is a local file according to isLocalFile()
     and contains no query or fragment, a local file path is returned.
    \value StripTrailingSlash  The trailing slash is removed from the path, if one is present.
    \value NormalizePathSegments  Modifies the path to remove redundant directory separators,
             and to resolve "."s and ".."s (as far as possible). For non-local paths, adjacent
             slashes are preserved.

    Note that the case folding rules in \l{RFC 3491}{Nameprep}, which QUrl
    conforms to, require host names to always be converted to lower case,
    regardless of the Qt::FormattingOptions used.

    The options from QUrl::ComponentFormattingOptions are also possible.

    \sa QUrl::ComponentFormattingOptions
*/

/*!
    \enum QUrl::ComponentFormattingOption
    \since 5.0

    The component formatting options define how the components of an URL will
    be formatted when written out as text. They can be combined with the
    options from QUrl::FormattingOptions when used in toString() and
    toEncoded().

    \value PrettyDecoded   The component is returned in a "pretty form", with
                           most percent-encoded characters decoded. The exact
                           behavior of PrettyDecoded varies from component to
                           component and may also change from Qt release to Qt
                           release. This is the default.

    \value EncodeSpaces    Leave space characters in their encoded form ("%20").

    \value EncodeUnicode   Leave non-US-ASCII characters encoded in their UTF-8
                           percent-encoded form (e.g., "%C3%A9" for the U+00E9
                           codepoint, LATIN SMALL LETTER E WITH ACUTE).

    \value EncodeDelimiters Leave certain delimiters in their encoded form, as
                            would appear in the URL when the full URL is
                            represented as text. The delimiters are affected
                            by this option change from component to component.
                            This flag has no effect in toString() or toEncoded().

    \value EncodeReserved  Leave US-ASCII characters not permitted in the URL by
                           the specification in their encoded form. This is the
                           default on toString() and toEncoded().

    \value DecodeReserved  Decode the US-ASCII characters that the URL specification
                           does not allow to appear in the URL. This is the
                           default on the getters of individual components.

    \value FullyEncoded    Leave all characters in their properly-encoded form,
                           as this component would appear as part of a URL. When
                           used with toString(), this produces a fully-compliant
                           URL in QString form, exactly equal to the result of
                           toEncoded()

    \value FullyDecoded    Attempt to decode as much as possible. For individual
                           components of the URL, this decodes every percent
                           encoding sequence, including control characters (U+0000
                           to U+001F) and UTF-8 sequences found in percent-encoded form.
                           Use of this mode may cause data loss, see below for more information.

    The values of EncodeReserved and DecodeReserved should not be used together
    in one call. The behavior is undefined if that happens. They are provided
    as separate values because the behavior of the "pretty mode" with regards
    to reserved characters is different on certain components and specially on
    the full URL.

    \section2 Full decoding

    The FullyDecoded mode is similar to the behavior of the functions returning
    QString in Qt 4.x, in that every character represents itself and never has
    any special meaning. This is true even for the percent character ('%'),
    which should be interpreted to mean a literal percent, not the beginning of
    a percent-encoded sequence. The same actual character, in all other
    decoding modes, is represented by the sequence "%25".

    Whenever re-applying data obtained with QUrl::FullyDecoded into a QUrl,
    care must be taken to use the QUrl::DecodedMode parameter to the setters
    (like setPath() and setUserName()). Failure to do so may cause
    re-interpretation of the percent character ('%') as the beginning of a
    percent-encoded sequence.

    This mode is quite useful when portions of a URL are used in a non-URL
    context. For example, to extract the username, password or file paths in an
    FTP client application, the FullyDecoded mode should be used.

    This mode should be used with care, since there are two conditions that
    cannot be reliably represented in the returned QString. They are:

    \list
      \li \b{Non-UTF-8 sequences:} URLs may contain sequences of
      percent-encoded characters that do not form valid UTF-8 sequences. Since
      URLs need to be decoded using UTF-8, any decoder failure will result in
      the QString containing one or more replacement characters where the
      sequence existed.

      \li \b{Encoded delimiters:} URLs are also allowed to make a distinction
      between a delimiter found in its literal form and its equivalent in
      percent-encoded form. This is most commonly found in the query, but is
      permitted in most parts of the URL.
    \endlist

    The following example illustrates the problem:

    \snippet code/src_corelib_io_qurl.cpp 10

    If the two URLs were used via HTTP GET, the interpretation by the web
    server would probably be different. In the first case, it would interpret
    as one parameter, with a key of "q" and value "a+=b&c". In the second
    case, it would probably interpret as two parameters, one with a key of "q"
    and value "a =b", and the second with a key "c" and no value.

    \sa QUrl::FormattingOptions
*/

/*!
    \enum QUrl::UserInputResolutionOption
    \since 5.4

    The user input resolution options define how fromUserInput() should
    interpret strings that could either be a relative path or the short
    form of a HTTP URL. For instance \c{file.pl} can be either a local file
    or the URL \c{http://file.pl}.

    \value DefaultResolution  The default resolution mechanism is to check
                              whether a local file exists, in the working
                              directory given to fromUserInput, and only
                              return a local path in that case. Otherwise a URL
                              is assumed.
    \value AssumeLocalFile    This option makes fromUserInput() always return
                              a local path unless the input contains a scheme, such as
                              \c{http://file.pl}. This is useful for applications
                              such as text editors, which are able to create
                              the file if it doesn't exist.

    \sa fromUserInput()
*/

/*!
    \enum  QUrl::AceProcessingOption
    \since 6.3

    The ACE processing options control the way URLs are transformed to and from
    ASCII-Compatible Encoding.

    \value IgnoreIDNWhitelist         Ignore the IDN whitelist when converting URLs
                                      to Unicode.
    \value AceTransitionalProcessing  Use transitional processing described in UTS #46.
                                      This allows better compatibility with IDNA 2003
                                      specification.

    The default is to use nontransitional processing and to allow non-ASCII
    characters only inside URLs whose top-level domains are listed in the IDN whitelist.

    \sa toAce(), fromAce(), idnWhitelist()
*/

/*!
    \fn QUrl::QUrl(QUrl &&other)

    Move-constructs a QUrl instance, making it point at the same
    object that \a other was pointing to.

    \since 5.2
*/

/*!
    \fn QUrl &QUrl::operator=(QUrl &&other)

    Move-assigns \a other to this QUrl instance.

    \since 5.2
*/

#include "qurl.h"
#include "qurl_p.h"
#include "qplatformdefs.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qdebug.h"
#include "qhash.h"
#include "qdatastream.h"
#include "private/qipaddress_p.h"
#include "qurlquery.h"
#include "private/qdir_p.h"
#include <private/qtools_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtMiscUtils;

inline static bool isHex(char c)
{
    c |= 0x20;
    return isAsciiDigit(c) || (c >= 'a' && c <= 'f');
}

static inline QString ftpScheme()
{
    return QStringLiteral("ftp");
}

static inline QString fileScheme()
{
    return QStringLiteral("file");
}

static inline QString webDavScheme()
{
    return QStringLiteral("webdavs");
}

static inline QString webDavSslTag()
{
    return QStringLiteral("@SSL");
}

class QUrlPrivate
{
public:
    enum Section : uchar {
        Scheme = 0x01,
        UserName = 0x02,
        Password = 0x04,
        UserInfo = UserName | Password,
        Host = 0x08,
        Port = 0x10,
        Authority = UserInfo | Host | Port,
        Path = 0x20,
        Hierarchy = Authority | Path,
        Query = 0x40,
        Fragment = 0x80,
        FullUrl = 0xff
    };

    enum Flags : uchar {
        IsLocalFile = 0x01
    };

    enum ErrorCode {
        // the high byte of the error code matches the Section
        // the first item in each value must be the generic "Invalid xxx Error"
        InvalidSchemeError = Scheme << 8,

        InvalidUserNameError = UserName << 8,

        InvalidPasswordError = Password << 8,

        InvalidRegNameError = Host << 8,
        InvalidIPv4AddressError,
        InvalidIPv6AddressError,
        InvalidCharacterInIPv6Error,
        InvalidIPvFutureError,
        HostMissingEndBracket,

        InvalidPortError = Port << 8,
        PortEmptyError,

        InvalidPathError = Path << 8,

        InvalidQueryError = Query << 8,

        InvalidFragmentError = Fragment << 8,

        // the following three cases are only possible in combination with
        // presence/absence of the path, authority and scheme. See validityError().
        AuthorityPresentAndPathIsRelative = Authority << 8 | Path << 8 | 0x10000,
        AuthorityAbsentAndPathIsDoubleSlash,
        RelativeUrlPathContainsColonBeforeSlash = Scheme << 8 | Authority << 8 | Path << 8 | 0x10000,

        NoError = 0
    };

    struct Error {
        QString source;
        qsizetype position;
        ErrorCode code;
    };

    QUrlPrivate();
    QUrlPrivate(const QUrlPrivate &copy);
    ~QUrlPrivate();

    void parse(const QString &url, QUrl::ParsingMode parsingMode);
    bool isEmpty() const
    { return sectionIsPresent == 0 && port == -1 && path.isEmpty(); }

    std::unique_ptr<Error> cloneError() const;
    void clearError();
    void setError(ErrorCode errorCode, const QString &source, qsizetype supplement = -1);
    ErrorCode validityError(QString *source = nullptr, qsizetype *position = nullptr) const;
    bool validateComponent(Section section, const QString &input, qsizetype begin, qsizetype end);
    bool validateComponent(Section section, const QString &input)
    { return validateComponent(section, input, 0, input.size()); }

    // no QString scheme() const;
    void appendAuthority(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendUserInfo(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendUserName(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPassword(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendHost(QString &appendTo, QUrl::FormattingOptions options) const;
    void appendPath(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendQuery(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;
    void appendFragment(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const;

    // the "end" parameters are like STL iterators: they point to one past the last valid element
    bool setScheme(const QString &value, qsizetype len, bool doSetError);
    void setAuthority(const QString &auth, qsizetype from, qsizetype end, QUrl::ParsingMode mode);
    template <typename String> void setUserInfo(String &&value, QUrl::ParsingMode mode);
    template <typename String> void setUserName(String &&value, QUrl::ParsingMode mode);
    template <typename String> void setPassword(String &&value, QUrl::ParsingMode mode);
    bool setHost(const QString &value, qsizetype from, qsizetype end, QUrl::ParsingMode mode);
    template <typename String> void setPath(String &&value, QUrl::ParsingMode mode);
    template <typename String> void setQuery(String &&value, QUrl::ParsingMode mode);
    template <typename String> void setFragment(String &&value, QUrl::ParsingMode mode);

    uint presentSections() const noexcept
    {
        uint s = sectionIsPresent;

        // We have to ignore the host-is-present flag for local files (the
        // "file" protocol), due to the requirements of the XDG file URI
        // specification.
        if (isLocalFile())
            s &= ~Host;

        // If the password was set, we must have a username too
        if (s & Password)
            s |= UserName;

        return s;
    }

    inline bool hasScheme() const { return sectionIsPresent & Scheme; }
    inline bool hasAuthority() const { return sectionIsPresent & Authority; }
    inline bool hasUserInfo() const { return sectionIsPresent & UserInfo; }
    inline bool hasUserName() const { return sectionIsPresent & UserName; }
    inline bool hasPassword() const { return sectionIsPresent & Password; }
    inline bool hasHost() const { return sectionIsPresent & Host; }
    inline bool hasPort() const { return port != -1; }
    inline bool hasPath() const { return !path.isEmpty(); }
    inline bool hasQuery() const { return sectionIsPresent & Query; }
    inline bool hasFragment() const { return sectionIsPresent & Fragment; }

    inline bool isLocalFile() const { return flags & IsLocalFile; }
    QString toLocalFile(QUrl::FormattingOptions options) const;

    bool normalizePathSegments(QString *path) const
    {
        QDirPrivate::PathNormalizations mode = QDirPrivate::UrlNormalizationMode;
        if (!isLocalFile())
            mode |= QDirPrivate::RemotePath;
        return qt_normalizePathSegments(path, mode);
    }
    QString mergePaths(const QString &relativePath) const;

    void clear()
    {
        clearError();
        scheme = userName = password = host = path = query = fragment = QString();
        port = -1;
        sectionIsPresent = 0;
        flags = 0;
    }

    QAtomicInt ref;
    int port;

    QString scheme;
    QString userName;
    QString password;
    QString host;
    QString path;
    QString query;
    QString fragment;

    std::unique_ptr<Error> error;

    // not used for:
    //  - Port (port == -1 means absence)
    //  - Path (there's no path delimiter, so we optimize its use out of existence)
    // Schemes are never supposed to be empty, but we keep the flag anyway
    uchar sectionIsPresent;
    uchar flags;

    // 32-bit: 2 bytes tail padding available
    // 64-bit: 6 bytes tail padding available
};

inline QUrlPrivate::QUrlPrivate()
    : ref(1), port(-1),
      sectionIsPresent(0),
      flags(0)
{
}

inline QUrlPrivate::QUrlPrivate(const QUrlPrivate &copy)
    : ref(1), port(copy.port),
      scheme(copy.scheme),
      userName(copy.userName),
      password(copy.password),
      host(copy.host),
      path(copy.path),
      query(copy.query),
      fragment(copy.fragment),
      error(copy.cloneError()),
      sectionIsPresent(copy.sectionIsPresent),
      flags(copy.flags)
{
}

inline QUrlPrivate::~QUrlPrivate()
    = default;

std::unique_ptr<QUrlPrivate::Error> QUrlPrivate::cloneError() const
{
    return error ? std::make_unique<Error>(*error) : nullptr;
}

inline void QUrlPrivate::clearError()
{
    error.reset();
}

inline void QUrlPrivate::setError(ErrorCode errorCode, const QString &source, qsizetype supplement)
{
    if (error) {
        // don't overwrite an error set in a previous section during parsing
        return;
    }
    error = std::make_unique<Error>();
    error->code = errorCode;
    error->source = source;
    error->position = supplement;
}

// From RFC 3986, Appendix A Collected ABNF for URI
//    URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
//[...]
//    scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
//
//    authority     = [ userinfo "@" ] host [ ":" port ]
//    userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
//    host          = IP-literal / IPv4address / reg-name
//    port          = *DIGIT
//[...]
//    reg-name      = *( unreserved / pct-encoded / sub-delims )
//[..]
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
// the path component has a complex ABNF that basically boils down to
// slash-separated segments of "pchar"

// The above is the strict definition of the URL components and we mostly
// adhere to it, with few exceptions. QUrl obeys the following behavior:
//  - percent-encoding sequences always use uppercase HEXDIG;
//  - unreserved characters are *always* decoded, no exceptions;
//  - the space character and bytes with the high bit set are controlled by
//    the EncodeSpaces and EncodeUnicode bits;
//  - control characters, the percent sign itself, and bytes with the high
//    bit set that don't form valid UTF-8 sequences are always encoded,
//    except in FullyDecoded mode;
//  - sub-delims are always left alone, except in FullyDecoded mode;
//  - gen-delim change behavior depending on which section of the URL (or
//    the entire URL) we're looking at; see below;
//  - characters not mentioned above, like "<", and ">", are usually
//    decoded in individual sections of the URL, but encoded when the full
//    URL is put together (we can change on subjective definition of
//    "pretty").
//
// The behavior for the delimiters bears some explanation. The spec says in
// section 2.2:
//     URIs that differ in the replacement of a reserved character with its
//     corresponding percent-encoded octet are not equivalent.
// (note: QUrl API mistakenly uses the "reserved" term, so we will refer to
// them here as "delimiters").
//
// For that reason, we cannot encode delimiters found in decoded form and we
// cannot decode the ones found in encoded form if that would change the
// interpretation. Conversely, we *can* perform the transformation if it would
// not change the interpretation. From the last component of a URL to the first,
// here are the gen-delims we can unambiguously transform when the field is
// taken in isolation:
//  - fragment: none, since it's the last
//  - query: "#" is unambiguous
//  - path: "#" and "?" are unambiguous
//  - host: completely special but never ambiguous, see setHost() below.
//  - password: the "#", "?", "/", "[", "]" and "@" characters are unambiguous
//  - username: the "#", "?", "/", "[", "]", "@", and ":" characters are unambiguous
//  - scheme: doesn't accept any delimiter, see setScheme() below.
//
// Internally, QUrl stores each component in the format that corresponds to the
// default mode (PrettyDecoded). It deviates from the "strict" FullyEncoded
// mode in the following way:
//  - spaces are decoded
//  - valid UTF-8 sequences are decoded
//  - gen-delims that can be unambiguously transformed are decoded (exception:
//    square brackets in path, query and fragment are left as they were)
//  - characters controlled by DecodeReserved are often decoded, though this behavior
//    can change depending on the subjective definition of "pretty"
//
// Note that the list of gen-delims that we can transform is different for the
// user info (user name + password) and the authority (user info + host +
// port).


// list the recoding table modifications to be used with the recodeFromUser and
// appendToUser functions, according to the rules above. Spaces and UTF-8
// sequences are handled outside the tables.

// the encodedXXX tables are run with the delimiters set to "leave" by default;
// the decodedXXX tables are run with the delimiters set to "decode" by default
// (except for the query, which doesn't use these functions)

namespace {
template <typename T> constexpr ushort decode(T x) noexcept { return ushort(x); }
template <typename T> constexpr ushort leave(T x) noexcept { return ushort(0x100 | x); }
template <typename T> constexpr ushort encode(T x) noexcept { return ushort(0x200 | x); }
}

static const ushort userNameInIsolation[] = {
    decode(':'), // 0
    decode('@'), // 1
    decode(']'), // 2
    decode('['), // 3
    decode('/'), // 4
    decode('?'), // 5
    decode('#'), // 6

    decode('"'), // 7
    decode('<'),
    decode('>'),
    decode('^'),
    decode('\\'),
    decode('|'),
    decode('{'),
    decode('}'),
    0
};
static const ushort * const passwordInIsolation = userNameInIsolation + 1;
static const ushort * const pathInIsolation = userNameInIsolation + 5;
static const ushort * const queryInIsolation = userNameInIsolation + 6;
static const ushort * const fragmentInIsolation = userNameInIsolation + 7;

static const ushort userNameInUserInfo[] =  {
    encode(':'), // 0
    decode('@'), // 1
    decode(']'), // 2
    decode('['), // 3
    decode('/'), // 4
    decode('?'), // 5
    decode('#'), // 6

    decode('"'), // 7
    decode('<'),
    decode('>'),
    decode('^'),
    decode('\\'),
    decode('|'),
    decode('{'),
    decode('}'),
    0
};
static const ushort * const passwordInUserInfo = userNameInUserInfo + 1;

static const ushort userNameInAuthority[] = {
    encode(':'), // 0
    encode('@'), // 1
    encode(']'), // 2
    encode('['), // 3
    decode('/'), // 4
    decode('?'), // 5
    decode('#'), // 6

    decode('"'), // 7
    decode('<'),
    decode('>'),
    decode('^'),
    decode('\\'),
    decode('|'),
    decode('{'),
    decode('}'),
    0
};
static const ushort * const passwordInAuthority = userNameInAuthority + 1;

static const ushort userNameInUrl[] = {
    encode(':'), // 0
    encode('@'), // 1
    encode(']'), // 2
    encode('['), // 3
    encode('/'), // 4
    encode('?'), // 5
    encode('#'), // 6

    // no need to list encode(x) for the other characters
    0
};
static const ushort * const passwordInUrl = userNameInUrl + 1;
static const ushort * const pathInUrl = userNameInUrl + 5;
static const ushort * const queryInUrl = userNameInUrl + 6;
static const ushort * const fragmentInUrl = userNameInUrl + 6;

static void
recodeFromUser(QString &output, const QString &input, const ushort *actions, QUrl::ParsingMode mode)
{
    output.resize(0);
    qsizetype appended;
    if (mode == QUrl::DecodedMode)
        appended = qt_encodeFromUser(output, input, actions);
    else
        appended = qt_urlRecode(output, input, {}, actions);
    if (!appended)
        output = input;
}

static void
recodeFromUser(QString &output, QStringView input, const ushort *actions, QUrl::ParsingMode mode)
{
    Q_ASSERT_X(mode != QUrl::DecodedMode, "recodeFromUser",
               "This function should only be called when parsing encoded components");
    Q_UNUSED(mode);
    output.resize(0);
    if (qt_urlRecode(output, input, {}, actions))
        return;
    output.append(input);
}

// appendXXXX functions: copy from the internal form to the external, user form.
// the internal value is stored in its PrettyDecoded form, so that case is easy.
static inline void appendToUser(QString &appendTo, QStringView value, QUrl::FormattingOptions options,
                                const ushort *actions)
{
    // The stored value is already QUrl::PrettyDecoded, so there's nothing to
    // do if that's what the user asked for (test only
    // ComponentFormattingOptions, ignore FormattingOptions).
    if ((options & 0xFFFF0000) == QUrl::PrettyDecoded ||
            !qt_urlRecode(appendTo, value, options, actions))
        appendTo += value;

    // copy nullness, if necessary, because QString::operator+=(QStringView) doesn't
    if (appendTo.isNull() && !value.isNull())
        appendTo.detach();
}

inline void QUrlPrivate::appendAuthority(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    if ((options & QUrl::RemoveUserInfo) != QUrl::RemoveUserInfo) {
        appendUserInfo(appendTo, options, appendingTo);

        // add '@' only if we added anything
        if (hasUserName() || (hasPassword() && (options & QUrl::RemovePassword) == 0))
            appendTo += u'@';
    }
    appendHost(appendTo, options);
    if (!(options & QUrl::RemovePort) && port != -1)
        appendTo += u':' + QString::number(port);
}

inline void QUrlPrivate::appendUserInfo(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    if (Q_LIKELY(!hasUserInfo()))
        return;

    const ushort *userNameActions;
    const ushort *passwordActions;
    if (options & QUrl::EncodeDelimiters) {
        userNameActions = userNameInUrl;
        passwordActions = passwordInUrl;
    } else {
        switch (appendingTo) {
        case UserInfo:
            userNameActions = userNameInUserInfo;
            passwordActions = passwordInUserInfo;
            break;

        case Authority:
            userNameActions = userNameInAuthority;
            passwordActions = passwordInAuthority;
            break;

        case FullUrl:
            userNameActions = userNameInUrl;
            passwordActions = passwordInUrl;
            break;

        default:
            // can't happen
            Q_UNREACHABLE();
            break;
        }
    }

    if (!qt_urlRecode(appendTo, userName, options, userNameActions))
        appendTo += userName;
    if (options & QUrl::RemovePassword || !hasPassword()) {
        return;
    } else {
        appendTo += u':';
        if (!qt_urlRecode(appendTo, password, options, passwordActions))
            appendTo += password;
    }
}

inline void QUrlPrivate::appendUserName(QString &appendTo, QUrl::FormattingOptions options) const
{
    // only called from QUrl::userName()
    appendToUser(appendTo, userName, options,
                 options & QUrl::EncodeDelimiters ? userNameInUrl : userNameInIsolation);
    if (appendTo.isNull() && hasPassword())
        appendTo.detach();      // the presence of password implies presence of username
}

inline void QUrlPrivate::appendPassword(QString &appendTo, QUrl::FormattingOptions options) const
{
    // only called from QUrl::password()
    appendToUser(appendTo, password, options,
                 options & QUrl::EncodeDelimiters ? passwordInUrl : passwordInIsolation);
}

inline void QUrlPrivate::appendPath(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    QString thePath = path;
    if (options & QUrl::NormalizePathSegments)
        normalizePathSegments(&thePath);

    QStringView thePathView(thePath);
    if (options & QUrl::RemoveFilename) {
        const qsizetype slash = thePathView.lastIndexOf(u'/');
        if (slash == -1)
            return;
        thePathView = thePathView.left(slash + 1);
    }
    // check if we need to remove trailing slashes
    if (options & QUrl::StripTrailingSlash) {
        while (thePathView.size() > 1 && thePathView.endsWith(u'/'))
            thePathView.chop(1);
    }

    appendToUser(appendTo, thePathView, options,
                 appendingTo == FullUrl || options & QUrl::EncodeDelimiters ? pathInUrl : pathInIsolation);
}

inline void QUrlPrivate::appendFragment(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    appendToUser(appendTo, fragment, options,
                 options & QUrl::EncodeDelimiters ? fragmentInUrl :
                 appendingTo == FullUrl ? nullptr : fragmentInIsolation);
}

inline void QUrlPrivate::appendQuery(QString &appendTo, QUrl::FormattingOptions options, Section appendingTo) const
{
    appendToUser(appendTo, query, options,
                 appendingTo == FullUrl || options & QUrl::EncodeDelimiters ? queryInUrl : queryInIsolation);
}

// setXXX functions

inline bool QUrlPrivate::setScheme(const QString &value, qsizetype len, bool doSetError)
{
    // schemes are strictly RFC-compliant:
    //    scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    // we also lowercase the scheme

    // schemes in URLs are not allowed to be empty, but they can be in
    // "Relative URIs" which QUrl also supports. QUrl::setScheme does
    // not call us with len == 0, so this can only be from parse()
    scheme.clear();
    if (len == 0)
        return false;

    sectionIsPresent |= Scheme;

    // validate it:
    qsizetype needsLowercasing = -1;
    const ushort *p = reinterpret_cast<const ushort *>(value.data());
    for (qsizetype i = 0; i < len; ++i) {
        if (isAsciiLower(p[i]))
            continue;
        if (isAsciiUpper(p[i])) {
            needsLowercasing = i;
            continue;
        }
        if (i) {
            if (isAsciiDigit(p[i]))
                continue;
            if (p[i] == '+' || p[i] == '-' || p[i] == '.')
                continue;
        }

        // found something else
        // don't call setError needlessly:
        // if we've been called from parse(), it will try to recover
        if (doSetError)
            setError(InvalidSchemeError, value, i);
        return false;
    }

    scheme = value.left(len);

    if (needsLowercasing != -1) {
        // schemes are ASCII only, so we don't need the full Unicode toLower
        QChar *schemeData = scheme.data(); // force detaching here
        for (qsizetype i = needsLowercasing; i >= 0; --i) {
            ushort c = schemeData[i].unicode();
            if (isAsciiUpper(c))
                schemeData[i] = QChar(c + 0x20);
        }
    }

    // did we set to the file protocol?
    if (scheme == fileScheme()
#ifdef Q_OS_WIN
        || scheme == webDavScheme()
#endif
       ) {
        flags |= IsLocalFile;
    } else {
        flags &= ~IsLocalFile;
    }
    return true;
}

inline void QUrlPrivate::setAuthority(const QString &auth, qsizetype from, qsizetype end, QUrl::ParsingMode mode)
{
    Q_ASSERT_X(mode != QUrl::DecodedMode, "setAuthority",
               "This function should only be called when parsing encoded components");
    sectionIsPresent &= ~Authority;
    port = -1;
    if (from == end && !auth.isNull())
        sectionIsPresent |= Host;   // empty but not null authority implies host

    // we never actually _loop_
    while (from != end) {
        qsizetype userInfoIndex = auth.indexOf(u'@', from);
        if (size_t(userInfoIndex) < size_t(end)) {
            setUserInfo(QStringView(auth).sliced(from, userInfoIndex - from), mode);
            if (mode == QUrl::StrictMode && !validateComponent(UserInfo, auth, from, userInfoIndex))
                break;
            from = userInfoIndex + 1;
        }

        qsizetype colonIndex = auth.lastIndexOf(u':', end - 1);
        if (colonIndex < from)
            colonIndex = -1;

        if (size_t(colonIndex) < size_t(end)) {
            if (auth.at(from).unicode() == '[') {
                // check if colonIndex isn't inside the "[...]" part
                qsizetype closingBracket = auth.indexOf(u']', from);
                if (size_t(closingBracket) > size_t(colonIndex))
                    colonIndex = -1;
            }
        }

        if (size_t(colonIndex) < size_t(end) - 1) {
            // found a colon with digits after it
            unsigned long x = 0;
            for (qsizetype i = colonIndex + 1; i < end; ++i) {
                ushort c = auth.at(i).unicode();
                if (isAsciiDigit(c)) {
                    x *= 10;
                    x += c - '0';
                } else {
                    x = ulong(-1); // x != ushort(x)
                    break;
                }
            }
            if (x == ushort(x)) {
                port = ushort(x);
            } else {
                setError(InvalidPortError, auth, colonIndex + 1);
                if (mode == QUrl::StrictMode)
                    break;
            }
        }

        setHost(auth, from, qMin<size_t>(end, colonIndex), mode);
        if (mode == QUrl::StrictMode && !validateComponent(Host, auth, from, qMin<size_t>(end, colonIndex))) {
            // clear host too
            sectionIsPresent &= ~Authority;
            break;
        }

        // success
        return;
    }
    // clear all sections but host
    sectionIsPresent &= ~Authority | Host;
    userName.clear();
    password.clear();
    host.clear();
    port = -1;
}

template <typename String> void QUrlPrivate::setUserInfo(String &&value, QUrl::ParsingMode mode)
{
    Q_ASSERT_X(mode != QUrl::DecodedMode, "setUserInfo",
               "This function should only be called when parsing encoded components");
    qsizetype delimIndex = value.indexOf(u':');
    if (delimIndex < 0) {
        // no password
        setUserName(std::move(value), mode);
        password.clear();
        sectionIsPresent &= ~Password;
    } else {
        setUserName(value.first(delimIndex), mode);
        setPassword(value.sliced(delimIndex + 1), mode);
    }
}

template <typename String> inline void QUrlPrivate::setUserName(String &&value, QUrl::ParsingMode mode)
{
    sectionIsPresent |= UserName;
    recodeFromUser(userName, value, userNameInIsolation, mode);
}

template <typename String> inline void QUrlPrivate::setPassword(String &&value, QUrl::ParsingMode mode)
{
    sectionIsPresent |= Password;
    recodeFromUser(password, value, passwordInIsolation, mode);
}

template <typename String> inline void QUrlPrivate::setPath(String &&value, QUrl::ParsingMode mode)
{
    // sectionIsPresent |= Path; // not used, save some cycles
    recodeFromUser(path, value, pathInIsolation, mode);
}

template <typename String> inline void QUrlPrivate::setFragment(String &&value, QUrl::ParsingMode mode)
{
    sectionIsPresent |= Fragment;
    recodeFromUser(fragment, value, fragmentInIsolation, mode);
}

template <typename String> inline void QUrlPrivate::setQuery(String &&value, QUrl::ParsingMode mode)
{
    sectionIsPresent |= Query;
    recodeFromUser(query, value, queryInIsolation, mode);
}

// Host handling
// The RFC says the host is:
//    host          = IP-literal / IPv4address / reg-name
//    IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
//    IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
//  [a strict definition of IPv6Address and IPv4Address]
//     reg-name      = *( unreserved / pct-encoded / sub-delims )
//
// We deviate from the standard in all but IPvFuture. For IPvFuture we accept
// and store only exactly what the RFC says we should. No percent-encoding is
// permitted in this field, so Unicode characters and space aren't either.
//
// For IPv4 addresses, we accept broken addresses like inet_aton does (that is,
// less than three dots). However, we correct the address to the proper form
// and store the corrected address. After correction, we comply to the RFC and
// it's exclusively composed of unreserved characters.
//
// For IPv6 addresses, we accept addresses including trailing (embedded) IPv4
// addresses, the so-called v4-compat and v4-mapped addresses. We also store
// those addresses like that in the hostname field, which violates the spec.
// IPv6 hosts are stored with the square brackets in the QString. It also
// requires no transformation in any way.
//
// As for registered names, it's the other way around: we accept only valid
// hostnames as specified by STD 3 and IDNA. That means everything we accept is
// valid in the RFC definition above, but there are many valid reg-names
// according to the RFC that we do not accept in the name of security. Since we
// do accept IDNA, reg-names are subject to ACE encoding and decoding, which is
// specified by the DecodeUnicode flag. The hostname is stored in its Unicode form.

inline void QUrlPrivate::appendHost(QString &appendTo, QUrl::FormattingOptions options) const
{
    if (host.isEmpty()) {
        if ((sectionIsPresent & Host) && appendTo.isNull())
            appendTo.detach();
        return;
    }
    if (host.at(0).unicode() == '[') {
        // IPv6 addresses might contain a zone-id which needs to be recoded
        if (options != 0)
            if (qt_urlRecode(appendTo, host, options, nullptr))
                return;
        appendTo += host;
    } else {
        // this is either an IPv4Address or a reg-name
        // if it is a reg-name, it is already stored in Unicode form
        if (options & QUrl::EncodeUnicode && !(options & 0x4000000))
            appendTo += qt_ACE_do(host, ToAceOnly, AllowLeadingDot, {});
        else
            appendTo += host;
    }
}

// the whole IPvFuture is passed and parsed here, including brackets;
// returns null if the parsing was successful, or the QChar of the first failure
static const QChar *parseIpFuture(QString &host, const QChar *begin, const QChar *end, QUrl::ParsingMode mode)
{
    //    IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    static const char acceptable[] =
            "!$&'()*+,;=" // sub-delims
            ":"           // ":"
            "-._~";       // unreserved

    // the brackets and the "v" have been checked
    const QChar *const origBegin = begin;
    if (begin[3].unicode() != '.')
        return &begin[3];
    if (isHexDigit(begin[2].unicode())) {
        // this is so unlikely that we'll just go down the slow path
        // decode the whole string, skipping the "[vH." and "]" which we already know to be there
        host += QStringView(begin, 4);

        // uppercase the version, if necessary
        if (begin[2].unicode() >= 'a')
            host[host.size() - 2] = QChar{begin[2].unicode() - 0x20};

        begin += 4;
        --end;

        QString decoded;
        if (mode == QUrl::TolerantMode && qt_urlRecode(decoded, QStringView{begin, end}, QUrl::FullyDecoded, nullptr)) {
            begin = decoded.constBegin();
            end = decoded.constEnd();
        }

        for ( ; begin != end; ++begin) {
            if (isAsciiLetterOrNumber(begin->unicode()))
                host += *begin;
            else if (begin->unicode() < 0x80 && strchr(acceptable, begin->unicode()) != nullptr)
                host += *begin;
            else
                return decoded.isEmpty() ? begin : &origBegin[2];
        }
        host += u']';
        return nullptr;
    }
    return &origBegin[2];
}

// ONLY the IPv6 address is parsed here, WITHOUT the brackets
static const QChar *parseIp6(QString &host, const QChar *begin, const QChar *end, QUrl::ParsingMode mode)
{
    QStringView decoded(begin, end);
    QString decodedBuffer;
    if (mode == QUrl::TolerantMode) {
        // this struct is kept in automatic storage because it's only 4 bytes
        const ushort decodeColon[] = { decode(':'), 0 };
        if (qt_urlRecode(decodedBuffer, decoded, QUrl::ComponentFormattingOption::PrettyDecoded, decodeColon))
            decoded = decodedBuffer;
    }

    const QStringView zoneIdIdentifier(u"%25");
    QIPAddressUtils::IPv6Address address;
    QStringView zoneId;

    qsizetype zoneIdPosition = decoded.indexOf(zoneIdIdentifier);
    if ((zoneIdPosition != -1) && (decoded.lastIndexOf(zoneIdIdentifier) == zoneIdPosition)) {
        zoneId = decoded.mid(zoneIdPosition + zoneIdIdentifier.size());
        decoded.truncate(zoneIdPosition);

        // was there anything after the zone ID separator?
        if (zoneId.isEmpty())
            return end;
    }

    // did the address become empty after removing the zone ID?
    // (it might have always been empty)
    if (decoded.isEmpty())
        return end;

    const QChar *ret = QIPAddressUtils::parseIp6(address, decoded.constBegin(), decoded.constEnd());
    if (ret)
        return begin + (ret - decoded.constBegin());

    host.reserve(host.size() + (end - begin) + 2);  // +2 for the brackets
    host += u'[';
    QIPAddressUtils::toString(host, address);

    if (!zoneId.isEmpty()) {
        host += zoneIdIdentifier;
        host += zoneId;
    }
    host += u']';
    return nullptr;
}

inline bool
QUrlPrivate::setHost(const QString &value, qsizetype from, qsizetype iend, QUrl::ParsingMode mode)
{
    Q_ASSERT_X(mode != QUrl::DecodedMode, "setUserInfo",
               "This function should only be called when parsing encoded components");
    const QChar *begin = value.constData() + from;
    const QChar *end = value.constData() + iend;

    const qsizetype len = end - begin;
    host.clear();
    sectionIsPresent &= ~Host;
    if (!value.isNull() || (sectionIsPresent & Authority))
        sectionIsPresent |= Host;
    if (len == 0)
        return true;

    if (begin[0].unicode() == '[') {
        // IPv6Address or IPvFuture
        // smallest IPv6 address is      "[::]"   (len = 4)
        // smallest IPvFuture address is "[v7.X]" (len = 6)
        if (end[-1].unicode() != ']') {
            setError(HostMissingEndBracket, value);
            return false;
        }

        if (len > 5 && begin[1].unicode() == 'v') {
            const QChar *c = parseIpFuture(host, begin, end, mode);
            if (c)
                setError(InvalidIPvFutureError, value, c - value.constData());
            return !c;
        } else if (begin[1].unicode() == 'v') {
            setError(InvalidIPvFutureError, value, from);
        }

        const QChar *c = parseIp6(host, begin + 1, end - 1, mode);
        if (!c)
            return true;

        if (c == end - 1)
            setError(InvalidIPv6AddressError, value, from);
        else
            setError(InvalidCharacterInIPv6Error, value, c - value.constData());
        return false;
    }

    // check if it's an IPv4 address
    QIPAddressUtils::IPv4Address ip4;
    if (QIPAddressUtils::parseIp4(ip4, begin, end)) {
        // yes, it was
        QIPAddressUtils::toString(host, ip4);
        return true;
    }

    // This is probably a reg-name.
    // But it can also be an encoded string that, when decoded becomes one
    // of the types above.
    //
    // Two types of encoding are possible:
    //  percent encoding (e.g., "%31%30%2E%30%2E%30%2E%31" -> "10.0.0.1")
    //  Unicode encoding (some non-ASCII characters case-fold to digits
    //                    when nameprepping is done)
    //
    // The qt_ACE_do function below does IDNA normalization and the STD3 check.
    // That means a Unicode string may become an IPv4 address, but it cannot
    // produce a '[' or a '%'.

    // check for percent-encoding first
    QString s;
    if (mode == QUrl::TolerantMode && qt_urlRecode(s, QStringView{begin, end}, { }, nullptr)) {
        // something was decoded
        // anything encoded left?
        qsizetype pos = s.indexOf(QChar(0x25)); // '%'
        if (pos != -1) {
            setError(InvalidRegNameError, s, pos);
            return false;
        }

        // recurse
        return setHost(s, 0, s.size(), QUrl::StrictMode);
    }

    s = qt_ACE_do(value.mid(from, iend - from), NormalizeAce, ForbidLeadingDot, {});
    if (s.isEmpty()) {
        setError(InvalidRegNameError, value);
        return false;
    }

    // check IPv4 again
    if (QIPAddressUtils::parseIp4(ip4, s.constBegin(), s.constEnd())) {
        QIPAddressUtils::toString(host, ip4);
    } else {
        host = s;
    }
    return true;
}

inline void QUrlPrivate::parse(const QString &url, QUrl::ParsingMode parsingMode)
{
    //   URI-reference = URI / relative-ref
    //   URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    //   relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
    //   hier-part     = "//" authority path-abempty
    //                 / other path types
    //   relative-part = "//" authority path-abempty
    //                 /  other path types here

    Q_ASSERT_X(parsingMode != QUrl::DecodedMode, "parse",
               "This function should only be called when parsing encoded URLs");
    Q_ASSERT(sectionIsPresent == 0);
    Q_ASSERT(!error);

    // find the important delimiters
    qsizetype colon = -1;
    qsizetype question = -1;
    qsizetype hash = -1;
    const qsizetype len = url.size();
    const QChar *const begin = url.constData();
    const ushort *const data = reinterpret_cast<const ushort *>(begin);

    for (qsizetype i = 0; i < len; ++i) {
        size_t uc = data[i];
        if (uc == '#' && hash == -1) {
            hash = i;

            // nothing more to be found
            break;
        }

        if (question == -1) {
            if (uc == ':' && colon == -1)
                colon = i;
            else if (uc == '?')
                question = i;
        }
    }

    // check if we have a scheme
    qsizetype hierStart;
    if (colon != -1 && setScheme(url, colon, /* don't set error */ false)) {
        hierStart = colon + 1;
    } else {
        // recover from a failed scheme: it might not have been a scheme at all
        scheme.clear();
        sectionIsPresent = 0;
        hierStart = 0;
    }

    qsizetype pathStart;
    qsizetype hierEnd = qMin<size_t>(qMin<size_t>(question, hash), len);
    if (hierEnd - hierStart >= 2 && data[hierStart] == '/' && data[hierStart + 1] == '/') {
        // we have an authority, it ends at the first slash after these
        qsizetype authorityEnd = hierEnd;
        for (qsizetype i = hierStart + 2; i < authorityEnd ; ++i) {
            if (data[i] == '/') {
                authorityEnd = i;
                break;
            }
        }

        setAuthority(url, hierStart + 2, authorityEnd, parsingMode);

        // even if we failed to set the authority properly, let's try to recover
        pathStart = authorityEnd;
        setPath(QStringView(url).sliced(pathStart, hierEnd - pathStart), parsingMode);
    } else {
        Q_ASSERT(userName.isNull());
        Q_ASSERT(password.isNull());
        Q_ASSERT(host.isNull());
        Q_ASSERT(port == -1);
        pathStart = hierStart;

        if (hierStart < hierEnd)
            setPath(QStringView(url).sliced(hierStart, hierEnd - hierStart), parsingMode);
        else
            path.clear();
    }

    Q_ASSERT(query.isNull());
    if (size_t(question) < size_t(hash))
        setQuery(QStringView(url).sliced(question + 1, qMin<size_t>(hash, len) - question - 1),
                 parsingMode);

    Q_ASSERT(fragment.isNull());
    if (hash != -1)
        setFragment(QStringView(url).sliced(hash + 1, len - hash - 1), parsingMode);

    if (error || parsingMode == QUrl::TolerantMode)
        return;

    // The parsing so far was partially tolerant of errors, except for the
    // scheme parser (which is always strict) and the authority (which was
    // executed in strict mode).
    // If we haven't found any errors so far, continue the strict-mode parsing
    // from the path component onwards.

    if (!validateComponent(Path, url, pathStart, hierEnd))
        return;
    if (size_t(question) < size_t(hash) && !validateComponent(Query, url, question + 1, qMin<size_t>(hash, len)))
        return;
    if (hash != -1)
        validateComponent(Fragment, url, hash + 1, len);
}

QString QUrlPrivate::toLocalFile(QUrl::FormattingOptions options) const
{
    QString tmp;
    QString ourPath;
    appendPath(ourPath, options, QUrlPrivate::Path);

    // magic for shared drive on windows
    if (!host.isEmpty()) {
        tmp = "//"_L1 + host;
#ifdef Q_OS_WIN // QTBUG-42346, WebDAV is visible as local file on Windows only.
        if (scheme == webDavScheme())
            tmp += webDavSslTag();
#endif
        if (!ourPath.isEmpty() && !ourPath.startsWith(u'/'))
            tmp += u'/';
        tmp += ourPath;
    } else {
        tmp = ourPath;
#ifdef Q_OS_WIN
        // magic for drives on windows
        if (ourPath.length() > 2 && ourPath.at(0) == u'/' && ourPath.at(2) == u':')
            tmp.remove(0, 1);
#endif
    }
    return tmp;
}

/*
    From http://www.ietf.org/rfc/rfc3986.txt, 5.2.3: Merge paths

    Returns a merge of the current path with the relative path passed
    as argument.

    Note: \a relativePath is relative (does not start with '/').
*/
inline QString QUrlPrivate::mergePaths(const QString &relativePath) const
{
    // If the base URI has a defined authority component and an empty
    // path, then return a string consisting of "/" concatenated with
    // the reference's path; otherwise,
    if (!host.isEmpty() && path.isEmpty())
        return u'/' + relativePath;

    // Return a string consisting of the reference's path component
    // appended to all but the last segment of the base URI's path
    // (i.e., excluding any characters after the right-most "/" in the
    // base URI path, or excluding the entire base URI path if it does
    // not contain any "/" characters).
    QString newPath;
    if (!path.contains(u'/'))
        newPath = relativePath;
    else
        newPath = QStringView{path}.left(path.lastIndexOf(u'/') + 1) + relativePath;

    return newPath;
}

// Authority-less URLs cannot have paths starting with double slashes (see
// QUrlPrivate::validityError). We refuse to turn a valid URL into invalid by
// way of QUrl::resolved().
static void fixupNonAuthorityPath(QString *path)
{
    if (path->isEmpty() || path->at(0) != u'/')
        return;

    // Find the first non-slash character, because its position is equal to the
    // number of slashes. We'll remove all but one of them.
    qsizetype i = 0;
    while (i + 1 < path->size() && path->at(i + 1) == u'/')
        ++i;
    if (i)
        path->remove(0, i);
}

inline QUrlPrivate::ErrorCode QUrlPrivate::validityError(QString *source, qsizetype *position) const
{
    Q_ASSERT(!source == !position);
    if (error) {
        if (source) {
            *source = error->source;
            *position = error->position;
        }
        return error->code;
    }

    // There are three more cases of invalid URLs that QUrl recognizes and they
    // are only possible with constructed URLs (setXXX methods), not with
    // parsing. Therefore, they are tested here.
    //
    // Two cases are a non-empty path that doesn't start with a slash and:
    //  - with an authority
    //  - without an authority, without scheme but the path with a colon before
    //    the first slash
    // The third case is an empty authority and a non-empty path that starts
    // with "//".
    // Those cases are considered invalid because toString() would produce a URL
    // that wouldn't be parsed back to the same QUrl.

    if (path.isEmpty())
        return NoError;
    if (path.at(0) == u'/') {
        if (hasAuthority() || path.size() == 1 || path.at(1) != u'/')
            return NoError;
        if (source) {
            *source = path;
            *position = 0;
        }
        return AuthorityAbsentAndPathIsDoubleSlash;
    }

    if (sectionIsPresent & QUrlPrivate::Host) {
        if (source) {
            *source = path;
            *position = 0;
        }
        return AuthorityPresentAndPathIsRelative;
    }
    if (sectionIsPresent & QUrlPrivate::Scheme)
        return NoError;

    // check for a path of "text:text/"
    for (qsizetype i = 0; i < path.size(); ++i) {
        ushort c = path.at(i).unicode();
        if (c == '/') {
            // found the slash before the colon
            return NoError;
        }
        if (c == ':') {
            // found the colon before the slash, it's invalid
            if (source) {
                *source = path;
                *position = i;
            }
            return RelativeUrlPathContainsColonBeforeSlash;
        }
    }
    return NoError;
}

bool QUrlPrivate::validateComponent(QUrlPrivate::Section section, const QString &input,
                                    qsizetype begin, qsizetype end)
{
    // What we need to look out for, that the regular parser tolerates:
    //  - percent signs not followed by two hex digits
    //  - forbidden characters, which should always appear encoded
    //    '"' / '<' / '>' / '\' / '^' / '`' / '{' / '|' / '}' / BKSP
    //    control characters
    //  - delimiters not allowed in certain positions
    //    . scheme: parser is already strict
    //    . user info: gen-delims except ":" disallowed ("/" / "?" / "#" / "[" / "]" / "@")
    //    . host: parser is stricter than the standard
    //    . port: parser is stricter than the standard
    //    . path: all delimiters allowed
    //    . fragment: all delimiters allowed
    //    . query: all delimiters allowed
    static const char forbidden[] = "\"<>\\^`{|}\x7F";
    static const char forbiddenUserInfo[] = ":/?#[]@";

    Q_ASSERT(section != Authority && section != Hierarchy && section != FullUrl);

    const ushort *const data = reinterpret_cast<const ushort *>(input.constData());
    for (size_t i = size_t(begin); i < size_t(end); ++i) {
        uint uc = data[i];
        if (uc >= 0x80)
            continue;

        bool error = false;
        if ((uc == '%' && (size_t(end) < i + 2 || !isHex(data[i + 1]) || !isHex(data[i + 2])))
                || uc <= 0x20 || strchr(forbidden, uc)) {
            // found an error
            error = true;
        } else if (section & UserInfo) {
            if (section == UserInfo && strchr(forbiddenUserInfo + 1, uc))
                error = true;
            else if (section != UserInfo && strchr(forbiddenUserInfo, uc))
                error = true;
        }

        if (!error)
            continue;

        ErrorCode errorCode = ErrorCode(int(section) << 8);
        if (section == UserInfo) {
            // is it the user name or the password?
            errorCode = InvalidUserNameError;
            for (size_t j = size_t(begin); j < i; ++j)
                if (data[j] == ':') {
                    errorCode = InvalidPasswordError;
                    break;
                }
        }

        setError(errorCode, input, i);
        return false;
    }

    // no errors
    return true;
}

#if 0
inline void QUrlPrivate::validate() const
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

    if (scheme == "mailto"_L1) {
        if (!host.isEmpty() || port != -1 || !userName.isEmpty() || !password.isEmpty()) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "expected empty host, username,"
                                                           "port and password"),
                                      0, 0);
        }
    } else if (scheme == ftpScheme() || scheme == httpScheme()) {
        if (host.isEmpty() && !(path.isEmpty() && encodedPath.isEmpty())) {
            that->isValid = false;
            that->errorInfo.setParams(0, QT_TRANSLATE_NOOP(QUrl, "the host is empty, but not the path"),
                                      0, 0);
        }
    }
}
#endif

/*!
    \macro QT_NO_URL_CAST_FROM_STRING
    \relates QUrl

    Disables automatic conversions from QString (or char *) to QUrl.

    Compiling your code with this define is useful when you have a lot of
    code that uses QString for file names and you wish to convert it to
    use QUrl for network transparency. In any code that uses QUrl, it can
    help avoid missing QUrl::resolved() calls, and other misuses of
    QString to QUrl conversions.

    For example, if you have code like

    \code
        url = filename; // probably not what you want
    \endcode

    you can rewrite it as

    \code
        url = QUrl::fromLocalFile(filename);
        url = baseurl.resolved(QUrl(filename));
    \endcode

    \sa QT_NO_CAST_FROM_ASCII
*/


/*!
    Constructs a URL by parsing \a url. Note this constructor expects a proper
    URL or URL-Reference and will not attempt to guess intent. For example, the
    following declaration:

    \snippet code/src_corelib_io_qurl.cpp constructor-url-reference

    Will construct a valid URL but it may not be what one expects, as the
    scheme() part of the input is missing. For a string like the above,
    applications may want to use fromUserInput(). For this constructor or
    setUrl(), the following is probably what was intended:

    \snippet code/src_corelib_io_qurl.cpp constructor-url

    QUrl will automatically percent encode
    all characters that are not allowed in a URL and decode the percent-encoded
    sequences that represent an unreserved character (letters, digits, hyphens,
    underscores, dots and tildes). All other characters are left in their
    original forms.

    Parses the \a url using the parser mode \a parsingMode. In TolerantMode
    (the default), QUrl will correct certain mistakes, notably the presence of
    a percent character ('%') not followed by two hexadecimal digits, and it
    will accept any character in any position. In StrictMode, encoding mistakes
    will not be tolerated and QUrl will also check that certain forbidden
    characters are not present in unencoded form. If an error is detected in
    StrictMode, isValid() will return false. The parsing mode DecodedMode is not
    permitted in this context.

    Example:

    \snippet code/src_corelib_io_qurl.cpp 0

    To construct a URL from an encoded string, you can also use fromEncoded():

    \snippet code/src_corelib_io_qurl.cpp 1

    Both functions are equivalent and, in Qt 5, both functions accept encoded
    data. Usually, the choice of the QUrl constructor or setUrl() versus
    fromEncoded() will depend on the source data: the constructor and setUrl()
    take a QString, whereas fromEncoded takes a QByteArray.

    \sa setUrl(), fromEncoded(), TolerantMode
*/
QUrl::QUrl(const QString &url, ParsingMode parsingMode) : d(nullptr)
{
    setUrl(url, parsingMode);
}

/*!
    Constructs an empty QUrl object.
*/
QUrl::QUrl() : d(nullptr)
{
}

/*!
    Constructs a copy of \a other.
*/
QUrl::QUrl(const QUrl &other) noexcept : d(other.d)
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
    Returns \c true if the URL is non-empty and valid; otherwise returns \c false.

    The URL is run through a conformance test. Every part of the URL
    must conform to the standard encoding rules of the URI standard
    for the URL to be reported as valid.

    \snippet code/src_corelib_io_qurl.cpp 2
*/
bool QUrl::isValid() const
{
    if (isEmpty()) {
        // also catches d == nullptr
        return false;
    }
    return d->validityError() == QUrlPrivate::NoError;
}

/*!
    Returns \c true if the URL has no data; otherwise returns \c false.

    \sa clear()
*/
bool QUrl::isEmpty() const
{
    if (!d) return true;
    return d->isEmpty();
}

/*!
    Resets the content of the QUrl. After calling this function, the
    QUrl is equal to one that has been constructed with the default
    empty constructor.

    \sa isEmpty()
*/
void QUrl::clear()
{
    if (d && !d->ref.deref())
        delete d;
    d = nullptr;
}

/*!
    Parses \a url and sets this object to that value. QUrl will automatically
    percent encode all characters that are not allowed in a URL and decode the
    percent-encoded sequences that represent an unreserved character (letters,
    digits, hyphens, underscores, dots and tildes). All other characters are
    left in their original forms.

    Parses the \a url using the parser mode \a parsingMode. In TolerantMode
    (the default), QUrl will correct certain mistakes, notably the presence of
    a percent character ('%') not followed by two hexadecimal digits, and it
    will accept any character in any position. In StrictMode, encoding mistakes
    will not be tolerated and QUrl will also check that certain forbidden
    characters are not present in unencoded form. If an error is detected in
    StrictMode, isValid() will return false. The parsing mode DecodedMode is
    not permitted in this context and will produce a run-time warning.

    \sa url(), toString()
*/
void QUrl::setUrl(const QString &url, ParsingMode parsingMode)
{
    if (parsingMode == DecodedMode) {
        qWarning("QUrl: QUrl::DecodedMode is not permitted when parsing a full URL");
    } else {
        detachToClear();
        d->parse(url, parsingMode);
    }
}

/*!
    Sets the scheme of the URL to \a scheme. As a scheme can only
    contain ASCII characters, no conversion or decoding is done on the
    input. It must also start with an ASCII letter.

    The scheme describes the type (or protocol) of the URL. It's
    represented by one or more ASCII characters at the start the URL.

    A scheme is strictly \l {RFC 3986}-compliant:
        \tt {scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )}

    The following example shows a URL where the scheme is "ftp":

    \image qurl-authority2.png

    To set the scheme, the following call is used:
    \snippet code/src_corelib_io_qurl.cpp 11

    The scheme can also be empty, in which case the URL is interpreted
    as relative.

    \sa scheme(), isRelative()
*/
void QUrl::setScheme(const QString &scheme)
{
    detach();
    d->clearError();
    if (scheme.isEmpty()) {
        // schemes are not allowed to be empty
        d->sectionIsPresent &= ~QUrlPrivate::Scheme;
        d->flags &= ~QUrlPrivate::IsLocalFile;
        d->scheme.clear();
    } else {
        d->setScheme(scheme, scheme.size(), /* do set error */ true);
    }
}

/*!
    Returns the scheme of the URL. If an empty string is returned,
    this means the scheme is undefined and the URL is then relative.

    The scheme can only contain US-ASCII letters or digits, which means it
    cannot contain any character that would otherwise require encoding.
    Additionally, schemes are always returned in lowercase form.

    \sa setScheme(), isRelative()
*/
QString QUrl::scheme() const
{
    if (!d) return QString();

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

    \image qurl-authority.png

    The \a authority data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form
    and the tolerant parser will correct stray '%' not followed by two hex
    characters.

    This function does not allow \a mode to be QUrl::DecodedMode. To set fully
    decoded data, call setUserName(), setPassword(), setHost() and setPort()
    individually.

    \sa setUserInfo(), setHost(), setPort()
*/
void QUrl::setAuthority(const QString &authority, ParsingMode mode)
{
    detach();
    d->clearError();

    if (mode == DecodedMode) {
        qWarning("QUrl::setAuthority(): QUrl::DecodedMode is not permitted in this function");
        return;
    }

    d->setAuthority(authority, 0, authority.size(), mode);
}

/*!
    Returns the authority of the URL if it is defined; otherwise
    an empty string is returned.

    This function returns an unambiguous value, which may contain that
    characters still percent-encoded, plus some control sequences not
    representable in decoded form in QString.

    The \a options argument controls how to format the user info component. The
    value of QUrl::FullyDecoded is not permitted in this function. If you need
    to obtain fully decoded data, call userName(), password(), host() and
    port() individually.

    \sa setAuthority(), userInfo(), userName(), password(), host(), port()
*/
QString QUrl::authority(ComponentFormattingOptions options) const
{
    QString result;
    if (!d)
        return result;

    if (options == QUrl::FullyDecoded) {
        qWarning("QUrl::authority(): QUrl::FullyDecoded is not permitted in this function");
        return result;
    }

    d->appendAuthority(result, options, QUrlPrivate::Authority);
    return result;
}

/*!
    Sets the user info of the URL to \a userInfo. The user info is an
    optional part of the authority of the URL, as described in
    setAuthority().

    The user info consists of a user name and optionally a password,
    separated by a ':'. If the password is empty, the colon must be
    omitted. The following example shows a valid user info string:

    \image qurl-authority3.png

    The \a userInfo data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form
    and the tolerant parser will correct stray '%' not followed by two hex
    characters.

    This function does not allow \a mode to be QUrl::DecodedMode. To set fully
    decoded data, call setUserName() and setPassword() individually.

    \sa userInfo(), setUserName(), setPassword(), setAuthority()
*/
void QUrl::setUserInfo(const QString &userInfo, ParsingMode mode)
{
    detach();
    d->clearError();
    QString trimmed = userInfo.trimmed();
    if (mode == DecodedMode) {
        qWarning("QUrl::setUserInfo(): QUrl::DecodedMode is not permitted in this function");
        return;
    }

    d->setUserInfo(std::move(trimmed), mode);
    if (userInfo.isNull()) {
        // QUrlPrivate::setUserInfo cleared almost everything
        // but it leaves the UserName bit set
        d->sectionIsPresent &= ~QUrlPrivate::UserInfo;
    } else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::UserInfo, userInfo)) {
        d->sectionIsPresent &= ~QUrlPrivate::UserInfo;
        d->userName.clear();
        d->password.clear();
    }
}

/*!
    Returns the user info of the URL, or an empty string if the user
    info is undefined.

    This function returns an unambiguous value, which may contain that
    characters still percent-encoded, plus some control sequences not
    representable in decoded form in QString.

    The \a options argument controls how to format the user info component. The
    value of QUrl::FullyDecoded is not permitted in this function. If you need
    to obtain fully decoded data, call userName() and password() individually.

    \sa setUserInfo(), userName(), password(), authority()
*/
QString QUrl::userInfo(ComponentFormattingOptions options) const
{
    QString result;
    if (!d)
        return result;

    if (options == QUrl::FullyDecoded) {
        qWarning("QUrl::userInfo(): QUrl::FullyDecoded is not permitted in this function");
        return result;
    }

    d->appendUserInfo(result, options, QUrlPrivate::UserInfo);
    return result;
}

/*!
    Sets the URL's user name to \a userName. The \a userName is part
    of the user info element in the authority of the URL, as described
    in setUserInfo().

    The \a userName data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode (the default), all characters are accepted in undecoded form
    and the tolerant parser will correct stray '%' not followed by two hex
    characters. In DecodedMode, '%' stand for themselves and encoded characters
    are not possible.

    QUrl::DecodedMode should be used when setting the user name from a data
    source which is not a URL, such as a password dialog shown to the user or
    with a user name obtained by calling userName() with the QUrl::FullyDecoded
    formatting option.

    \sa userName(), setUserInfo()
*/
void QUrl::setUserName(const QString &userName, ParsingMode mode)
{
    detach();
    d->clearError();

    d->setUserName(userName, mode);
    if (userName.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::UserName;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::UserName, userName))
        d->userName.clear();
}

/*!
    Returns the user name of the URL if it is defined; otherwise
    an empty string is returned.

    The \a options argument controls how to format the user name component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context, such as setting in QAuthenticator or
    negotiating a login.

    \sa setUserName(), userInfo()
*/
QString QUrl::userName(ComponentFormattingOptions options) const
{
    QString result;
    if (d)
        d->appendUserName(result, options);
    return result;
}

/*!
    Sets the URL's password to \a password. The \a password is part of
    the user info element in the authority of the URL, as described in
    setUserInfo().

    The \a password data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    QUrl::DecodedMode should be used when setting the password from a data
    source which is not a URL, such as a password dialog shown to the user or
    with a password obtained by calling password() with the QUrl::FullyDecoded
    formatting option.

    \sa password(), setUserInfo()
*/
void QUrl::setPassword(const QString &password, ParsingMode mode)
{
    detach();
    d->clearError();

    d->setPassword(password, mode);
    if (password.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::Password;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Password, password))
        d->password.clear();
}

/*!
    Returns the password of the URL if it is defined; otherwise
    an empty string is returned.

    The \a options argument controls how to format the user name component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context, such as setting in QAuthenticator or
    negotiating a login.

    \sa setPassword()
*/
QString QUrl::password(ComponentFormattingOptions options) const
{
    QString result;
    if (d)
        d->appendPassword(result, options);
    return result;
}

/*!
    Sets the host of the URL to \a host. The host is part of the
    authority.

    The \a host data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    Note that, in all cases, the result of the parsing must be a valid hostname
    according to STD 3 rules, as modified by the Internationalized Resource
    Identifiers specification (RFC 3987). Invalid hostnames are not permitted
    and will cause isValid() to become false.

    \sa host(), setAuthority()
*/
void QUrl::setHost(const QString &host, ParsingMode mode)
{
    detach();
    d->clearError();

    QString data = host;
    if (mode == DecodedMode) {
        data.replace(u'%', "%25"_L1);
        mode = TolerantMode;
    }

    if (d->setHost(data, 0, data.size(), mode)) {
        return;
    } else if (!data.startsWith(u'[')) {
        // setHost failed, it might be IPv6 or IPvFuture in need of bracketing
        Q_ASSERT(d->error);

        data.prepend(u'[');
        data.append(u']');
        if (!d->setHost(data, 0, data.size(), mode)) {
            // failed again
            if (data.contains(u':')) {
                // source data contains ':', so it's an IPv6 error
                d->error->code = QUrlPrivate::InvalidIPv6AddressError;
            }
            d->sectionIsPresent &= ~QUrlPrivate::Host;
        } else {
            // succeeded
            d->clearError();
        }
    }
}

/*!
    Returns the host of the URL if it is defined; otherwise
    an empty string is returned.

    The \a options argument controls how the hostname will be formatted. The
    QUrl::EncodeUnicode option will cause this function to return the hostname
    in the ASCII-Compatible Encoding (ACE) form, which is suitable for use in
    channels that are not 8-bit clean or that require the legacy hostname (such
    as DNS requests or in HTTP request headers). If that flag is not present,
    this function returns the International Domain Name (IDN) in Unicode form,
    according to the list of permissible top-level domains (see
    idnWhitelist()).

    All other flags are ignored. Host names cannot contain control or percent
    characters, so the returned value can be considered fully decoded.

    \sa setHost(), idnWhitelist(), setIdnWhitelist(), authority()
*/
QString QUrl::host(ComponentFormattingOptions options) const
{
    QString result;
    if (d) {
        d->appendHost(result, options);
        if (result.startsWith(u'['))
            result = result.mid(1, result.size() - 2);
    }
    return result;
}

/*!
    Sets the port of the URL to \a port. The port is part of the
    authority of the URL, as described in setAuthority().

    \a port must be between 0 and 65535 inclusive. Setting the
    port to -1 indicates that the port is unspecified.
*/
void QUrl::setPort(int port)
{
    detach();
    d->clearError();

    if (port < -1 || port > 65535) {
        d->setError(QUrlPrivate::InvalidPortError, QString::number(port), 0);
        port = -1;
    }

    d->port = port;
    if (port != -1)
        d->sectionIsPresent |= QUrlPrivate::Host;
}

/*!
    \since 4.1

    Returns the port of the URL, or \a defaultPort if the port is
    unspecified.

    Example:

    \snippet code/src_corelib_io_qurl.cpp 3
*/
int QUrl::port(int defaultPort) const
{
    if (!d) return defaultPort;
    return d->port == -1 ? defaultPort : d->port;
}

/*!
    Sets the path of the URL to \a path. The path is the part of the
    URL that comes after the authority but before the query string.

    \image qurl-ftppath.png

    For non-hierarchical schemes, the path will be everything
    following the scheme declaration, as in the following example:

    \image qurl-mailtopath.png

    The \a path data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    QUrl::DecodedMode should be used when setting the path from a data source
    which is not a URL, such as a dialog shown to the user or with a path
    obtained by calling path() with the QUrl::FullyDecoded formatting option.

    \sa path()
*/
void QUrl::setPath(const QString &path, ParsingMode mode)
{
    detach();
    d->clearError();

    d->setPath(path, mode);

    // optimized out, since there is no path delimiter
//    if (path.isNull())
//        d->sectionIsPresent &= ~QUrlPrivate::Path;
//    else
    if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Path, path))
        d->path.clear();
}

/*!
    Returns the path of the URL.

    \snippet code/src_corelib_io_qurl.cpp 12

    The \a options argument controls how to format the path component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context, such as sending to an FTP server.

    An example of data loss is when you have non-Unicode percent-encoded sequences
    and use FullyDecoded (the default):

    \snippet code/src_corelib_io_qurl.cpp 13

    In this example, there will be some level of data loss because the \c %FF cannot
    be converted.

    Data loss can also occur when the path contains sub-delimiters (such as \c +):

    \snippet code/src_corelib_io_qurl.cpp 14

    Other decoding examples:

    \snippet code/src_corelib_io_qurl.cpp 15

    \sa setPath()
*/
QString QUrl::path(ComponentFormattingOptions options) const
{
    QString result;
    if (d)
        d->appendPath(result, options, QUrlPrivate::Path);
    return result;
}

/*!
    \since 5.2

    Returns the name of the file, excluding the directory path.

    Note that, if this QUrl object is given a path ending in a slash, the name of the file is considered empty.

    If the path doesn't contain any slash, it is fully returned as the fileName.

    Example:

    \snippet code/src_corelib_io_qurl.cpp 7

    The \a options argument controls how to format the file name component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    \sa path()
*/
QString QUrl::fileName(ComponentFormattingOptions options) const
{
    const QString ourPath = path(options);
    const qsizetype slash = ourPath.lastIndexOf(u'/');
    if (slash == -1)
        return ourPath;
    return ourPath.mid(slash + 1);
}

/*!
    \since 4.2

    Returns \c true if this URL contains a Query (i.e., if ? was seen on it).

    \sa setQuery(), query(), hasFragment()
*/
bool QUrl::hasQuery() const
{
    if (!d) return false;
    return d->hasQuery();
}

/*!
    Sets the query string of the URL to \a query.

    This function is useful if you need to pass a query string that
    does not fit into the key-value pattern, or that uses a different
    scheme for encoding special characters than what is suggested by
    QUrl.

    Passing a value of QString() to \a query (a null QString) unsets
    the query completely. However, passing a value of QString("")
    will set the query to an empty value, as if the original URL
    had a lone "?".

    The \a query data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    Query strings often contain percent-encoded sequences, so use of
    DecodedMode is discouraged. One special sequence to be aware of is that of
    the plus character ('+'). QUrl does not convert spaces to plus characters,
    even though HTML forms posted by web browsers do. In order to represent an
    actual plus character in a query, the sequence "%2B" is usually used. This
    function will leave "%2B" sequences untouched in TolerantMode or
    StrictMode.

    \sa query(), hasQuery()
*/
void QUrl::setQuery(const QString &query, ParsingMode mode)
{
    detach();
    d->clearError();

    d->setQuery(query, mode);
    if (query.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::Query;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Query, query))
        d->query.clear();
}

/*!
    \overload
    \since 5.0
    Sets the query string of the URL to \a query.

    This function reconstructs the query string from the QUrlQuery object and
    sets on this QUrl object. This function does not have parsing parameters
    because the QUrlQuery contains data that is already parsed.

    \sa query(), hasQuery()
*/
void QUrl::setQuery(const QUrlQuery &query)
{
    detach();
    d->clearError();

    // we know the data is in the right format
    d->query = query.toString();
    if (query.isEmpty())
        d->sectionIsPresent &= ~QUrlPrivate::Query;
    else
        d->sectionIsPresent |= QUrlPrivate::Query;
}

/*!
    Returns the query string of the URL if there's a query string, or an empty
    result if not. To determine if the parsed URL contained a query string, use
    hasQuery().

    The \a options argument controls how to format the query component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that use of QUrl::FullyDecoded in queries is discouraged, as queries
    often contain data that is supposed to remain percent-encoded, including
    the use of the "%2B" sequence to represent a plus character ('+').

    \sa setQuery(), hasQuery()
*/
QString QUrl::query(ComponentFormattingOptions options) const
{
    QString result;
    if (d) {
        d->appendQuery(result, options, QUrlPrivate::Query);
        if (d->hasQuery() && result.isNull())
            result.detach();
    }
    return result;
}

/*!
    Sets the fragment of the URL to \a fragment. The fragment is the
    last part of the URL, represented by a '#' followed by a string of
    characters. It is typically used in HTTP for referring to a
    certain link or point on a page:

    \image qurl-fragment.png

    The fragment is sometimes also referred to as the URL "reference".

    Passing an argument of QString() (a null QString) will unset the fragment.
    Passing an argument of QString("") (an empty but not null QString) will set the
    fragment to an empty string (as if the original URL had a lone "#").

    The \a fragment data is interpreted according to \a mode: in StrictMode,
    any '%' characters must be followed by exactly two hexadecimal characters
    and some characters (including space) are not allowed in undecoded form. In
    TolerantMode, all characters are accepted in undecoded form and the
    tolerant parser will correct stray '%' not followed by two hex characters.
    In DecodedMode, '%' stand for themselves and encoded characters are not
    possible.

    QUrl::DecodedMode should be used when setting the fragment from a data
    source which is not a URL or with a fragment obtained by calling
    fragment() with the QUrl::FullyDecoded formatting option.

    \sa fragment(), hasFragment()
*/
void QUrl::setFragment(const QString &fragment, ParsingMode mode)
{
    detach();
    d->clearError();

    d->setFragment(fragment, mode);
    if (fragment.isNull())
        d->sectionIsPresent &= ~QUrlPrivate::Fragment;
    else if (mode == StrictMode && !d->validateComponent(QUrlPrivate::Fragment, fragment))
        d->fragment.clear();
}

/*!
    Returns the fragment of the URL. To determine if the parsed URL contained a
    fragment, use hasFragment().

    The \a options argument controls how to format the fragment component. All
    values produce an unambiguous result. With QUrl::FullyDecoded, all
    percent-encoded sequences are decoded; otherwise, the returned value may
    contain some percent-encoded sequences for some control sequences not
    representable in decoded form in QString.

    Note that QUrl::FullyDecoded may cause data loss if those non-representable
    sequences are present. It is recommended to use that value when the result
    will be used in a non-URL context.

    \sa setFragment(), hasFragment()
*/
QString QUrl::fragment(ComponentFormattingOptions options) const
{
    QString result;
    if (d) {
        d->appendFragment(result, options, QUrlPrivate::Fragment);
        if (d->hasFragment() && result.isNull())
            result.detach();
    }
    return result;
}

/*!
    \since 4.2

    Returns \c true if this URL contains a fragment (i.e., if # was seen on it).

    \sa fragment(), setFragment()
*/
bool QUrl::hasFragment() const
{
    if (!d) return false;
    return d->hasFragment();
}

/*!
    Returns the result of the merge of this URL with \a relative. This
    URL is used as a base to convert \a relative to an absolute URL.

    If \a relative is not a relative URL, this function will return \a
    relative directly. Otherwise, the paths of the two URLs are
    merged, and the new URL returned has the scheme and authority of
    the base URL, but with the merged path, as in the following
    example:

    \snippet code/src_corelib_io_qurl.cpp 5

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

    QUrl t;
    if (!relative.d->scheme.isEmpty()) {
        t = relative;
        t.detach();
    } else {
        if (relative.d->hasAuthority()) {
            t = relative;
            t.detach();
        } else {
            t.d = new QUrlPrivate;

            // copy the authority
            t.d->userName = d->userName;
            t.d->password = d->password;
            t.d->host = d->host;
            t.d->port = d->port;
            t.d->sectionIsPresent = d->sectionIsPresent & QUrlPrivate::Authority;

            if (relative.d->path.isEmpty()) {
                t.d->path = d->path;
                if (relative.d->hasQuery()) {
                    t.d->query = relative.d->query;
                    t.d->sectionIsPresent |= QUrlPrivate::Query;
                } else if (d->hasQuery()) {
                    t.d->query = d->query;
                    t.d->sectionIsPresent |= QUrlPrivate::Query;
                }
            } else {
                t.d->path = relative.d->path.startsWith(u'/')
                            ? relative.d->path
                            : d->mergePaths(relative.d->path);
                if (relative.d->hasQuery()) {
                    t.d->query = relative.d->query;
                    t.d->sectionIsPresent |= QUrlPrivate::Query;
                }
            }
        }
        t.d->scheme = d->scheme;
        if (d->hasScheme())
            t.d->sectionIsPresent |= QUrlPrivate::Scheme;
        else
            t.d->sectionIsPresent &= ~QUrlPrivate::Scheme;
        t.d->flags |= d->flags & QUrlPrivate::IsLocalFile;
    }
    t.d->fragment = relative.d->fragment;
    if (relative.d->hasFragment())
        t.d->sectionIsPresent |= QUrlPrivate::Fragment;
    else
        t.d->sectionIsPresent &= ~QUrlPrivate::Fragment;

    t.d->normalizePathSegments(&t.d->path);
    if (!t.d->hasAuthority()) {
        if (t.d->isLocalFile() && t.d->path.startsWith(u'/'))
            t.d->sectionIsPresent |= QUrlPrivate::Host;
        else
            fixupNonAuthorityPath(&t.d->path);
    }

#if defined(QURL_DEBUG)
    qDebug("QUrl(\"%ls\").resolved(\"%ls\") = \"%ls\"",
           qUtf16Printable(url()),
           qUtf16Printable(relative.url()),
           qUtf16Printable(t.url()));
#endif
    return t;
}

/*!
    Returns \c true if the URL is relative; otherwise returns \c false. A URL is
    relative reference if its scheme is undefined; this function is therefore
    equivalent to calling scheme().isEmpty().

    Relative references are defined in RFC 3986 section 4.2.

    \sa {Relative URLs vs Relative Paths}
*/
bool QUrl::isRelative() const
{
    if (!d) return true;
    return !d->hasScheme();
}

/*!
    Returns a string representation of the URL. The output can be customized by
    passing flags with \a options. The option QUrl::FullyDecoded is not
    permitted in this function since it would generate ambiguous data.

    The resulting QString can be passed back to a QUrl later on.

    Synonym for toString(options).

    \sa FormattingOptions, toEncoded(), toString()
*/
QString QUrl::url(FormattingOptions options) const
{
    return toString(options);
}

/*!
    Returns a string representation of the URL. The output can be customized by
    passing flags with \a options. The option QUrl::FullyDecoded is not
    permitted in this function since it would generate ambiguous data.

    The default formatting option is \l{QUrl::FormattingOptions}{PrettyDecoded}.

    \sa FormattingOptions, url(), setUrl()
*/
QString QUrl::toString(FormattingOptions options) const
{
    QString url;
    if (!isValid()) {
        // also catches isEmpty()
        return url;
    }
    if ((options & QUrl::FullyDecoded) == QUrl::FullyDecoded) {
        qWarning("QUrl: QUrl::FullyDecoded is not permitted when reconstructing the full URL");
        options &= ~QUrl::FullyDecoded;
        //options |= QUrl::PrettyDecoded; // no-op, value is 0
    }

    // return just the path if:
    //  - QUrl::PreferLocalFile is passed
    //  - QUrl::RemovePath isn't passed (rather stupid if the user did...)
    //  - there's no query or fragment to return
    //    that is, either they aren't present, or we're removing them
    //  - it's a local file
    if (options.testFlag(QUrl::PreferLocalFile) && !options.testFlag(QUrl::RemovePath)
            && (!d->hasQuery() || options.testFlag(QUrl::RemoveQuery))
            && (!d->hasFragment() || options.testFlag(QUrl::RemoveFragment))
            && isLocalFile()) {
        url = d->toLocalFile(options | QUrl::FullyDecoded);
        return url;
    }

    // for the full URL, we consider that the reserved characters are prettier if encoded
    if (options & DecodeReserved)
        options &= ~EncodeReserved;
    else
        options |= EncodeReserved;

    if (!(options & QUrl::RemoveScheme) && d->hasScheme())
        url += d->scheme + u':';

    bool pathIsAbsolute = d->path.startsWith(u'/');
    if (!((options & QUrl::RemoveAuthority) == QUrl::RemoveAuthority) && d->hasAuthority()) {
        url += "//"_L1;
        d->appendAuthority(url, options, QUrlPrivate::FullUrl);
    } else if (isLocalFile() && pathIsAbsolute) {
        // Comply with the XDG file URI spec, which requires triple slashes.
        url += "//"_L1;
    }

    if (!(options & QUrl::RemovePath))
        d->appendPath(url, options, QUrlPrivate::FullUrl);

    if (!(options & QUrl::RemoveQuery) && d->hasQuery()) {
        url += u'?';
        d->appendQuery(url, options, QUrlPrivate::FullUrl);
    }
    if (!(options & QUrl::RemoveFragment) && d->hasFragment()) {
        url += u'#';
        d->appendFragment(url, options, QUrlPrivate::FullUrl);
    }

    return url;
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
    \since 5.2

    Returns an adjusted version of the URL.
    The output can be customized by passing flags with \a options.

    The encoding options from QUrl::ComponentFormattingOption don't make
    much sense for this method, nor does QUrl::PreferLocalFile.

    This is always equivalent to QUrl(url.toString(options)).

    \sa FormattingOptions, toEncoded(), toString()
*/
QUrl QUrl::adjusted(QUrl::FormattingOptions options) const
{
    if (!isValid()) {
        // also catches isEmpty()
        return QUrl();
    }
    QUrl that = *this;
    if (options & RemoveScheme)
        that.setScheme(QString());
    if ((options & RemoveAuthority) == RemoveAuthority) {
        that.setAuthority(QString());
    } else {
        if ((options & RemoveUserInfo) == RemoveUserInfo)
            that.setUserInfo(QString());
        else if (options & RemovePassword)
            that.setPassword(QString());
        if (options & RemovePort)
            that.setPort(-1);
    }
    if (options & RemoveQuery)
        that.setQuery(QString());
    if (options & RemoveFragment)
        that.setFragment(QString());
    if (options & RemovePath) {
        that.setPath(QString());
    } else if (auto pathOpts = options & (StripTrailingSlash | RemoveFilename | NormalizePathSegments)) {
        that.detach();
        that.d->path.resize(0);
        d->appendPath(that.d->path, pathOpts, QUrlPrivate::Path);
    }
    if (that.d->isLocalFile() && that.d->path.startsWith(u'/')) {
        // ensure absolute file URLs have an empty authority to comply with the
        // XDG file spec (note this may undo a RemoveAuthority)
        that.d->sectionIsPresent |= QUrlPrivate::Host;
    }
    return that;
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
    options &= ~(FullyDecoded | FullyEncoded);
    return toString(options | FullyEncoded).toLatin1();
}

/*!
    Parses \a input and returns the corresponding QUrl. \a input is
    assumed to be in encoded form, containing only ASCII characters.

    Parses the URL using \a mode. See setUrl() for more information on
    this parameter. QUrl::DecodedMode is not permitted in this context.

    \note In Qt versions prior to 6.7, this function took a QByteArray, not
    QByteArrayView. If you experience compile errors, it's because your code
    is passing objects that are implicitly convertible to QByteArray, but not
    QByteArrayView. Wrap the corresponding argument in \c{QByteArray{~~~}} to
    make the cast explicit. This is backwards-compatible with old Qt versions.

    \sa toEncoded(), setUrl()
*/
QUrl QUrl::fromEncoded(QByteArrayView input, ParsingMode mode)
{
    return QUrl(QString::fromUtf8(input), mode);
}

/*!
    Returns a decoded copy of \a input. \a input is first decoded from
    percent encoding, then converted from UTF-8 to unicode.

    \note Given invalid input (such as a string containing the sequence "%G5",
    which is not a valid hexadecimal number) the output will be invalid as
    well. As an example: the sequence "%G5" could be decoded to 'W'.
*/
QString QUrl::fromPercentEncoding(const QByteArray &input)
{
    QByteArray ba = QByteArray::fromPercentEncoding(input);
    return QString::fromUtf8(ba);
}

/*!
    Returns an encoded copy of \a input. \a input is first converted
    to UTF-8, and all ASCII-characters that are not in the unreserved group
    are percent encoded. To prevent characters from being percent encoded
    pass them to \a exclude. To force characters to be percent encoded pass
    them to \a include.

    Unreserved is defined as:
       \tt {ALPHA / DIGIT / "-" / "." / "_" / "~"}

    \snippet code/src_corelib_io_qurl.cpp 6
*/
QByteArray QUrl::toPercentEncoding(const QString &input, const QByteArray &exclude, const QByteArray &include)
{
    return input.toUtf8().toPercentEncoding(exclude, include);
}

/*!
    \since 6.3

    Returns the Unicode form of the given domain name
    \a domain, which is encoded in the ASCII Compatible Encoding (ACE).
    The output can be customized by passing flags with \a options.
    The result of this function is considered equivalent to \a domain.

    If the value in \a domain cannot be encoded, it will be converted
    to QString and returned.

    The ASCII-Compatible Encoding (ACE) is defined by RFC 3490, RFC 3491
    and RFC 3492 and updated by the Unicode Technical Standard #46. It is part
    of the Internationalizing Domain Names in Applications (IDNA) specification,
    which allows for domain names (like \c "example.com") to be written using
    non-US-ASCII characters.
*/
QString QUrl::fromAce(const QByteArray &domain, QUrl::AceProcessingOptions options)
{
    return qt_ACE_do(QString::fromLatin1(domain), NormalizeAce,
                     ForbidLeadingDot /*FIXME: make configurable*/, options);
}

/*!
    \since 6.3

    Returns the ASCII Compatible Encoding of the given domain name \a domain.
    The output can be customized by passing flags with \a options.
    The result of this function is considered equivalent to \a domain.

    The ASCII-Compatible Encoding (ACE) is defined by RFC 3490, RFC 3491
    and RFC 3492 and updated by the Unicode Technical Standard #46. It is part
    of the Internationalizing Domain Names in Applications (IDNA) specification,
    which allows for domain names (like \c "example.com") to be written using
    non-US-ASCII characters.

    This function returns an empty QByteArray if \a domain is not a valid
    hostname. Note, in particular, that IPv6 literals are not valid domain
    names.
*/
QByteArray QUrl::toAce(const QString &domain, AceProcessingOptions options)
{
    return qt_ACE_do(domain, ToAceOnly, ForbidLeadingDot /*FIXME: make configurable*/, options)
            .toLatin1();
}

/*!
    \internal

    \fn bool QUrl::operator<(const QUrl &lhs, const QUrl &rhs)

    Returns \c true if URL \a lhs is "less than" URL \a rhs. This
    provides a means of ordering URLs.
*/

Qt::weak_ordering compareThreeWay(const QUrl &lhs, const QUrl &rhs)
{
    if (!lhs.d || !rhs.d) {
        bool thisIsEmpty = !lhs.d || lhs.d->isEmpty();
        bool thatIsEmpty = !rhs.d || rhs.d->isEmpty();

        // sort an empty URL first
        if (thisIsEmpty) {
            if (!thatIsEmpty)
                return Qt::weak_ordering::less;
            else
                return Qt::weak_ordering::equivalent;
        } else {
            return Qt::weak_ordering::greater;
        }
    }

    int cmp;
    cmp = lhs.d->scheme.compare(rhs.d->scheme);
    if (cmp != 0)
        return Qt::compareThreeWay(cmp, 0);

    cmp = lhs.d->userName.compare(rhs.d->userName);
    if (cmp != 0)
        return Qt::compareThreeWay(cmp, 0);

    cmp = lhs.d->password.compare(rhs.d->password);
    if (cmp != 0)
        return Qt::compareThreeWay(cmp, 0);

    cmp = lhs.d->host.compare(rhs.d->host);
    if (cmp != 0)
        return Qt::compareThreeWay(cmp, 0);

    if (lhs.d->port != rhs.d->port)
        return Qt::compareThreeWay(lhs.d->port, rhs.d->port);

    cmp = lhs.d->path.compare(rhs.d->path);
    if (cmp != 0)
        return Qt::compareThreeWay(cmp, 0);

    if (lhs.d->hasQuery() != rhs.d->hasQuery())
        return rhs.d->hasQuery() ? Qt::weak_ordering::less : Qt::weak_ordering::greater;

    cmp = lhs.d->query.compare(rhs.d->query);
    if (cmp != 0)
        return Qt::compareThreeWay(cmp, 0);

    if (lhs.d->hasFragment() != rhs.d->hasFragment())
        return rhs.d->hasFragment() ? Qt::weak_ordering::less : Qt::weak_ordering::greater;

    cmp = lhs.d->fragment.compare(rhs.d->fragment);
    return Qt::compareThreeWay(cmp, 0);
}

/*!
    \fn bool QUrl::operator==(const QUrl &lhs, const QUrl &rhs)

    Returns \c true if \a lhs and \a rhs URLs are equivalent;
    otherwise returns \c false.

    \sa matches()
*/

bool comparesEqual(const QUrl &lhs, const QUrl &rhs)
{
    if (!lhs.d && !rhs.d)
        return true;
    if (!lhs.d)
        return rhs.d->isEmpty();
    if (!rhs.d)
        return lhs.d->isEmpty();

    return (lhs.d->presentSections() == rhs.d->presentSections()) &&
            lhs.d->scheme == rhs.d->scheme &&
            lhs.d->userName == rhs.d->userName &&
            lhs.d->password == rhs.d->password &&
            lhs.d->host == rhs.d->host &&
            lhs.d->port == rhs.d->port &&
            lhs.d->path == rhs.d->path &&
            lhs.d->query == rhs.d->query &&
            lhs.d->fragment == rhs.d->fragment;
}

/*!
    \since 5.2

    Returns \c true if this URL and the given \a url are equal after
    applying \a options to both; otherwise returns \c false.

    This is equivalent to calling \l{adjusted()}{adjusted}(options) on both URLs
    and comparing the resulting urls, but faster.

*/
bool QUrl::matches(const QUrl &url, FormattingOptions options) const
{
    if (!d && !url.d)
        return true;
    if (!d)
        return url.d->isEmpty();
    if (!url.d)
        return d->isEmpty();

    uint mask = d->presentSections();

    if (options.testFlag(QUrl::RemoveScheme))
        mask &= ~QUrlPrivate::Scheme;
    else if (d->scheme != url.d->scheme)
        return false;

    if (options.testFlag(QUrl::RemovePassword))
        mask &= ~QUrlPrivate::Password;
    else if (d->password != url.d->password)
        return false;

    if (options.testFlag(QUrl::RemoveUserInfo))
        mask &= ~QUrlPrivate::UserName;
    else if (d->userName != url.d->userName)
        return false;

    if (options.testFlag(QUrl::RemovePort))
        mask &= ~QUrlPrivate::Port;
    else if (d->port != url.d->port)
        return false;

    if (options.testFlag(QUrl::RemoveAuthority))
        mask &= ~QUrlPrivate::Host;
    else if (d->host != url.d->host)
        return false;

    if (options.testFlag(QUrl::RemoveQuery))
        mask &= ~QUrlPrivate::Query;
    else if (d->query != url.d->query)
        return false;

    if (options.testFlag(QUrl::RemoveFragment))
        mask &= ~QUrlPrivate::Fragment;
    else if (d->fragment != url.d->fragment)
        return false;

    if ((d->sectionIsPresent & mask) != (url.d->sectionIsPresent & mask))
        return false;

    if (options.testFlag(QUrl::RemovePath))
        return true;

    // Compare paths, after applying path-related options
    QString path1;
    d->appendPath(path1, options, QUrlPrivate::Path);
    QString path2;
    url.d->appendPath(path2, options, QUrlPrivate::Path);
    return path1 == path2;
}

/*!
    \fn bool QUrl::operator !=(const QUrl &lhs, const QUrl &rhs)

    Returns \c true if \a lhs and \a rhs URLs are not equal;
    otherwise returns \c false.

    \sa matches()
*/

/*!
    Assigns the specified \a url to this object.
*/
QUrl &QUrl::operator =(const QUrl &url) noexcept
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
    detachToClear();
    if (!url.isEmpty())
        d->parse(url, TolerantMode);
    return *this;
}

/*!
    \fn void QUrl::swap(QUrl &other)
    \since 4.8
    \memberswap{URL}
*/

/*!
    \internal

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

    Forces a detach resulting in a clear state.
*/
void QUrl::detachToClear()
{
    if (d && (d->ref.loadAcquire() == 1 || !d->ref.deref())) {
        // we had the only copy
        d->ref.storeRelaxed(1);
        d->clear();
    } else {
        d = new QUrlPrivate;
    }
}

/*!
    \internal
*/
bool QUrl::isDetached() const
{
    return !d || d->ref.loadRelaxed() == 1;
}

static QString fromNativeSeparators(const QString &pathName)
{
#if defined(Q_OS_WIN)
    QString result(pathName);
    const QChar nativeSeparator = u'\\';
    auto i = result.indexOf(nativeSeparator);
    if (i != -1) {
        QChar * const data = result.data();
        const auto length = result.length();
        for (; i < length; ++i) {
            if (data[i] == nativeSeparator)
                data[i] = u'/';
        }
    }
    return result;
#else
    return pathName;
#endif
}

/*!
    Returns a QUrl representation of \a localFile, interpreted as a local
    file. This function accepts paths separated by slashes as well as the
    native separator for this platform.

    This function also accepts paths with a doubled leading slash (or
    backslash) to indicate a remote file, as in
    "//servername/path/to/file.txt". Note that only certain platforms can
    actually open this file using QFile::open().

    An empty \a localFile leads to an empty URL (since Qt 5.4).

    \snippet code/src_corelib_io_qurl.cpp 16

    In the first line in snippet above, a file URL is constructed from a
    local, relative path. A file URL with a relative path only makes sense
    if there is a base URL to resolve it against. For example:

    \snippet code/src_corelib_io_qurl.cpp 17

    To resolve such a URL, it's necessary to remove the scheme beforehand:

    \snippet code/src_corelib_io_qurl.cpp 18

    For this reason, it is better to use a relative URL (that is, no scheme)
    for relative file paths:

    \snippet code/src_corelib_io_qurl.cpp 19

    \sa toLocalFile(), isLocalFile(), QDir::toNativeSeparators()
*/
QUrl QUrl::fromLocalFile(const QString &localFile)
{
    QUrl url;
    QString deslashified = fromNativeSeparators(localFile);
    if (deslashified.isEmpty())
        return url;
    QString scheme = fileScheme();
    char16_t firstChar = deslashified.at(0).unicode();
    char16_t secondChar = deslashified.size() > 1 ? deslashified.at(1).unicode() : u'\0';

    // magic for drives on windows
    if (firstChar != u'/' && secondChar == u':') {
        deslashified.prepend(u'/');
        firstChar = u'/';
    } else if (firstChar == u'/' && secondChar == u'/') {
        // magic for shared drive on windows
        qsizetype indexOfPath = deslashified.indexOf(u'/', 2);
        QStringView hostSpec = QStringView{deslashified}.mid(2, indexOfPath - 2);
        // Check for Windows-specific WebDAV specification: "//host@SSL/path".
        if (hostSpec.endsWith(webDavSslTag(), Qt::CaseInsensitive)) {
            hostSpec.truncate(hostSpec.size() - 4);
            scheme = webDavScheme();
        }

        // hosts can't be IPv6 addresses without [], so we can use QUrlPrivate::setHost
        url.detach();
        if (!url.d->setHost(hostSpec.toString(), 0, hostSpec.size(), StrictMode)) {
            if (url.d->error->code != QUrlPrivate::InvalidRegNameError)
                return url;

            // Path hostname is not a valid URL host, so set it entirely in the path
            // (by leaving deslashified unchanged)
        } else if (indexOfPath > 2) {
            deslashified = deslashified.right(deslashified.size() - indexOfPath);
        } else {
            deslashified.clear();
        }
    }
    if (firstChar == u'/') {
        // ensure absolute file URLs have an empty authority to comply with the XDG file spec
        url.detach();
        url.d->sectionIsPresent |= QUrlPrivate::Host;
    }

    url.setScheme(scheme);
    url.setPath(deslashified, DecodedMode);

    return url;
}

/*!
    Returns the path of this URL formatted as a local file path. The path
    returned will use forward slashes, even if it was originally created
    from one with backslashes.

    If this URL contains a non-empty hostname, it will be encoded in the
    returned value in the form found on SMB networks (for example,
    "//servername/path/to/file.txt").

    \snippet code/src_corelib_io_qurl.cpp 20

    Note: if the path component of this URL contains a non-UTF-8 binary
    sequence (such as %80), the behaviour of this function is undefined.

    \sa fromLocalFile(), isLocalFile()
*/
QString QUrl::toLocalFile() const
{
    // the call to isLocalFile() also ensures that we're parsed
    if (!isLocalFile())
        return QString();

    return d->toLocalFile(QUrl::FullyDecoded);
}

/*!
    \since 4.8
    Returns \c true if this URL is pointing to a local file path. A URL is a
    local file path if the scheme is "file".

    Note that this function considers URLs with hostnames to be local file
    paths, even if the eventual file path cannot be opened with
    QFile::open().

    \sa fromLocalFile(), toLocalFile()
*/
bool QUrl::isLocalFile() const
{
    return d && d->isLocalFile();
}

/*!
    Returns \c true if this URL is a parent of \a childUrl. \a childUrl is a child
    of this URL if the two URLs share the same scheme and authority,
    and this URL's path is a parent of the path of \a childUrl.
*/
bool QUrl::isParentOf(const QUrl &childUrl) const
{
    QString childPath = childUrl.path();

    if (!d)
        return ((childUrl.scheme().isEmpty())
            && (childUrl.authority().isEmpty())
            && childPath.size() > 0 && childPath.at(0) == u'/');

    QString ourPath = path();

    return ((childUrl.scheme().isEmpty() || d->scheme == childUrl.scheme())
            && (childUrl.authority().isEmpty() || authority() == childUrl.authority())
            &&  childPath.startsWith(ourPath)
            && ((ourPath.endsWith(u'/') && childPath.size() > ourPath.size())
                || (!ourPath.endsWith(u'/') && childPath.size() > ourPath.size()
                    && childPath.at(ourPath.size()) == u'/')));
}


#ifndef QT_NO_DATASTREAM
/*! \relates QUrl

    Writes url \a url to the stream \a out and returns a reference
    to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator<<(QDataStream &out, const QUrl &url)
{
    QByteArray u;
    if (url.isValid())
        u = url.toEncoded();
    out << u;
    return out;
}

/*! \relates QUrl

    Reads a url into \a url from the stream \a in and returns a
    reference to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/
QDataStream &operator>>(QDataStream &in, QUrl &url)
{
    QByteArray u;
    in >> u;
    url.setUrl(QString::fromLatin1(u));
    return in;
}
#endif // QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QUrl &url)
{
    QDebugStateSaver saver(d);
    d.nospace() << "QUrl(" << url.toDisplayString() << ')';
    return d;
}
#endif

static QString errorMessage(QUrlPrivate::ErrorCode errorCode, const QString &errorSource, qsizetype errorPosition)
{
    QChar c = size_t(errorPosition) < size_t(errorSource.size()) ?
                errorSource.at(errorPosition) : QChar(QChar::Null);

    switch (errorCode) {
    case QUrlPrivate::NoError:
        Q_UNREACHABLE_RETURN(QString()); // QUrl::errorString should have treated this condition

    case QUrlPrivate::InvalidSchemeError: {
        auto msg = "Invalid scheme (character '%1' not permitted)"_L1;
        return msg.arg(c);
    }

    case QUrlPrivate::InvalidUserNameError:
        return "Invalid user name (character '%1' not permitted)"_L1
                .arg(c);

    case QUrlPrivate::InvalidPasswordError:
        return "Invalid password (character '%1' not permitted)"_L1
                .arg(c);

    case QUrlPrivate::InvalidRegNameError:
        if (errorPosition >= 0)
            return "Invalid hostname (character '%1' not permitted)"_L1
                    .arg(c);
        else
            return QStringLiteral("Invalid hostname (contains invalid characters)");
    case QUrlPrivate::InvalidIPv4AddressError:
        return QString(); // doesn't happen yet
    case QUrlPrivate::InvalidIPv6AddressError:
        return QStringLiteral("Invalid IPv6 address");
    case QUrlPrivate::InvalidCharacterInIPv6Error:
        return "Invalid IPv6 address (character '%1' not permitted)"_L1.arg(c);
    case QUrlPrivate::InvalidIPvFutureError:
        return "Invalid IPvFuture address (character '%1' not permitted)"_L1.arg(c);
    case QUrlPrivate::HostMissingEndBracket:
        return QStringLiteral("Expected ']' to match '[' in hostname");

    case QUrlPrivate::InvalidPortError:
        return QStringLiteral("Invalid port or port number out of range");
    case QUrlPrivate::PortEmptyError:
        return QStringLiteral("Port field was empty");

    case QUrlPrivate::InvalidPathError:
        return "Invalid path (character '%1' not permitted)"_L1
                .arg(c);

    case QUrlPrivate::InvalidQueryError:
        return "Invalid query (character '%1' not permitted)"_L1
                .arg(c);

    case QUrlPrivate::InvalidFragmentError:
        return "Invalid fragment (character '%1' not permitted)"_L1
                .arg(c);

    case QUrlPrivate::AuthorityPresentAndPathIsRelative:
        return QStringLiteral("Path component is relative and authority is present");
    case QUrlPrivate::AuthorityAbsentAndPathIsDoubleSlash:
        return QStringLiteral("Path component starts with '//' and authority is absent");
    case QUrlPrivate::RelativeUrlPathContainsColonBeforeSlash:
        return QStringLiteral("Relative URL's path component contains ':' before any '/'");
    }

    Q_UNREACHABLE_RETURN(QString());
}

static inline void appendComponentIfPresent(QString &msg, bool present, const char *componentName,
                                            const QString &component)
{
    if (present)
        msg += QLatin1StringView(componentName) % u'"' % component % "\","_L1;
}

/*!
    \since 4.2

    Returns an error message if the last operation that modified this QUrl
    object ran into a parsing error. If no error was detected, this function
    returns an empty string and isValid() returns \c true.

    The error message returned by this function is technical in nature and may
    not be understood by end users. It is mostly useful to developers trying to
    understand why QUrl will not accept some input.

    \sa QUrl::ParsingMode
*/
QString QUrl::errorString() const
{
    QString msg;
    if (!d)
        return msg;

    QString errorSource;
    qsizetype errorPosition = 0;
    QUrlPrivate::ErrorCode errorCode = d->validityError(&errorSource, &errorPosition);
    if (errorCode == QUrlPrivate::NoError)
        return msg;

    msg += errorMessage(errorCode, errorSource, errorPosition);
    msg += "; source was \""_L1;
    msg += errorSource;
    msg += "\";"_L1;
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Scheme,
                             " scheme = ", d->scheme);
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::UserInfo,
                             " userinfo = ", userInfo());
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Host,
                             " host = ", d->host);
    appendComponentIfPresent(msg, d->port != -1,
                             " port = ", QString::number(d->port));
    appendComponentIfPresent(msg, !d->path.isEmpty(),
                             " path = ", d->path);
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Query,
                             " query = ", d->query);
    appendComponentIfPresent(msg, d->sectionIsPresent & QUrlPrivate::Fragment,
                             " fragment = ", d->fragment);
    if (msg.endsWith(u','))
        msg.chop(1);
    return msg;
}

/*!
    \since 5.1

    Converts a list of \a urls into a list of QString objects, using toString(\a options).
*/
QStringList QUrl::toStringList(const QList<QUrl> &urls, FormattingOptions options)
{
    QStringList lst;
    lst.reserve(urls.size());
    for (const QUrl &url : urls)
        lst.append(url.toString(options));
    return lst;

}

/*!
    \since 5.1

    Converts a list of strings representing \a urls into a list of urls, using QUrl(str, \a mode).
    Note that this means all strings must be urls, not for instance local paths.
*/
QList<QUrl> QUrl::fromStringList(const QStringList &urls, ParsingMode mode)
{
    QList<QUrl> lst;
    lst.reserve(urls.size());
    for (const QString &str : urls)
        lst.append(QUrl(str, mode));
    return lst;
}

/*!
    \typedef QUrl::DataPtr
    \internal
*/

/*!
    \fn DataPtr &QUrl::data_ptr()
    \internal
*/

/*!
    \fn size_t qHash(const QUrl &key, size_t seed)
    \qhashold{QHash}
    \since 5.0
*/
size_t qHash(const QUrl &url, size_t seed) noexcept
{
    QtPrivate::QHashCombineWithSeed hasher(seed);

    // non-commutative, we must hash the port first
    if (!url.d)
        return hasher(0, -1);
    size_t state = hasher(0, url.d->port);

    if (url.d->hasScheme())
        state = hasher(state, url.d->scheme);
    if (url.d->hasUserInfo()) {
        // see presentSections(), appendUserName(), etc.
        state = hasher(state, url.d->userName);
        state = hasher(state, url.d->password);
    }
    if (url.d->hasHost() || url.d->isLocalFile())   // for XDG compatibility
        state = hasher(state, url.d->host);
    if (url.d->hasPath())
        state = hasher(state, url.d->path);
    if (url.d->hasQuery())
        state = hasher(state, url.d->query);
    if (url.d->hasFragment())
        state = hasher(state, url.d->fragment);
    return state;
}

static QUrl adjustFtpPath(QUrl url)
{
    if (url.scheme() == ftpScheme()) {
        QString path = url.path(QUrl::PrettyDecoded);
        if (path.startsWith("//"_L1))
            url.setPath("/%2F"_L1 + QStringView{path}.mid(2), QUrl::TolerantMode);
    }
    return url;
}

static bool isIp6(const QString &text)
{
    QIPAddressUtils::IPv6Address address;
    return !text.isEmpty() && QIPAddressUtils::parseIp6(address, text.begin(), text.end()) == nullptr;
}

/*!
    Returns a valid URL from a user supplied \a userInput string if one can be
    deduced. In the case that is not possible, an invalid QUrl() is returned.

    This allows the user to input a URL or a local file path in the form of a plain
    string. This string can be manually typed into a location bar, obtained from
    the clipboard, or passed in via command line arguments.

    When the string is not already a valid URL, a best guess is performed,
    making various assumptions.

    In the case the string corresponds to a valid file path on the system,
    a file:// URL is constructed, using QUrl::fromLocalFile().

    If that is not the case, an attempt is made to turn the string into a
    http:// or ftp:// URL. The latter in the case the string starts with
    'ftp'. The result is then passed through QUrl's tolerant parser, and
    in the case or success, a valid QUrl is returned, or else a QUrl().

    \section1 Examples:

    \list
    \li qt-project.org becomes http://qt-project.org
    \li ftp.qt-project.org becomes ftp://ftp.qt-project.org
    \li hostname becomes http://hostname
    \li /home/user/test.html becomes file:///home/user/test.html
    \endlist

    In order to be able to handle relative paths, this method takes an optional
    \a workingDirectory path. This is especially useful when handling command
    line arguments.
    If \a workingDirectory is empty, no handling of relative paths will be done.

    By default, an input string that looks like a relative path will only be treated
    as such if the file actually exists in the given working directory.
    If the application can handle files that don't exist yet, it should pass the
    flag AssumeLocalFile in \a options.

    \since 5.4
*/
QUrl QUrl::fromUserInput(const QString &userInput, const QString &workingDirectory,
                         UserInputResolutionOptions options)
{
    QString trimmedString = userInput.trimmed();

    if (trimmedString.isEmpty())
        return QUrl();

    // Check for IPv6 addresses, since a path starting with ":" is absolute (a resource)
    // and IPv6 addresses can start with "c:" too
    if (isIp6(trimmedString)) {
        QUrl url;
        url.setHost(trimmedString);
        url.setScheme(QStringLiteral("http"));
        return url;
    }

    const QUrl url = QUrl(trimmedString, QUrl::TolerantMode);

    // Check for a relative path
    if (!workingDirectory.isEmpty()) {
        const QFileInfo fileInfo(QDir(workingDirectory), userInput);
        if (fileInfo.exists())
            return QUrl::fromLocalFile(fileInfo.absoluteFilePath());

        // Check both QUrl::isRelative (to detect full URLs) and QDir::isAbsolutePath (since on Windows drive letters can be interpreted as schemes)
        if ((options & AssumeLocalFile) && url.isRelative() && !QDir::isAbsolutePath(userInput))
            return QUrl::fromLocalFile(fileInfo.absoluteFilePath());
    }

    // Check first for files, since on Windows drive letters can be interpreted as schemes
    if (QDir::isAbsolutePath(trimmedString))
        return QUrl::fromLocalFile(trimmedString);

    QUrl urlPrepended = QUrl("http://"_L1 + trimmedString, QUrl::TolerantMode);

    // Check the most common case of a valid url with a scheme
    // We check if the port would be valid by adding the scheme to handle the case host:port
    // where the host would be interpreted as the scheme
    if (url.isValid()
        && !url.scheme().isEmpty()
        && urlPrepended.port() == -1)
        return adjustFtpPath(url);

    // Else, try the prepended one and adjust the scheme from the host name
    if (urlPrepended.isValid() && (!urlPrepended.host().isEmpty() || !urlPrepended.path().isEmpty())) {
        qsizetype dotIndex = trimmedString.indexOf(u'.');
        const QStringView hostscheme = QStringView{trimmedString}.left(dotIndex);
        if (hostscheme.compare(ftpScheme(), Qt::CaseInsensitive) == 0)
            urlPrepended.setScheme(ftpScheme());
        return adjustFtpPath(urlPrepended);
    }

    return QUrl();
}

QT_END_NAMESPACE
