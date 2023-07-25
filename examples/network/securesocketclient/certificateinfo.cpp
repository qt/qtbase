// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "certificateinfo.h"
#include "ui_certificateinfo.h"

CertificateInfo::CertificateInfo(QWidget *parent)
    : QDialog(parent)
{
    form = new Ui_CertificateInfo;
    form->setupUi(this);

    connect(form->certificationPathView, &QComboBox::currentIndexChanged,
            this, &CertificateInfo::updateCertificateInfo);
}

CertificateInfo::~CertificateInfo()
{
    delete form;
}

void CertificateInfo::setCertificateChain(const QList<QSslCertificate> &chain)
{
    certificateChain = chain;

    form->certificationPathView->clear();
    for (int i = 0; i < certificateChain.size(); ++i) {
        const QSslCertificate &cert = certificateChain.at(i);
        form->certificationPathView->addItem(tr("%1%2 (%3)").arg(!i ? QString() : tr("Issued by: "))
                                             .arg(cert.subjectInfo(QSslCertificate::Organization).join(QLatin1Char(' ')))
                                             .arg(cert.subjectInfo(QSslCertificate::CommonName).join(QLatin1Char(' '))));
    }
    form->certificationPathView->setCurrentIndex(0);
}

void CertificateInfo::updateCertificateInfo(int index)
{
    form->certificateInfoView->clear();
    if (index >= 0 && index < certificateChain.size()) {
        const QSslCertificate &cert = certificateChain.at(index);
        QStringList lines;
        lines << tr("Organization: %1").arg(cert.subjectInfo(QSslCertificate::Organization).join(u' '))
              << tr("Subunit: %1").arg(cert.subjectInfo(QSslCertificate::OrganizationalUnitName).join(u' '))
              << tr("Country: %1").arg(cert.subjectInfo(QSslCertificate::CountryName).join(u' '))
              << tr("Locality: %1").arg(cert.subjectInfo(QSslCertificate::LocalityName).join(u' '))
              << tr("State/Province: %1").arg(cert.subjectInfo(QSslCertificate::StateOrProvinceName).join(u' '))
              << tr("Common Name: %1").arg(cert.subjectInfo(QSslCertificate::CommonName).join(u' '))
              << QString()
              << tr("Issuer Organization: %1").arg(cert.issuerInfo(QSslCertificate::Organization).join(u' '))
              << tr("Issuer Unit Name: %1").arg(cert.issuerInfo(QSslCertificate::OrganizationalUnitName).join(u' '))
              << tr("Issuer Country: %1").arg(cert.issuerInfo(QSslCertificate::CountryName).join(u' '))
              << tr("Issuer Locality: %1").arg(cert.issuerInfo(QSslCertificate::LocalityName).join(u' '))
              << tr("Issuer State/Province: %1").arg(cert.issuerInfo(QSslCertificate::StateOrProvinceName).join(u' '))
              << tr("Issuer Common Name: %1").arg(cert.issuerInfo(QSslCertificate::CommonName).join(u' '));
        for (const auto &line : lines)
            form->certificateInfoView->addItem(line);
    }
}
