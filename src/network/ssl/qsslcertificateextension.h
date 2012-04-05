/****************************************************************************
**
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSSLCERTIFICATEEXTENSION_H
#define QSSLCERTIFICATEEXTENSION_H

#include <QtCore/qnamespace.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qstring.h>
#include <QtCore/qvariant.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


#ifndef QT_NO_SSL

class QSslCertificateExtensionPrivate;

class Q_NETWORK_EXPORT QSslCertificateExtension
{
public:
    QSslCertificateExtension();
    QSslCertificateExtension(const QSslCertificateExtension &other);
    ~QSslCertificateExtension();

    QSslCertificateExtension &operator=(const QSslCertificateExtension &other);

    void swap(QSslCertificateExtension &other) { qSwap(d, other.d); }

    QString oid() const;
    QString name() const;
    QVariant value() const;
    bool isCritical() const;

    bool isSupported() const;

private:
    friend class QSslCertificatePrivate;
    QSharedDataPointer<QSslCertificateExtensionPrivate> d;
};

#endif // QT_NO_SSL

QT_END_NAMESPACE

QT_END_HEADER


#endif // QSSLCERTIFICATEEXTENSION_H


