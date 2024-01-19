// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qrestaccessmanager.h"
#include "qrestaccessmanager_p.h"
#include "qrestreply.h"

#include <QtNetwork/qhttpmultipart.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lcQrest, "qt.network.access.rest")

/*!

    \class QRestAccessManager
    \brief The QRestAccessManager is a networking convenience class for RESTful
    client applications.
    \since 6.7

    \ingroup network
    \inmodule QtNetwork
    \reentrant

    \preliminary

    QRestAccessManager provides a networking API for typical REST client
    applications. It provides the means to issue HTTP requests such as GET
    and POST. The responses to these requests can be handled with traditional
    Qt signal and slot mechanisms, as well as by providing callbacks
    directly - see \l {Issuing Network Requests and Handling Replies}.

    The class is a wrapper on top of QNetworkAccessManager, and it both amends
    convenience methods and omits typically less used features. These
    features are still accessible by configuring the underlying
    QNetworkAccessManager directly. QRestAccessManager is closely related to
    the QRestReply class, which it returns when issuing network requests.

    QRestAccessManager and related QRestReply classes can only be used in the
    thread they live in. For further information see
    \l {QObject#Thread Affinity}{QObject thread affinity} documentation.

    \section1 Issuing Network Requests and Handling Replies

    Network requests are initiated with a function call corresponding to
    the desired HTTP method, such as \c get() and \c post().

    \section2 Using Signals and Slots

    The function returns a QRestReply* object, whose signals can be used
    to follow up on the completion of the request in a traditional
    Qt-signals-and-slots way.

    Here's an example of how you could send a GET request and handle the
    response:

    \snippet code/src_network_access_qrestaccessmanager.cpp 0

    \section2 Using Callbacks and Context Objects

    The functions also take a context object of QObject (subclass) type
    and a callback function as parameters. The callback takes one QRestReply*
    as a parameter. The callback can be any callable, incl. a
    pointer-to-member-function..

    These callbacks are invoked when the QRestReply has finished processing
    (also in the case the processing finished due to an error).

    The context object can be \c nullptr, although, generally speaking,
    this is discouraged. Using a valid context object ensures that if the
    context object is destroyed during request processing, the callback will
    not be called. Stray callbacks which access a destroyed context is a source
    of application misbehavior.

    Here's an example of how you could send a GET request and check the
    response:

    \snippet code/src_network_access_qrestaccessmanager.cpp 1

    Many of the functions take in data for sending to a server. The data is
    supplied as the second parameter after the request.

    Here's an example of how you could send a POST request and check the
    response:

    \snippet code/src_network_access_qrestaccessmanager.cpp 2

    \section2 Supported data types

    The following table summarizes the methods and the supported data types.
    \c X means support.

    \table
    \header
        \li Data type
        \li \c get()
        \li \c post()
        \li \c put()
        \li \c head()
        \li \c patch()
        \li \c deleteResource()
        \li \c sendCustomRequest()
    \row
        \li No data
        \li X
        \li -
        \li -
        \li X
        \li -
        \li X
        \li -
    \row
        \li QByteArray
        \li X
        \li X
        \li X
        \li -
        \li X
        \li -
        \li X
    \row
        \li QJsonObject *)
        \li X
        \li X
        \li X
        \li -
        \li X
        \li -
        \li -
    \row
        \li QJsonArray *)
        \li -
        \li X
        \li X
        \li -
        \li X
        \li -
        \li -
    \row
        \li QVariantMap **)
        \li -
        \li X
        \li X
        \li -
        \li X
        \li -
        \li -
    \row
        \li QHttpMultiPart
        \li -
        \li X
        \li X
        \li -
        \li -
        \li -
        \li X
    \row
        \li QIODevice
        \li X
        \li X
        \li X
        \li -
        \li X
        \li -
        \li X
    \endtable

    *) QJsonObject and QJsonArray are sent in \l QJsonDocument::Compact format,
       and the \c Content-Type header is set to \c {application/json} if the
       \c Content-Type header was not set

    **) QVariantMap is converted to and treated as a QJsonObject

    \sa QRestReply, QNetworkRequestFactory, QNetworkAccessManager
*/

/*!
    \fn void QRestAccessManager::authenticationRequired(QRestReply *reply,
                                            QAuthenticator *authenticator)

    This signal is emitted when the final server requires authentication.
    The authentication relates to the provided \a reply instance, and any
    credentials are to be filled in the provided \a authenticator instance.

    See \l QNetworkAccessManager::authenticationRequired() for details.
*/

/*!
    \fn void QRestAccessManager::proxyAuthenticationRequired(
                     const QNetworkProxy &proxy, QAuthenticator *authenticator)

    This signal is emitted when a proxy authentication requires action.
    The proxy details are in \a proxy object, and any credentials are
    to be filled in the provided \a authenticator object.

    See \l QNetworkAccessManager::proxyAuthenticationRequired() for details.
*/

/*!
    \fn void QRestAccessManager::requestFinished(QRestReply *reply)

    This signal is emitted whenever a pending network reply is
    finished. \a reply parameter will contain a pointer to the
    reply that has just finished. This signal is emitted in tandem
    with the QRestReply::finished() signal. QRestReply provides
    functions for checking the status of the request, as well as for
    acquiring any received data.

    \note Do not delete \a reply object in the slot connected to this
    signal. Use deleteLater() if needed. See also \l deletesRepliesOnFinished().

    \sa QRestReply::finished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::get(
                    const QNetworkRequest &request,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP GET} based on \a request.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 3

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    \sa QRestReply, QRestReply::finished(),
        QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::get(
                    const QNetworkRequest &request, const QByteArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP GET} based on \a request and provided \a data.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 4

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    \sa QRestReply, QRestReply::finished(),
        QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::get(
                    const QNetworkRequest &request, const QJsonObject &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::get(
                    const QNetworkRequest &request, QIODevice *data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::post(
                    const QNetworkRequest &request, const QJsonObject &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP POST} based on \a request.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 5

    Alternatively, the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    The \c post() method always requires \a data parameter. The following
    data types are supported:
    \list
        \li QByteArray
        \li QJsonObject *)
        \li QJsonArray *)
        \li QVariantMap **)
        \li QHttpMultiPart*
        \li QIODevice*
    \endlist

    *) Sent in \l QJsonDocument::Compact format, and the
    \c Content-Type header is set to \c {application/json} if the
    \c Content-Type header was not set
    **) QVariantMap is converted to and treated as a QJsonObject

    \sa QRestReply, QRestReply::finished(),
        QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::post(
                    const QNetworkRequest &request, const QJsonArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::post(
                    const QNetworkRequest &request, const QVariantMap &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::post(
                    const QNetworkRequest &request, const QByteArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::post(
                    const QNetworkRequest &request, QHttpMultiPart *data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::post(
                    const QNetworkRequest &request, QIODevice *data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::put(
                    const QNetworkRequest &request, const QJsonObject &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP PUT} based on \a request.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 6

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    The \c put() method always requires \a data parameter. The following
    data types are supported:
    \list
        \li QByteArray
        \li QJsonObject *)
        \li QJsonArray *)
        \li QVariantMap **)
        \li QHttpMultiPart*
        \li QIODevice*
    \endlist

    *) Sent in \l QJsonDocument::Compact format, and the
    \c Content-Type header is set to \c {application/json}  if the
    \c Content-Type header was not set
    **) QVariantMap is converted to and treated as a QJsonObject

    \sa QRestReply, QRestReply::finished(), QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::put(
                    const QNetworkRequest &request, const QJsonArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::put(
                    const QNetworkRequest &request, const QVariantMap &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::put(
                    const QNetworkRequest &request, const QByteArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::put(
                    const QNetworkRequest &request, QHttpMultiPart *data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::put(
                    const QNetworkRequest &request, QIODevice *data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::patch(
                    const QNetworkRequest &request, const QJsonObject &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP PATCH} based on \a request.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 10

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    The \c patch() method always requires \a data parameter. The following
    data types are supported:
    \list
        \li QByteArray
        \li QJsonObject *)
        \li QJsonArray *)
        \li QVariantMap **)
        \li QIODevice*
    \endlist

    *) Sent in \l QJsonDocument::Compact format, and the
    \c Content-Type header is set to \c {application/json}  if the
    \c Content-Type header was not set
    **) QVariantMap is converted to and treated as a QJsonObject

    \sa QRestReply, QRestReply::finished(), QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::patch(
                    const QNetworkRequest &request, const QJsonArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::patch(
                    const QNetworkRequest &request, const QVariantMap &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::patch(
                    const QNetworkRequest &request, const QByteArray &data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::patch(
                    const QNetworkRequest &request, QIODevice *data,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::head(
                    const QNetworkRequest &request,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP HEAD} based on \a request.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 7

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    \c head() request does not support providing data.

    \sa QRestReply, QRestReply::finished(),
        QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::deleteResource(
                    const QNetworkRequest &request,
                    const ContextTypeForFunctor<Functor> *context,
                    Functor &&callback)

    Issues an \c {HTTP DELETE} based on \a request.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 8

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

    \c deleteResource() request does not support providing data.

    \sa QRestReply, QRestReply::finished(),
        QRestAccessManager::requestFinished()
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::sendCustomRequest(
            const QNetworkRequest& request, const QByteArray &method, const QByteArray &data,
            const ContextTypeForFunctor<Functor> *context,
            Functor &&callback)

    Issues \a request based HTTP request with custom \a method and the
    provided \a data.

    The optional \a callback and \a context object can be provided for
    handling the request completion as illustrated below:

    \snippet code/src_network_access_qrestaccessmanager.cpp 9

    Alternatively the signals of the returned QRestReply* object can be
    used. For further information see
    \l {Issuing Network Requests and Handling Replies}.

*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::sendCustomRequest(
            const QNetworkRequest& request, const QByteArray &method, QIODevice *data,
            const ContextTypeForFunctor<Functor> *context,
            Functor &&callback)

    \overload
*/

/*!
    \fn template<typename Functor, QRestAccessManager::if_compatible_callback<Functor>> QRestReply *QRestAccessManager::sendCustomRequest(
            const QNetworkRequest& request, const QByteArray &method, QHttpMultiPart *data,
            const ContextTypeForFunctor<Functor> *context,
            Functor &&callback)

    \overload
*/

QRestAccessManager::QRestAccessManager(QNetworkAccessManager *manager, QObject *parent)
    : QObject(*new QRestAccessManagerPrivate, parent)
{
    Q_D(QRestAccessManager);
    d->qnam = manager;
    if (!d->qnam)
        qCWarning(lcQrest, "QRestAccessManager: QNetworkAccesManager is nullptr");
}

/*!
    Destroys the QRestAccessManager object and frees up any
    resources, including any unfinished QRestReply objects.
*/
QRestAccessManager::~QRestAccessManager()
    = default;

/*!
    Returns the underlying QNetworkAccessManager instance. The instance
    can be used for accessing less-frequently used features and configurations.

    \sa QNetworkAccessManager
*/
QNetworkAccessManager *QRestAccessManager::networkAccessManager() const
{
    Q_D(const QRestAccessManager);
    return d->qnam;
}

QRestAccessManagerPrivate::QRestAccessManagerPrivate()
    = default;

QRestAccessManagerPrivate::~QRestAccessManagerPrivate()
{
    if (!activeRequests.isEmpty()) {
        qCWarning(lcQrest, "Access manager destroyed while %lld requests were still in progress",
                  qlonglong(activeRequests.size()));
    }
}

QNetworkReply *QRestAccessManager::postWithDataImpl(const QNetworkRequest &request,
                                                 const QJsonObject &data, const QObject *context,
                                                 QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&](auto req, auto json) { return d->qnam->post(req, json); },
                             data, request, context, slot);
}

QNetworkReply *QRestAccessManager::postWithDataImpl(const QNetworkRequest &request,
                                                 const QJsonArray &data, const QObject *context,
                                                 QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&](auto req, auto json) { return d->qnam->post(req, json); },
                             data, request, context, slot);
}

QNetworkReply *QRestAccessManager::postWithDataImpl(const QNetworkRequest &request,
                                                 const QVariantMap &data, const QObject *context,
                                                 QtPrivate::QSlotObjectBase *slot)
{
    return postWithDataImpl(request, QJsonObject::fromVariantMap(data), context, slot);
}

QNetworkReply *QRestAccessManager::postWithDataImpl(const QNetworkRequest &request,
                                                 const QByteArray &data, const QObject *context,
                                                 QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->post(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::postWithDataImpl(const QNetworkRequest &request,
                                                 QHttpMultiPart *data, const QObject *context,
                                                 QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->post(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::postWithDataImpl(const QNetworkRequest &request,
                                                 QIODevice *data, const QObject *context,
                                                 QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->post(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::getNoDataImpl(const QNetworkRequest &request,
                                     const QObject *context, QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->get(request); }, context, slot);
}

QNetworkReply *QRestAccessManager::getWithDataImpl(const QNetworkRequest &request,
                                                const QByteArray &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->get(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::getWithDataImpl(const QNetworkRequest &request,
                                                const QJsonObject &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&](auto req, auto json) { return d->qnam->get(req, json); },
                             data, request, context, slot);
}

QNetworkReply *QRestAccessManager::getWithDataImpl(const QNetworkRequest &request,
                                                QIODevice *data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->get(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::deleteResourceNoDataImpl(const QNetworkRequest &request,
                                     const QObject *context, QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->deleteResource(request); }, context, slot);
}

QNetworkReply *QRestAccessManager::headNoDataImpl(const QNetworkRequest &request,
                                     const QObject *context, QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->head(request); }, context, slot);
}

QNetworkReply *QRestAccessManager::putWithDataImpl(const QNetworkRequest &request,
                                                const QJsonObject &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&](auto req, auto json) { return d->qnam->put(req, json); },
                             data, request, context, slot);
}

QNetworkReply *QRestAccessManager::putWithDataImpl(const QNetworkRequest &request,
                                                const QJsonArray &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&](auto req, auto json) { return d->qnam->put(req, json); },
                             data, request, context, slot);
}

QNetworkReply *QRestAccessManager::putWithDataImpl(const QNetworkRequest &request,
                                                const QVariantMap &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    return putWithDataImpl(request, QJsonObject::fromVariantMap(data), context, slot);
}

QNetworkReply *QRestAccessManager::putWithDataImpl(const QNetworkRequest &request,
                                                const QByteArray &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->put(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::putWithDataImpl(const QNetworkRequest &request,
                                                QHttpMultiPart *data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->put(request, data); }, context, slot);
}

QNetworkReply *QRestAccessManager::putWithDataImpl(const QNetworkRequest &request, QIODevice *data,
                                     const QObject *context, QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->put(request, data); }, context, slot);
}

static const auto PATCH = "PATCH"_ba;

QNetworkReply *QRestAccessManager::patchWithDataImpl(const QNetworkRequest &request,
                                                const QJsonObject &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest(
            [&](auto req, auto json){ return d->qnam->sendCustomRequest(req, PATCH, json); },
            data, request, context, slot);
}

QNetworkReply *QRestAccessManager::patchWithDataImpl(const QNetworkRequest &request,
                                                const QJsonArray &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest(
            [&](auto req, auto json){ return d->qnam->sendCustomRequest(req, PATCH, json); },
            data, request, context, slot);
}

QNetworkReply *QRestAccessManager::patchWithDataImpl(const QNetworkRequest &request,
                                                const QVariantMap &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    return patchWithDataImpl(request, QJsonObject::fromVariantMap(data), context, slot);
}

QNetworkReply *QRestAccessManager::patchWithDataImpl(const QNetworkRequest &request,
                                                const QByteArray &data, const QObject *context,
                                                QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->sendCustomRequest(request, PATCH, data); },
                             context, slot);
}

QNetworkReply *QRestAccessManager::patchWithDataImpl(const QNetworkRequest &request, QIODevice *data,
                                           const QObject *context, QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->sendCustomRequest(request, PATCH, data); },
                             context, slot);
}

QNetworkReply *QRestAccessManager::customWithDataImpl(const QNetworkRequest &request,
                                                   const QByteArray& method, const QByteArray &data,
                                                   const QObject *context,
                                                   QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->sendCustomRequest(request, method, data); },
                             context, slot);
}

QNetworkReply *QRestAccessManager::customWithDataImpl(const QNetworkRequest &request,
                                                   const QByteArray& method, QIODevice *data,
                                                   const QObject *context,
                                                   QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->sendCustomRequest(request, method, data); },
                             context, slot);
}

QNetworkReply *QRestAccessManager::customWithDataImpl(const QNetworkRequest &request,
                                                   const QByteArray& method, QHttpMultiPart *data,
                                                   const QObject *context,
                                                   QtPrivate::QSlotObjectBase *slot)
{
    Q_D(QRestAccessManager);
    return d->executeRequest([&]() { return d->qnam->sendCustomRequest(request, method, data); },
                             context, slot);
}

QNetworkReply *QRestAccessManagerPrivate::createActiveRequest(QNetworkReply *reply,
                                                    const QObject *contextObject,
                                                    QtPrivate::QSlotObjectBase *slot)
{
    Q_Q(QRestAccessManager);
    Q_ASSERT(reply);
    QtPrivate::SlotObjSharedPtr slotPtr(QtPrivate::SlotObjUniquePtr{slot}); // adopts
    activeRequests.insert(reply, CallerInfo{contextObject, slotPtr});
    // The signal connections below are made to 'q' to avoid stray signal
    // handling upon its destruction while requests were still in progress

    QObject::connect(reply, &QNetworkReply::finished, q, [reply, this]() {
        handleReplyFinished(reply);
    });
    // Safe guard in case reply is destroyed before it's finished
    QObject::connect(reply, &QObject::destroyed, q, [reply, this]() {
        activeRequests.remove(reply);
    });
    // If context object is destroyed, clean up any possible replies it had associated with it
    if (contextObject) {
        QObject::connect(contextObject, &QObject::destroyed, q, [reply, this]() {
            activeRequests.remove(reply);
        });
    }
    return reply;
}

void QRestAccessManagerPrivate::verifyThreadAffinity(const QObject *contextObject)
{
    Q_Q(QRestAccessManager);
    if (QThread::currentThread() != q->thread()) {
        qCWarning(lcQrest, "QRestAccessManager can only be called in the thread it belongs to");
        Q_ASSERT(false);
    }
    if (contextObject && (contextObject->thread() != q->thread())) {
        qCWarning(lcQrest, "QRestAccessManager: the context object must reside in the same thread");
        Q_ASSERT(false);
    }
}

QNetworkReply* QRestAccessManagerPrivate::warnNoAccessManager()
{
    qCWarning(lcQrest, "QRestAccessManager: QNetworkAccessManager not set");
    return nullptr;
}

void QRestAccessManagerPrivate::handleReplyFinished(QNetworkReply *reply)
{
    auto request = activeRequests.find(reply);
    if (request == activeRequests.end()) {
        qCDebug(lcQrest, "QRestAccessManager: Unexpected reply received, ignoring");
        return;
    }

    CallerInfo caller = request.value();
    activeRequests.erase(request);

    if (caller.slot) {
        // Callback was provided
        QRestReply restReply(reply);
        void *argv[] = { nullptr, &restReply };
        // If we have context object, use it
        QObject *context = caller.contextObject
                ? const_cast<QObject*>(caller.contextObject.get()) : nullptr;
        caller.slot->call(context, argv);
    }
}

QT_END_NAMESPACE

#include "moc_qrestaccessmanager.cpp"
