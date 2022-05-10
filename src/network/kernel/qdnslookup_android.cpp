// Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdnslookup_p.h"

QT_BEGIN_NAMESPACE

void QDnsLookupRunnable::query(const int requestType, const QByteArray &requestName, const QHostAddress &nameserver, QDnsLookupReply *reply)
{
    Q_UNUSED(requestType);
    Q_UNUSED(requestName);
    Q_UNUSED(nameserver);
    Q_UNUSED(reply);
    qWarning("Not yet supported on Android");
    reply->error = QDnsLookup::ResolverError;
    reply->errorString = tr("Not yet supported on Android");
    return;
}

QT_END_NAMESPACE
