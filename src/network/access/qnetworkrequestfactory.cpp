// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qnetworkrequestfactory.h"
#include "qnetworkrequestfactory_p.h"

#if QT_CONFIG(ssl)
#include <QtNetwork/qsslconfiguration.h>
#endif

#include <QtCore/qloggingcategory.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

QT_DEFINE_QESDP_SPECIALIZATION_DTOR(QNetworkRequestFactoryPrivate)

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQrequestfactory, "qt.network.access.request.factory")

/*!
    \class QNetworkRequestFactory
    \since 6.7
    \ingroup shared
    \inmodule QtNetwork

    \brief Convenience class for grouping remote server endpoints that share
    common network request properties.

    \preliminary

    REST servers often have endpoints that require the same headers and other data.
    Grouping such endpoints with a QNetworkRequestFactory makes it more
    convenient to issue requests to these endpoints; only the typically
    varying parts such as \e path and \e query parameters are provided
    when creating a new request.

    Basic usage steps of QNetworkRequestFactory are as follows:
    \list
        \li Instantiation
        \li Setting the data common to all requests
        \li Issuing requests
    \endlist

    An example of usage:

    \snippet code/src_network_access_qnetworkrequestfactory.cpp 0
*/

/*!
    Creates a new QNetworkRequestFactory object.
    Use setBaseUrl() to set a valid base URL for the requests.

    \sa QNetworkRequestFactory(const QUrl &baseUrl), setBaseUrl()
*/

QNetworkRequestFactory::QNetworkRequestFactory()
    : d(new QNetworkRequestFactoryPrivate)
{
}

/*!
    Creates a new QNetworkRequestFactory object, initializing the base URL to
    \a baseUrl. The base URL is used to populate subsequent network
    requests.

    If the URL contains a \e path component, it will be extracted and used
    as a base path in subsequent network requests. This means that any
    paths provided when requesting individual requests will be appended
    to this base path, as illustrated below:

    \snippet code/src_network_access_qnetworkrequestfactory.cpp 1
 */
QNetworkRequestFactory::QNetworkRequestFactory(const QUrl &baseUrl)
    : d(new QNetworkRequestFactoryPrivate(baseUrl))
{
}

/*!
    Destroys this QNetworkRequestFactory object.
 */
QNetworkRequestFactory::~QNetworkRequestFactory()
    = default;

/*!
    Creates a copy of \a other.
 */
QNetworkRequestFactory::QNetworkRequestFactory(const QNetworkRequestFactory &other)
    = default;

/*!
    Creates a copy of \a other and returns a reference to this factory.
 */
QNetworkRequestFactory &QNetworkRequestFactory::operator=(const QNetworkRequestFactory &other)
    = default;

/*!
    \fn QNetworkRequestFactory::QNetworkRequestFactory(QNetworkRequestFactory &&other) noexcept

    Move-constructs the factory from \a other.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
*/

/*!
    \fn QNetworkRequestFactory &QNetworkRequestFactory::operator=(QNetworkRequestFactory &&other) noexcept

    Move-assigns \a other and returns a reference to this factory.

    \note The moved-from object \a other is placed in a
    partially-formed state, in which the only valid operations are
    destruction and assignment of a new value.
 */

/*!
    \fn void QNetworkRequestFactory::swap(QNetworkRequestFactory &other)

    Swaps this factory with \a other. This operation is
    very fast and never fails.
 */

/*!
    Returns the base URL used for the individual requests.

    The base URL may contain a path component. This path is used
    as path "prefix" for the paths that are provided when generating
    individual requests.

    \sa setBaseUrl()
 */
QUrl QNetworkRequestFactory::baseUrl() const
{
    return d->baseUrl;
}

/*!
    Sets the base URL used in individual requests to \a url.

    \sa baseUrl()
 */
void QNetworkRequestFactory::setBaseUrl(const QUrl &url)
{
    if (d->baseUrl == url)
        return;

    d.detach();
    d->baseUrl = url;
}

#if QT_CONFIG(ssl)
/*!
    Returns the SSL configuration set to this factory. The SSL configuration
    is set to each individual request.

    \sa setSslConfiguration()
 */
QSslConfiguration QNetworkRequestFactory::sslConfiguration() const
{
    return d->sslConfig;
}

/*!
    Sets the SSL configuration to \a configuration.

    \sa sslConfiguration()
 */
void QNetworkRequestFactory::setSslConfiguration(const QSslConfiguration &configuration)
{
    if (d->sslConfig == configuration)
        return;

    d.detach();
    d->sslConfig = configuration;
}
#endif

/*!
    Returns a QNetworkRequest.

    The returned request is filled with the data that this factory
    has been configured with.

    \sa createRequest(const QUrlQuery&), createRequest(const QString&, const QUrlQuery&)
*/

QNetworkRequest QNetworkRequestFactory::createRequest() const
{
    return d->newRequest(d->requestUrl());
}

/*!
    Returns a QNetworkRequest.

    The returned request's URL is formed by appending the provided \a path
    to the baseUrl (which may itself have a path component).

    \sa createRequest(const QString &, const QUrlQuery &), createRequest(), baseUrl()
*/
QNetworkRequest QNetworkRequestFactory::createRequest(const QString &path) const
{
    return d->newRequest(d->requestUrl(&path));
}

/*!
    Returns a QNetworkRequest.

    The returned request's URL is formed by appending the provided \a query
    to the baseUrl.

    \sa createRequest(const QString &, const QUrlQuery &), createRequest(), baseUrl()
*/
QNetworkRequest QNetworkRequestFactory::createRequest(const QUrlQuery &query) const
{
    return d->newRequest(d->requestUrl(nullptr, &query));
}

/*!
    Returns a QNetworkRequest.

    The returned requests URL is formed by appending the provided \a path
    and \a query to the baseUrl (which may have a path component).

    If the provided \a path contains query items, they will be combined
    with the items in \a query.

    \sa createRequest(const QUrlQuery&), createRequest(), baseUrl()
 */
QNetworkRequest QNetworkRequestFactory::createRequest(const QString &path, const QUrlQuery &query) const
{
    return d->newRequest(d->requestUrl(&path, &query));
}

/*!
    Sets \a headers that are common to all requests.

    These headers are added to individual requests' headers.
    This is a convenience mechanism for setting headers that
    repeat across requests.

    \sa commonHeaders(), clearCommonHeaders(), createRequest()
 */
void QNetworkRequestFactory::setCommonHeaders(const QHttpHeaders &headers)
{
    d.detach();
    d->headers = headers;
}

/*!
    Returns the currently set headers.

    \sa setCommonHeaders(), clearCommonHeaders()
 */
QHttpHeaders QNetworkRequestFactory::commonHeaders() const
{
    return d->headers;
}

/*!
    Clears current headers.

    \sa commonHeaders(), setCommonHeaders()
*/
void QNetworkRequestFactory::clearCommonHeaders()
{
    if (d->headers.isEmpty())
        return;
    d.detach();
    d->headers.clear();
}

/*!
    Returns the bearer token that has been set.

    The bearer token, if present, is used to set the
    \c {Authorization: Bearer my_token} header for requests. This is a common
    authorization convention and is provided as an additional convenience.

    The means to acquire the bearer token vary. Standard methods include \c OAuth2
    and the service provider's website/dashboard. It is expected that the bearer
    token changes over time. For example, when updated with a refresh token,
    always setting the new token again ensures that subsequent requests have
    the latest, valid token.

    The presence of the bearer token does not impact the \l commonHeaders()
    listing. If the \l commonHeaders() also lists \c Authorization header, it
    will be overwritten.

    \sa setBearerToken(), commonHeaders()
 */
QByteArray QNetworkRequestFactory::bearerToken() const
{
    return d->bearerToken;
}

/*!
    Sets the bearer token to \a token.

    \sa bearerToken(), clearBearerToken()
*/
void QNetworkRequestFactory::setBearerToken(const QByteArray &token)
{
    if (d->bearerToken == token)
        return;

    d.detach();
    d->bearerToken = token;
}

/*!
    Clears the bearer token.

    \sa bearerToken()
*/
void QNetworkRequestFactory::clearBearerToken()
{
    if (d->bearerToken.isEmpty())
        return;

    d.detach();
    d->bearerToken.clear();
}

/*!
    Returns the username set to this factory.

    \sa setUserName(), clearUserName(), password()
*/
QString QNetworkRequestFactory::userName() const
{
    return d->userName;
}

/*!
    Sets the username of this factory to \a userName.

    The username is set in the request URL when \l createRequest() is called.
    The QRestAccessManager / QNetworkAccessManager will attempt to use
    these credentials when the server indicates that authentication
    is required.

    \sa userName(), clearUserName(), password()
*/
void QNetworkRequestFactory::setUserName(const QString &userName)
{
    if (d->userName == userName)
        return;
    d.detach();
    d->userName = userName;
}

/*!
    Clears the username set to this factory.
*/
void QNetworkRequestFactory::clearUserName()
{
    if (d->userName.isEmpty())
        return;
    d.detach();
    d->userName.clear();
}

/*!
    Returns the password set to this factory.

    \sa password(), clearPassword(), userName()
*/
QString QNetworkRequestFactory::password() const
{
    return d->password;
}

/*!
    Sets the password of this factory to \a password.

    The password is set in the request URL when \l createRequest() is called.
    The QRestAccessManager / QNetworkAccessManager will attempt to use
    these credentials when the server indicates that authentication
    is required.

    \sa password(), clearPassword(), userName()
*/
void QNetworkRequestFactory::setPassword(const QString &password)
{
    if (d->password == password)
        return;
    d.detach();
    d->password = password;
}

/*!
    Clears the password set to this factory.

    \sa password(), setPassword(), userName()
*/
void QNetworkRequestFactory::clearPassword()
{
    if (d->password.isEmpty())
        return;
    d.detach();
    d->password.clear();
}

/*!
    Sets \a timeout used for transfers.

    \sa transferTimeout(), QNetworkRequest::setTransferTimeout(),
        QNetworkAccessManager::setTransferTimeout()
*/
void QNetworkRequestFactory::setTransferTimeout(std::chrono::milliseconds timeout)
{
    if (d->transferTimeout == timeout)
        return;

    d.detach();
    d->transferTimeout = timeout;
}

/*!
    Returns the timeout used for transfers.

    \sa setTransferTimeout(), QNetworkRequest::transferTimeout(),
        QNetworkAccessManager::transferTimeout()
*/
std::chrono::milliseconds QNetworkRequestFactory::transferTimeout() const
{
    return d->transferTimeout;
}

/*!
    Returns query parameters that are added to individual requests' query
    parameters. The query parameters are added to any potential query
    parameters provided with the individual \l createRequest() calls.

    Use cases for using repeating query parameters are server dependent,
    but typical examples include language setting \c {?lang=en}, format
    specification \c {?format=json}, API version specification
    \c {?version=1.0} and API key authentication.

    \sa setQueryParameters(), clearQueryParameters(), createRequest()
*/
QUrlQuery QNetworkRequestFactory::queryParameters() const
{
    return d->queryParameters;
}

/*!
    Sets \a query parameters that are added to individual requests' query
    parameters.

    \sa queryParameters(), clearQueryParameters()
 */
void QNetworkRequestFactory::setQueryParameters(const QUrlQuery &query)
{
    if (d->queryParameters == query)
        return;

    d.detach();
    d->queryParameters = query;
}

/*!
    Clears the query parameters.

    \sa queryParameters()
*/
void QNetworkRequestFactory::clearQueryParameters()
{
    if (d->queryParameters.isEmpty())
        return;

    d.detach();
    d->queryParameters.clear();
}

QNetworkRequestFactoryPrivate::QNetworkRequestFactoryPrivate()
    = default;

QNetworkRequestFactoryPrivate::QNetworkRequestFactoryPrivate(const QUrl &baseUrl)
    : baseUrl(baseUrl)
{
}

QNetworkRequestFactoryPrivate::~QNetworkRequestFactoryPrivate()
    = default;

QNetworkRequest QNetworkRequestFactoryPrivate::newRequest(const QUrl &url) const
{
    QNetworkRequest request;
    request.setUrl(url);
#if QT_CONFIG(ssl)
    if (!sslConfig.isNull())
        request.setSslConfiguration(sslConfig);
#endif
    // Set the header entries to the request. Combine values as there
    // may be multiple values per name. Note: this would not necessarily
    // produce right result for 'Set-Cookie' header if it has multiple values,
    // but since it is a purely server-side (response) header, not relevant here.
    const auto headerNames = headers.toMultiMap().uniqueKeys(); // ### fixme: port QNR to QHH
    for (const auto &name : headerNames)
        request.setRawHeader(name, headers.combinedValue(name));

    constexpr char Bearer[] = "Bearer ";
    if (!bearerToken.isEmpty())
        request.setRawHeader("Authorization"_ba, Bearer + bearerToken);

    request.setTransferTimeout(transferTimeout);
    return request;
}

QUrl QNetworkRequestFactoryPrivate::requestUrl(const QString *path,
                                             const QUrlQuery *query) const
{
    const QUrl providedPath = path ? QUrl(*path) : QUrl{};
    const QUrlQuery providedQuery = query ? *query : QUrlQuery();

    if (!providedPath.scheme().isEmpty() || !providedPath.host().isEmpty()) {
        qCWarning(lcQrequestfactory, "The provided path %ls may only contain path and query item "
                  "components, and other parts will be ignored. Set the baseUrl instead",
                  qUtf16Printable(providedPath.toDisplayString()));
    }

    QUrl resultUrl = baseUrl;
    QUrlQuery resultQuery(providedQuery);
    QString basePath = baseUrl.path();

    resultUrl.setUserName(userName);
    resultUrl.setPassword(password);

    // Separate the path and query parameters components on the application-provided path
    const QString requestPath{providedPath.path()};
    const QUrlQuery pathQueryItems{providedPath};

    if (!pathQueryItems.isEmpty()) {
        // Add any query items provided as part of the path
        const auto items = pathQueryItems.queryItems(QUrl::ComponentFormattingOption::FullyEncoded);
        for (const auto &[key, value]: items)
            resultQuery.addQueryItem(key, value);
    }

    if (!queryParameters.isEmpty()) {
        // Add any query items set to this factory
        const QList<std::pair<QString,QString>> items =
                queryParameters.queryItems(QUrl::ComponentFormattingOption::FullyEncoded);
        for (const auto &item: items)
            resultQuery.addQueryItem(item.first, item.second);
    }

    if (!resultQuery.isEmpty())
        resultUrl.setQuery(resultQuery);

    if (requestPath.isEmpty())
        return resultUrl;

    // Ensure that the "base path" (the path that may be present
    // in the baseUrl), and the request path are joined with one '/'
    // If both have it, remove one, if neither has it, add one
    if (basePath.endsWith(u'/') && requestPath.startsWith(u'/'))
        basePath.chop(1);
    else if (!requestPath.startsWith(u'/') && !basePath.endsWith(u'/'))
        basePath.append(u'/');

    resultUrl.setPath(basePath.append(requestPath));
    return resultUrl;
}

#ifndef QT_NO_DEBUG_STREAM
/*!
    \fn QDebug QNetworkRequestFactory::operator<<(QDebug debug,
                                        const QNetworkRequestFactory &factory)

    Writes \a factory into \a debug stream.

    \sa {Debugging Techniques}
*/
QDebug operator<<(QDebug debug, const QNetworkRequestFactory &factory)
{
    const QDebugStateSaver saver(debug);
    debug.resetFormat().nospace();

    debug << "QNetworkRequestFactory(baseUrl = " << factory.baseUrl()
          << ", headers = " << factory.commonHeaders()
          << ", queryParameters = " << factory.queryParameters().queryItems()
          << ", bearerToken = " << (factory.bearerToken().isEmpty() ? "(empty)" : "(is set)")
          << ", transferTimeout = " << factory.transferTimeout()
          << ", userName = " << (factory.userName().isEmpty() ? "(empty)" : "(is set)")
          << ", password = " << (factory.password().isEmpty() ? "(empty)" : "(is set)")
#if QT_CONFIG(ssl)
          << ", SSL configuration"
          << (factory.sslConfiguration().isNull() ? " is not set (default)" : " is set")
#else
          << ", no SSL support"
#endif
          << ")";
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

QT_END_NAMESPACE
