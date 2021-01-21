/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QPASSWORDDIGESTOR_H
#define QPASSWORDDIGESTOR_H

#include <QtNetwork/qtnetworkglobal.h>
#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>

QT_BEGIN_NAMESPACE

namespace QPasswordDigestor {
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf1(QCryptographicHash::Algorithm algorithm,
                                   const QByteArray &password, const QByteArray &salt,
                                   int iterations, quint64 dkLen);
Q_NETWORK_EXPORT QByteArray deriveKeyPbkdf2(QCryptographicHash::Algorithm algorithm,
                                   const QByteArray &password, const QByteArray &salt,
                                   int iterations, quint64 dkLen);
} // namespace QPasswordDigestor

QT_END_NAMESPACE

#endif // QPASSWORDDIGESTOR_H
