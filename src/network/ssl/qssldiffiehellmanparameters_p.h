/****************************************************************************
**
** Copyright (C) 2015 Mikkel Krautz <mikkel@krautz.dk>
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


#ifndef QSSLDIFFIEHELLMANPARAMETERS_P_H
#define QSSLDIFFIEHELLMANPARAMETERS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qssldiffiehellmanparameters.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/private/qtnetworkglobal_p.h>

#include "qssldiffiehellmanparameters.h"

#include <QSharedData>

QT_BEGIN_NAMESPACE

class QSslDiffieHellmanParametersPrivate : public QSharedData
{
public:
    void initFromDer(const QByteArray &der);
    void initFromPem(const QByteArray &pem);

    QSslDiffieHellmanParameters::Error error = QSslDiffieHellmanParameters::NoError;
    QByteArray derData;
};

QT_END_NAMESPACE

#endif // QSSLDIFFIEHELLMANPARAMETERS_P_H
