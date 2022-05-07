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

#ifndef QTLSKEY_SCHANNEL_P_H
#define QTLSKEY_SCHANNEL_P_H

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

#include "../shared/qtlskey_generic_p.h"

#include <QtCore/qglobal.h>

QT_REQUIRE_CONFIG(ssl);

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

class TlsKeySchannel final : public TlsKeyGeneric
{
public:
    using TlsKeyGeneric::TlsKeyGeneric;

    QByteArray decrypt(Cipher cipher, const QByteArray &data, const QByteArray &key,
                       const QByteArray &iv) const override;
    QByteArray encrypt(Cipher cipher, const QByteArray &data, const QByteArray &key,
                       const QByteArray &iv) const override;
};

} // namespace QTlsPrivate

QT_END_NAMESPACE

#endif // QTLSKEY_SCHANNEL_P_H

