/****************************************************************************
** Copyright (C) 2011 Richard J. Moore <rich@kde.org>
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef QOCSPRESPONSE_H
#define QOCSPRESPONSE_H

#include <QtNetwork/qtnetworkglobal.h>

#include <QtCore/qobject.h>
#include <QtCore/qscopedpointer.h>

#ifndef Q_CLANG_QDOC
QT_REQUIRE_CONFIG(ssl);
#endif

QT_BEGIN_NAMESPACE

enum class OcspCertificateStatus
{
    Good,
    Revoked,
    Unknown
};

enum class OcspRevocationReason
{
    None = -1,
    Unspecified,
    KeyCompromise,
    CACompromise,
    AffiliationChanged,
    Superseded,
    CessationOfOperation,
    CertificateHold,
    RemoveFromCRL
};

class QOcspResponsePrivate;
class QOcspResponse
{
public:

    QOcspResponse();
    QOcspResponse(const QOcspResponse &other);
    QOcspResponse(QOcspResponse && other)  Q_DECL_NOEXCEPT;
    ~QOcspResponse();

    QOcspResponse &operator = (const QOcspResponse &other);
    QOcspResponse &operator = (QOcspResponse &&other) Q_DECL_NOTHROW;

    bool isNull() const;
    void clear();

    OcspCertificateStatus certificateStatus() const;
    OcspRevocationReason revocationReason() const;

    class QSslCertificate responder() const;

private:

    friend class QSslSocketBackendPrivate;
    QScopedPointer<QOcspResponsePrivate> d;
};

QT_END_NAMESPACE

#endif // QOCSPRESPONSE_H
