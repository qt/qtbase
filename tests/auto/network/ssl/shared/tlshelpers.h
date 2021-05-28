/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtNetwork/qtnetworkglobal.h>

#if QT_CONFIG(ssl)

#include <QtNetwork/qsslsocket.h>

#endif // QT_CONFIG(ssl)

#include <QtCore/qstring.h>
#include <QtCore/qglobal.h>

// TODO: these 'helpers' later to include OpenSSL resolver/sumbols
// required by some auto-tests.

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
