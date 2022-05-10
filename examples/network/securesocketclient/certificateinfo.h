// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CERTIFICATEINFO_H
#define CERTIFICATEINFO_H

#include <QDialog>
#include <QList>
#include <QSslCertificate>

QT_BEGIN_NAMESPACE
class Ui_CertificateInfo;
QT_END_NAMESPACE

class CertificateInfo : public QDialog
{
    Q_OBJECT
public:
    explicit CertificateInfo(QWidget *parent = nullptr);
    ~CertificateInfo();

    void setCertificateChain(const QList<QSslCertificate> &chain);

private slots:
    void updateCertificateInfo(int index);

private:
    Ui_CertificateInfo *form = nullptr;
    QList<QSslCertificate> certificateChain;
};

#endif
