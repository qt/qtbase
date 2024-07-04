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

#include <QtCore/qloggingcategory.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qhash.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qxpfunctional.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQrest)

class QRestReply;
class QRestAccessManagerPrivate : public QObjectPrivate
{
public:
    QRestAccessManagerPrivate();
    ~QRestAccessManagerPrivate() override;

    QNetworkReply* createActiveRequest(QNetworkReply *reply, const QObject *contextObject,
                                       QtPrivate::SlotObjUniquePtr slot);
    void handleReplyFinished(QNetworkReply *reply);

    using ReqOpRef = qxp::function_ref<QNetworkReply*(QNetworkAccessManager*) const>;
    QNetworkReply *executeRequest(ReqOpRef requestOperation,
                                  const QObject *context, QtPrivate::QSlotObjectBase *rawSlot)
    {
        QtPrivate::SlotObjUniquePtr slot(rawSlot);
        if (!qnam)
            return warnNoAccessManager();
        verifyThreadAffinity(context);
        QNetworkReply *reply = requestOperation(qnam);
        return createActiveRequest(reply, context, std::move(slot));
    }

    using ReqOpRefJson = qxp::function_ref<QNetworkReply*(QNetworkAccessManager*,
                                                          const QNetworkRequest &,
                                                          const QByteArray &) const>;
    QNetworkReply *executeRequest(ReqOpRefJson requestOperation, const QJsonDocument &jsonDoc,
                               const QNetworkRequest &request,
                               const QObject *context, QtPrivate::QSlotObjectBase *rawSlot)
    {
        QtPrivate::SlotObjUniquePtr slot(rawSlot);
        if (!qnam)
            return warnNoAccessManager();
        verifyThreadAffinity(context);
        QNetworkRequest req(request);
        auto h = req.headers();
        if (!h.contains(QHttpHeaders::WellKnownHeader::ContentType)) {
            h.append(QHttpHeaders::WellKnownHeader::ContentType,
                     QLatin1StringView{"application/json"});
        }
        req.setHeaders(std::move(h));
        QNetworkReply *reply = requestOperation(qnam, req, jsonDoc.toJson(QJsonDocument::Compact));
        return createActiveRequest(reply, context, std::move(slot));
    }

    void verifyThreadAffinity(const QObject *contextObject);
    Q_DECL_COLD_FUNCTION
    QNetworkReply* warnNoAccessManager();

    struct CallerInfo {
        QPointer<const QObject> contextObject = nullptr;
        QtPrivate::SlotObjSharedPtr slot;
    };
    QHash<QNetworkReply*, CallerInfo> activeRequests;

    QNetworkAccessManager *qnam = nullptr;
    bool deletesRepliesOnFinished = true;
    Q_DECLARE_PUBLIC(QRestAccessManager)
};

QT_END_NAMESPACE

#endif
