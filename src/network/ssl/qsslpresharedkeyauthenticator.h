/****************************************************************************
**
** Copyright (C) 2014 Governikus GmbH & Co. KG.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSSLPRESHAREDKEYAUTHENTICATOR_H
#define QSSLPRESHAREDKEYAUTHENTICATOR_H

#include <QtCore/QtGlobal>
#include <QtCore/QString>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QMetaType>

QT_BEGIN_NAMESPACE

class QSslPreSharedKeyAuthenticatorPrivate;

class Q_NETWORK_EXPORT QSslPreSharedKeyAuthenticator
{
public:
    QSslPreSharedKeyAuthenticator();
    ~QSslPreSharedKeyAuthenticator();
    QSslPreSharedKeyAuthenticator(const QSslPreSharedKeyAuthenticator &authenticator);
    QSslPreSharedKeyAuthenticator &operator=(const QSslPreSharedKeyAuthenticator &authenticator);

#ifdef Q_COMPILER_RVALUE_REFS
    inline QSslPreSharedKeyAuthenticator &operator=(QSslPreSharedKeyAuthenticator &&authenticator)
    { d.swap(authenticator.d); return *this; }
#endif

    void swap(QSslPreSharedKeyAuthenticator &authenticator)
    {
        d.swap(authenticator.d);
    }

    QByteArray identityHint() const;

    void setIdentity(const QByteArray &identity);
    QByteArray identity() const;
    int maximumIdentityLength() const;

    void setPreSharedKey(const QByteArray &preSharedKey);
    QByteArray preSharedKey() const;
    int maximumPreSharedKeyLength() const;

private:
    friend Q_NETWORK_EXPORT bool operator==(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs);
    friend class QSslSocketBackendPrivate;

    QSharedDataPointer<QSslPreSharedKeyAuthenticatorPrivate> d;
};

inline bool operator!=(const QSslPreSharedKeyAuthenticator &lhs, const QSslPreSharedKeyAuthenticator &rhs)
{
    return !operator==(lhs, rhs);
}

Q_DECLARE_SHARED(QSslPreSharedKeyAuthenticator)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QSslPreSharedKeyAuthenticator)
Q_DECLARE_METATYPE(QSslPreSharedKeyAuthenticator*)

#endif // QSSLPRESHAREDKEYAUTHENTICATOR_H
