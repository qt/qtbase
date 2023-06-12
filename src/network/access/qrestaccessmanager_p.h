// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRESTACCESSMANAGER_P_H
#define QRESTACCESSMANAGER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include "qrestaccessmanager.h"
#include "private/qobject_p.h"

#include <QtNetwork/qnetworkaccessmanager.h>

#include <QtCore/qjsonarray.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>

QT_BEGIN_NAMESPACE

class QRestReply;
class QRestAccessManagerPrivate : public QObjectPrivate
{
public:
    QRestAccessManagerPrivate();
    ~QRestAccessManagerPrivate() override;

    void ensureNetworkAccessManager();

    QRestReply *createActiveRequest(QNetworkReply *networkReply, const QObject *contextObject,
                                    QtPrivate::QSlotObjectBase *slot);

    void removeActiveRequest(QRestReply *restReply);
    void handleReplyFinished(QRestReply *restReply);
    void handleAuthenticationRequired(QNetworkReply *networkReply, QAuthenticator *authenticator);
    QRestReply *restReplyFromNetworkReply(QNetworkReply *networkReply);

    template<typename Functor>
    QRestReply *executeRequest(Functor requestOperation,
                               const QObject *context, QtPrivate::QSlotObjectBase *slot)
    {
        verifyThreadAffinity(context);
        QNetworkReply *reply = requestOperation();
        return createActiveRequest(reply, context, slot);
    }

    template<typename Functor, typename Json>
    QRestReply *executeRequest(Functor requestOperation, Json jsonData,
                               const QNetworkRequest &request,
                               const QObject *context, QtPrivate::QSlotObjectBase *slot)
    {
        verifyThreadAffinity(context);
        QNetworkRequest req(request);
        if (!request.header(QNetworkRequest::ContentTypeHeader).isValid()) {
            req.setHeader(QNetworkRequest::ContentTypeHeader,
                          QLatin1StringView{"application/json"});
        }
        QJsonDocument json(jsonData);
        QNetworkReply *reply = requestOperation(req, json.toJson(QJsonDocument::Compact));
        return createActiveRequest(reply, context, slot);
    }

    void verifyThreadAffinity(const QObject *contextObject);

    struct CallerInfo {
        const QObject *contextObject = nullptr;
        QtPrivate::SlotObjSharedPtr slot;
    };
    QHash<QRestReply*, CallerInfo> activeRequests;

    QNetworkAccessManager *qnam = nullptr;
    bool deletesRepliesOnFinished = true;
    Q_DECLARE_PUBLIC(QRestAccessManager)
};

QT_END_NAMESPACE

#endif
