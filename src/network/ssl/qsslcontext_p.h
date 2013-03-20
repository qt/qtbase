/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QSSLCONTEXT_H
#define QSSLCONTEXT_H

#include <QtCore/qvariant.h>
#include <QtNetwork/qsslcertificate.h>
#include <QtNetwork/qsslconfiguration.h>
#include <openssl/ssl.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SSL

class QSslContextPrivate;

class QSslContext
{
public:

    ~QSslContext();

    static QSslContext* fromConfiguration(QSslSocket::SslMode mode, const QSslConfiguration &configuration,
                                          bool allowRootCertOnDemandLoading);

    QSslError::SslError error() const;
    QString errorString() const;

    SSL* createSsl();
    bool cacheSession(SSL*); // should be called when handshake completed

protected:
    QSslContext();

private:
    SSL_CTX* ctx;
    EVP_PKEY *pkey;
    SSL_SESSION *session;
    QSslError::SslError errorCode;
    QString errorStr;
    QSslConfiguration sslConfiguration;
};

#endif // QT_NO_SSL

QT_END_NAMESPACE

#endif // QSSLCONTEXT_H
