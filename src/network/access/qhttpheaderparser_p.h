/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
