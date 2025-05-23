// Copyright (C) 2012 Collabora Ltd, author <robin.burchell@collabora.co.uk>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qdnslookup_p.h"

QT_BEGIN_NAMESPACE

void QDnsLookupRunnable::query(QDnsLookupReply *reply)
{
    reply->error = QDnsLookup::ResolverError;
    reply->errorString = tr("Not yet supported on this OS");
    return;
}

QT_END_NAMESPACE
