// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//! [0]
    connect(socket, &QSslSocket::preSharedKeyAuthenticationRequired,
            this, &AuthManager::handlePreSharedKeyAuthentication);
//! [0]

//! [1]
    void AuthManager::handlePreSharedKeyAuthentication(QSslPreSharedKeyAuthenticator *authenticator)
    {
        authenticator->setIdentity("My Qt App");

        const QByteArray key = deriveKey(authenticator->identityHint(), passphrase);
        authenticator->setPreSharedKey(key);
    }
//! [1]
