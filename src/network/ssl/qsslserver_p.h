// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2016 Kurt Pattyn <pattyn.kurt@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSSLSERVER_P_H
#define QSSLSERVER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/private/qtcpserver_p.h>

QT_BEGIN_NAMESPACE

class Q_NETWORK_EXPORT QSslServerPrivate : public QTcpServerPrivate
{
public:
    Q_DECLARE_PUBLIC(QSslServer)

    QSslServerPrivate();

    QSslConfiguration sslConfiguration;
};


QT_END_NAMESPACE

#endif // QSSLSERVER_P_H
