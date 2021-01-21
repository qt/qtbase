/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef QSSLCIPHER_P_H
#define QSSLCIPHER_P_H

#include <QtNetwork/private/qtnetworkglobal_p.h>
#include "qsslcipher.h"

QT_BEGIN_NAMESPACE

//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the QLibrary class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

class QSslCipherPrivate
{
public:
    QSslCipherPrivate()
        : isNull(true), supportedBits(0), bits(0),
          exportable(false), protocol(QSsl::UnknownProtocol)
    {
    }

    bool isNull;
    QString name;
    int supportedBits;
    int bits;
    QString keyExchangeMethod;
    QString authenticationMethod;
    QString encryptionMethod;
    bool exportable;
    QString protocolString;
    QSsl::SslProtocol protocol;
};

QT_END_NAMESPACE

#endif // QSSLCIPHER_P_H
