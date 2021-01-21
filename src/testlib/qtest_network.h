/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtTest module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QTEST_NETWORK_H
#define QTEST_NETWORK_H

#include <QtTest/qtest.h>

// enable NETWORK features
#ifndef QT_NETWORK_LIB
#define QT_NETWORK_LIB
#endif

#if 0
#pragma qt_class(QtTestNetwork)
#endif

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QNetworkCookie>
#include <QtNetwork/QNetworkReply>

#if 0
// inform syncqt
#pragma qt_no_master_include
#endif

QT_BEGIN_NAMESPACE

namespace QTest
{
template<>
inline char *toString<QHostAddress>(const QHostAddress &addr)
{
    switch (addr.protocol()) {
    case QAbstractSocket::UnknownNetworkLayerProtocol:
        return qstrdup("<unknown address (parse error)>");
    case QAbstractSocket::AnyIPProtocol:
        return qstrdup("QHostAddress::Any");
    case QAbstractSocket::IPv4Protocol:
    case QAbstractSocket::IPv6Protocol:
        break;
    }

    return toString(addr.toString());
}

inline char *toString(QNetworkReply::NetworkError code)
{
    const QMetaObject *mo = &QNetworkReply::staticMetaObject;
    int index = mo->indexOfEnumerator("NetworkError");
    if (index == -1)
        return qstrdup("");

    QMetaEnum qme = mo->enumerator(index);
    return qstrdup(qme.valueToKey(code));
}

inline char *toString(const QNetworkCookie &cookie)
{
    return toString(cookie.toRawForm());
}

inline char *toString(const QList<QNetworkCookie> &list)
{
    QByteArray result = "QList(";
    if (!list.isEmpty()) {
        for (const QNetworkCookie &cookie : list)
            result += "QNetworkCookie(" + cookie.toRawForm() + "), ";
        result.chop(2); // remove trailing ", "
    }
    result.append(')');
    return toString(result);
}

} // namespace QTest

QT_END_NAMESPACE

#endif
