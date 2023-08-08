// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRESTACCESSMANAGER_H
#define QRESTACCESSMANAGER_H

#include <QtNetwork/qnetworkaccessmanager.h>

#include <chrono>

QT_BEGIN_NAMESPACE

class QRestReply;

#define QREST_METHOD_WITH_DATA(METHOD, DATA)                                                     \
public:                                                                                          \
template <typename Functor, if_compatible_callback<Functor> = true>                              \
QRestReply *METHOD(const QNetworkRequest &request, DATA data,                                    \
       const ContextTypeForFunctor<Functor> *context,                                            \
       Functor &&callback)                                                                       \
{                                                                                                \
    return METHOD##WithDataImpl(request, data, context,                                          \
           QtPrivate::makeCallableObject<CallbackPrototype>(std::forward<Functor>(callback)));   \
}                                                                                                \
QRestReply *METHOD(const QNetworkRequest &request, DATA data)                                    \
{                                                                                                \
    return METHOD##WithDataImpl(request, data, nullptr, nullptr);                                \
}                                                                                                \
private:                                                                                         \
QRestReply *METHOD##WithDataImpl(const QNetworkRequest &request, DATA data,                      \
                                 const QObject *context, QtPrivate::QSlotObjectBase *slot);      \
/* end */


#define QREST_METHOD_NO_DATA(METHOD)                                                             \
public:                                                                                          \
template <typename Functor, if_compatible_callback<Functor> = true>                              \
QRestReply *METHOD(const QNetworkRequest &request,                                               \
       const ContextTypeForFunctor<Functor> *context,                                            \
       Functor &&callback)                                                                       \
{                                                                                                \
    return METHOD##NoDataImpl(request, context,                                                  \
           QtPrivate::makeCallableObject<CallbackPrototype>(std::forward<Functor>(callback)));   \
}                                                                                                \
QRestReply *METHOD(const QNetworkRequest &request)                                               \
{                                                                                                \
    return METHOD##NoDataImpl(request, nullptr, nullptr);                                        \
}                                                                                                \
private:                                                                                         \
QRestReply *METHOD##NoDataImpl(const QNetworkRequest &request,                                   \
                               const QObject *context, QtPrivate::QSlotObjectBase *slot);        \
/* end */

class QRestAccessManagerPrivate;
class Q_NETWORK_EXPORT QRestAccessManager : public QObject
{
    Q_OBJECT

    using CallbackPrototype = void(*)(QRestReply*);
    template <typename Functor>
    using ContextTypeForFunctor = typename QtPrivate::ContextTypeForFunctor<Functor>::ContextType;
    template <typename Functor>
    using if_compatible_callback = std::enable_if_t<
                     QtPrivate::AreFunctionsCompatible<CallbackPrototype, Functor>::value, bool>;
public:
    explicit QRestAccessManager(QObject *parent = nullptr);
    ~QRestAccessManager() override;

    QNetworkAccessManager *networkAccessManager() const;

    bool deletesRepliesOnFinished() const;
    void setDeletesRepliesOnFinished(bool autoDelete);

    void setTransferTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds transferTimeout() const;

    void abortRequests();

    QREST_METHOD_NO_DATA(deleteResource)
    QREST_METHOD_NO_DATA(head)
    QREST_METHOD_NO_DATA(get)
    QREST_METHOD_WITH_DATA(get, const QByteArray &)
    QREST_METHOD_WITH_DATA(get, const QJsonObject &)
    QREST_METHOD_WITH_DATA(get, QIODevice *)
    QREST_METHOD_WITH_DATA(post, const QJsonObject &)
    QREST_METHOD_WITH_DATA(post, const QJsonArray &)
    QREST_METHOD_WITH_DATA(post, const QVariantMap &)
    QREST_METHOD_WITH_DATA(post, const QByteArray &)
    QREST_METHOD_WITH_DATA(post, QHttpMultiPart *)
    QREST_METHOD_WITH_DATA(post, QIODevice *)
    QREST_METHOD_WITH_DATA(put, const QJsonObject &)
    QREST_METHOD_WITH_DATA(put, const QJsonArray &)
    QREST_METHOD_WITH_DATA(put, const QVariantMap &)
    QREST_METHOD_WITH_DATA(put, const QByteArray &)
    QREST_METHOD_WITH_DATA(put, QHttpMultiPart *)
    QREST_METHOD_WITH_DATA(put, QIODevice *)

Q_SIGNALS:
#ifndef QT_NO_NETWORKPROXY
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *authenticator);
#endif
    void authenticationRequired(QRestReply *reply, QAuthenticator *authenticator);
    void requestFinished(QRestReply *reply);

private:
    Q_DECLARE_PRIVATE(QRestAccessManager)
    Q_DISABLE_COPY(QRestAccessManager)
};

#undef QREST_METHOD_NO_DATA
#undef QREST_METHOD_WITH_DATA

QT_END_NAMESPACE

#endif
