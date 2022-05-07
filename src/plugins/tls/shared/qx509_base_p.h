/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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
******************************************************************************/

#ifndef QX509CERTIFICATE_BASE_P_H
#define QX509CERTIFICATE_BASE_P_H

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

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include <QtNetwork/private/qtlsbackend_p.h>

#include <QtNetwork/qssl.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class X509CertificateBase : public X509Certificate
{
public:
    bool isNull() const override;
    QByteArray version() const override;
    QByteArray serialNumber() const override;
    QStringList issuerInfo(QSslCertificate::SubjectInfo info) const override;
    QStringList issuerInfo(const QByteArray &attribute) const override;
    QStringList subjectInfo(QSslCertificate::SubjectInfo info) const override;
    QStringList subjectInfo(const QByteArray &attribute) const override;
    QList<QByteArray> subjectInfoAttributes() const override;
    QList<QByteArray> issuerInfoAttributes() const override;
    QDateTime effectiveDate() const override;
    QDateTime expiryDate() const override;

    qsizetype numberOfExtensions() const override;
    QString oidForExtension(qsizetype index) const override;
    QString nameForExtension(qsizetype index) const override;
    QVariant valueForExtension(qsizetype index) const override;
    bool isExtensionCritical(qsizetype index) const override;
    bool isExtensionSupported(qsizetype index) const override;

    static QByteArray subjectInfoToString(QSslCertificate::SubjectInfo info);
    static bool matchLineFeed(const QByteArray &pem, int *offset);

protected:
    bool validIndex(qsizetype index) const
    {
        return index >= 0 && index < extensions.size();
    }

    bool null = true;
    QByteArray versionString;
    QByteArray serialNumberString;

    QMultiMap<QByteArray, QString> issuerInfoEntries;
    QMultiMap<QByteArray, QString> subjectInfoEntries;
    QDateTime notValidAfter;
    QDateTime notValidBefore;

    struct X509CertificateExtension
    {
        QString oid;
        QString name;
        QVariant value;
        bool critical = false;
        bool supported = false;
    };

    QList<X509CertificateExtension> extensions;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QX509CERTIFICATE_BASE_P_H
