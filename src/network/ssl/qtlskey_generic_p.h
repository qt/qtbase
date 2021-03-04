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

#ifndef QTLSKEY_GENERIC_P_H
#define QTLSKEY_GENERIC_P_H

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

#include <private/qtlskey_base_p.h>
#include <private/qtlsbackend_p.h>

#include <QtCore/qnamespace.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

// This class is what previously was known as qsslkey_qt:
// it implements most of functionality needed by QSslKey
// not relying on any TLS implementation. It's used by
// our SecureTransport and Schannel backends.
class TlsKeyGeneric : public TlsKeyBase
{
public:
    using TlsKeyBase::TlsKeyBase;

    void decodeDer(KeyType type, KeyAlgorithm algorithm, const QByteArray &der,
                   const QByteArray &passPhrase, bool deepClear) override;
    void decodePem(KeyType type, KeyAlgorithm algorithm, const QByteArray &pem,
                   const QByteArray &passPhrase, bool deepClear) override;

    QByteArray toPem(const QByteArray &passPhrase) const override;

    QByteArray derFromPem(const QByteArray &pem, QMap<QByteArray,
                          QByteArray> *headers) const override;

    void fromHandle(Qt::HANDLE opaque, KeyType expectedType) override;

    void clear(bool deep) override;

    Qt::HANDLE handle() const override
    {
        return Qt::HANDLE(opaque);
    }

    int length() const override
    {
        return keyLength;
    }

    bool isPkcs8() const override
    {
        return pkcs8;
    }

private:
    QByteArray decryptPkcs8(const QByteArray &encrypted, const QByteArray &passPhrase);

    bool pkcs8 = false;
    Qt::HANDLE opaque = nullptr;
    QByteArray derData;
    int keyLength = -1;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLSKEY_GENERIC_P_H
