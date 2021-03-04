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
#ifndef QX509_GENERIC_P_H
#define QX509_GENERIC_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtnetworkglobal_p.h>

#include <private/qtlsbackend_p.h>
#include <private/qx509_base_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

// TLSTODO: This class is what previously was known as qsslcertificate_qt.
// A part of SecureTransport and Schannel plugin.
class X509CertificateGeneric : public X509CertificateBase
{
public:
    bool isEqual(const X509Certificate &rhs) const override;
    bool isSelfSigned() const override;

    QMultiMap<QSsl::AlternativeNameEntryType, QString> subjectAlternativeNames() const override;
    QByteArray toPem() const override;
    QByteArray toDer() const override;
    QString toText() const override;
    Qt::HANDLE handle() const override;

    size_t hash(size_t seed) const noexcept override;

    static QList<QSslCertificate> certificatesFromPem(const QByteArray &pem, int count);
    static QList<QSslCertificate> certificatesFromDer(const QByteArray &der, int count);

protected:

    bool subjectMatchesIssuer = false;
    QSsl::KeyAlgorithm publicKeyAlgorithm = QSsl::Rsa;
    QByteArray publicKeyDerData;

    QMultiMap<QSsl::AlternativeNameEntryType, QString> saNames;
    QByteArray derData;

    bool parse(const QByteArray &data);
    bool parseExtension(const QByteArray &data, X509CertificateExtension &extension);
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509_GENERIC_P_H
