// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QNETWORKREQUEST_P_H
#define QNETWORKREQUEST_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qnetworkrequest.h"
#include "QtCore/qbytearray.h"
#include "QtCore/qlist.h"
#include "QtCore/qhash.h"
#include "QtCore/qshareddata.h"
#include "QtCore/qsharedpointer.h"
#include "QtCore/qpointer.h"

QT_BEGIN_NAMESPACE

// this is the common part between QNetworkRequestPrivate, QNetworkReplyPrivate and QHttpPartPrivate
class QNetworkHeadersPrivate
{
public:
    typedef QPair<QByteArray, QByteArray> RawHeaderPair;
    typedef QList<RawHeaderPair> RawHeadersList;
    typedef QHash<QNetworkRequest::KnownHeaders, QVariant> CookedHeadersMap;
    typedef QHash<QNetworkRequest::Attribute, QVariant> AttributesMap;

    RawHeadersList rawHeaders;
    CookedHeadersMap cookedHeaders;
    AttributesMap attributes;
    QPointer<QObject> originatingObject;

    RawHeadersList::ConstIterator findRawHeader(const QByteArray &key) const;
    RawHeadersList allRawHeaders() const;
    QList<QByteArray> rawHeadersKeys() const;
    void setRawHeader(const QByteArray &key, const QByteArray &value);
    void setAllRawHeaders(const RawHeadersList &list);
    void setCookedHeader(QNetworkRequest::KnownHeaders header, const QVariant &value);

    static QDateTime fromHttpDate(const QByteArray &value);
    static QByteArray toHttpDate(const QDateTime &dt);

private:
    void setRawHeaderInternal(const QByteArray &key, const QByteArray &value);
    void parseAndSetHeader(const QByteArray &key, const QByteArray &value);
};

Q_DECLARE_TYPEINFO(QNetworkHeadersPrivate::RawHeaderPair, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE


#endif
