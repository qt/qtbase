/****************************************************************************
**
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
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
****************************************************************************/

#ifndef QSSLCERTIFICATEEXTENSION_H
#define QSSLCERTIFICATEEXTENSION_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

QT_BEGIN_NAMESPACE

class QSslCertificateExtensionPrivate;

class Q_NETWORK_EXPORT QSslCertificateExtension
{
public:
    QSslCertificateExtension();
    QSslCertificateExtension(const QSslCertificateExtension &other);
    QSslCertificateExtension &operator=(QSslCertificateExtension &&other) noexcept { swap(other); return *this; }
    QSslCertificateExtension &operator=(const QSslCertificateExtension &other);
    ~QSslCertificateExtension();

    void swap(QSslCertificateExtension &other) noexcept { qSwap(d, other.d); }

    QString oid() const;
    QString name() const;
    QVariant value() const;
    bool isCritical() const;

    bool isSupported() const;

private:
    friend class QSslCertificatePrivate;
    QSharedDataPointer<QSslCertificateExtensionPrivate> d;
};

Q_DECLARE_SHARED(QSslCertificateExtension)

QT_END_NAMESPACE


#endif // QSSLCERTIFICATEEXTENSION_H


