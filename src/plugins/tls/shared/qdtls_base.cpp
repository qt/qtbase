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

#include "qdtls_base_p.h"

QT_BEGIN_NAMESPACE

void QDtlsBasePrivate::setDtlsError(QDtlsError code, const QString &description)
{
    errorCode = code;
    errorDescription = description;
}

QDtlsError QDtlsBasePrivate::error() const
{
    return errorCode;
}

QString QDtlsBasePrivate::errorString() const
{
    return errorDescription;
}

void QDtlsBasePrivate::clearDtlsError()
{
    errorCode = QDtlsError::NoError;
    errorDescription.clear();
}

QSslConfiguration QDtlsBasePrivate::configuration() const
{
    return dtlsConfiguration;
}

void QDtlsBasePrivate::setConfiguration(const QSslConfiguration &configuration)
{
    dtlsConfiguration = configuration;
    clearDtlsError();
}

bool QDtlsBasePrivate::setCookieGeneratorParameters(const GenParams &params)
{
    if (!params.secret.size()) {
        setDtlsError(QDtlsError::InvalidInputParameters,
                     QDtls::tr("Invalid (empty) secret"));
        return false;
    }

    clearDtlsError();

    hashAlgorithm = params.hash;
    secret = params.secret;

    return true;
}

QDtlsClientVerifier::GeneratorParameters
QDtlsBasePrivate::cookieGeneratorParameters() const
{
    return {hashAlgorithm, secret};
}

bool QDtlsBasePrivate::isDtlsProtocol(QSsl::SslProtocol protocol)
{
    switch (protocol) {
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    case QSsl::DtlsV1_0:
    case QSsl::DtlsV1_0OrLater:
QT_WARNING_POP
    case QSsl::DtlsV1_2:
    case QSsl::DtlsV1_2OrLater:
        return true;
    default:
        return false;
    }
}

QT_END_NAMESPACE
