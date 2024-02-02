// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtNetwork/qtnetworkglobal.h>

#if QT_CONFIG(ssl)

#include <QtNetwork/qsslsocket.h>

#endif // QT_CONFIG(ssl)

#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

namespace TlsAux {

inline bool classImplemented(QSsl::ImplementedClass cl)
{
#if QT_CONFIG(ssl)
    return QSslSocket::implementedClasses().contains(cl);
#endif
    return cl == QSsl::ImplementedClass::Certificate; // This is the only thing our 'cert-only' supports.
}

} // namespace TlsAux



QT_END_NAMESPACE
