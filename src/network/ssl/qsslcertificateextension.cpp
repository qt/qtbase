/****************************************************************************
**
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*!
    \class QSslCertificateExtension
    \brief The QSslCertificateExtension provides an API for accessing the extensions of an X509 certificate.
    \since 5.0

    \rentrant
    \ingroup network
    \ingroup ssl
    \inmodule QtNetwork

    QSslCertificateExtension provides access to an extension stored in
    an X509 certificate. The information available depends on the type
    of extension being accessed.

    All X509 certificate extensions have the following properties:

    \table
    \header
       \o Property
       \o Description
    \row
       \o name
       \o The human readable name of the extension, eg. 'basicConstraints'.
    \row
       \o criticality
       \o This is a boolean value indicating if the extension is critical
          to correctly interpreting the certificate.
    \row
       \o oid
       \o The ASN.1 object identifier that specifies which extension this
          is.
    \row
       \o supported
       \o If this is true the structure of the extension's value will not
          change between Qt versions.
    \row
       \o value
       \o A QVariant with a structure dependent on the type of extension.
    \endtable

    Whilst this class provides access to any type of extension, only
    some are guaranteed to be returned in a format that will remain
    unchanged between releases. The isSupported() method returns true
    for extensions where this is the case.

    The extensions currently supported, and the structure of the value
    returned are as follows:

    \table
    \header
       \o Name
       \o OID
       \o Details
    \row
       \o basicConstraints
       \o 2.5.29.19
       \o Returned as a QVariantMap. The key 'ca' contains a boolean value,
          the optional key 'pathLenConstraint' contains an integer.
    \row
       \o authorityInfoAccess
       \o 1.3.6.1.5.5.7.1.1
       \o Returned as a QVariantMap. There is a key for each access method,
          with the value being a URI.
    \row
       \o subjectKeyIdentifier
       \o 2.5.29.14
       \o Returned as a QVariant containing a QString. The string is the key
          identifier.
    \row
       \o authorityKeyIdentifier
       \o 2.5.29.35
       \o Returned as a QVariantMap. The optional key 'keyid' contains the key
          identifier as a hex string stored in a QByteArray. The optional key
          'serial' contains the authority key serial number as a qlonglong.
          Currently there is no support for the general names field of this
          extension.
    \endtable

    In addition to the supported extensions above, many other common extensions
    will be returned in a reasonably structured way. Extensions that the SSL
    backend has no support for at all will be returned as a QByteArray.

    Further information about the types of extensions certificates can
    contain can be found in RFC 5280.

    \sa QSslCertificate::extensions()
 */

#include "qsslcertificateextension.h"
#include "qsslcertificateextension_p.h"

QT_BEGIN_NAMESPACE

QSslCertificateExtension::QSslCertificateExtension()
    : d(new QSslCertificateExtensionPrivate)
{
}

QSslCertificateExtension::QSslCertificateExtension(const QSslCertificateExtension &other)
    : d(other.d)
{
}

QSslCertificateExtension::~QSslCertificateExtension()
{
}

QSslCertificateExtension &QSslCertificateExtension::operator=(const QSslCertificateExtension &other)
{
    d = other.d;
    return *this;
}

/*!
    Returns the ASN.1 OID of this extension.
 */
QString QSslCertificateExtension::oid() const
{
    return d->oid;
}

/*!
    Returns the name of the extension. If no name is known for the
    extension then the OID will be returned.
 */
QString QSslCertificateExtension::name() const
{
    return d->name;
}

/*!
    Returns the value of the extension. The structure of the value
    returned depends on the extension type.
 */
QVariant QSslCertificateExtension::value() const
{
    return d->value;
}

/*!
    Returns the criticality of the extension.
 */
bool QSslCertificateExtension::isCritical() const
{
    return d->critical;
}

/*!
    Returns the true if this extension is supported. In this case,
    supported simply means that the structure of the QVariant returned
    by the value() accessor will remain unchanged between versions.
    Unsupported extensions can be freely used, however there is no
    guarantee that the returned data will have the same structure
    between versions.
 */
bool QSslCertificateExtension::isSupported() const
{
    return d->supported;
}

QT_END_NAMESPACE
