// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QHTTPHEADERPARSER_H
#define QHTTPHEADERPARSER_H

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

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

QT_BEGIN_NAMESPACE

class Q_NETWORK_PRIVATE_EXPORT QHttpHeaderParser
{
public:
    QHttpHeaderParser();

    void clear();
    bool parseHeaders(QByteArrayView headers);
    bool parseStatus(QByteArrayView status);

    const QList<QPair<QByteArray, QByteArray> >& headers() const;
    void setStatusCode(int code);
    int getStatusCode() const;
    int getMajorVersion() const;
    void setMajorVersion(int version);
    int getMinorVersion() const;
    void setMinorVersion(int version);
    QString getReasonPhrase() const;
    void setReasonPhrase(const QString &reason);

    QByteArray firstHeaderField(const QByteArray &name,
                                const QByteArray &defaultValue = QByteArray()) const;
    QByteArray combinedHeaderValue(const QByteArray &name,
                                   const QByteArray &defaultValue = QByteArray()) const;
    QList<QByteArray> headerFieldValues(const QByteArray &name) const;
    void setHeaderField(const QByteArray &name, const QByteArray &data);
    void prependHeaderField(const QByteArray &name, const QByteArray &data);
    void appendHeaderField(const QByteArray &name, const QByteArray &data);
    void removeHeaderField(const QByteArray &name);
    void clearHeaders();

private:
    QList<QPair<QByteArray, QByteArray> > fields;
    QString reasonPhrase;
    int statusCode;
    int majorVersion;
    int minorVersion;
};


QT_END_NAMESPACE

#endif // QHTTPHEADERPARSER_H
