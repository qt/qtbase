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

#ifndef QTLSKEY_BASE_P_H
#define QTLSKEY_BASE_P_H

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

#include <qssl.h>

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

// TLSTODO: Note, 'base' is supposed to move to plugins together with
// 'generic' and 'backendXXX'.
class TlsKeyBase : public TlsKey
{
public:
    TlsKeyBase(KeyType type = QSsl::PublicKey, KeyAlgorithm algorithm = QSsl::Opaque)
        : keyType(type),
          keyAlgorithm(algorithm)
    {
    }

    bool isNull() const override
    {
        return keyIsNull;
    }
    KeyType type() const override
    {
        return keyType;
    }
    KeyAlgorithm algorithm() const override
    {
        return keyAlgorithm;
    }
    bool isPkcs8 () const override
    {
        return false;
    }

    QByteArray pemFromDer(const QByteArray &der, const QMap<QByteArray, QByteArray> &headers) const override;

protected:
    static QByteArray pkcs8Header(bool encrypted);
    static QByteArray pkcs8Footer(bool encrypted);
    static bool isEncryptedPkcs8(const QByteArray &der);
public:
    // TLSTODO: this public is quick fix needed by old _openssl classes
    // will become non-public as soon as those classes fixed.
    bool keyIsNull = true;
    KeyType keyType = QSsl::PublicKey;
    KeyAlgorithm keyAlgorithm = QSsl::Opaque;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLSKEY_BASE_P_H
