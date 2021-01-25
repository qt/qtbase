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

#include "qtlsbackend_p.h"
#include "qsslsocket_p.h"
#include "qssl_p.h"

#include <QtCore/private/qfactoryloader_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qmutex.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QTlsBackend_iid, QStringLiteral("/tlsbackends")))

namespace {

class BackendCollection
{
public:
    void addBackend(QTlsBackend *backend)
    {
        Q_ASSERT(backend);
        Q_ASSERT(std::find(backends.begin(), backends.end(), backend) == backends.end());
        const QMutexLocker locker(&collectionMutex);
        backends.push_back(backend);
    }

    void removeBackend(QTlsBackend *backend)
    {
        Q_ASSERT(backend);
        const QMutexLocker locker(&collectionMutex);
        const auto it = std::find(backends.begin(), backends.end(), backend);
        Q_ASSERT(it != backends.end());
        backends.erase(it);
    }

    bool tryPopulateCollection()
    {
        if (!loader())
            return false;

        static QBasicMutex mutex;
        const QMutexLocker locker(&mutex);
        if (loaded)
            return true;

#if QT_CONFIG(library)
        loader->update();
#endif
        int index = 0;
        while (loader->instance(index))
            ++index;

        // TLSTODO: obviously, this one should go away:
        QSslSocketPrivate::registerAdHocFactory();

        return loaded = true;
    }

    QList<QString> backendNames()
    {
        QList<QString> names;
        if (!tryPopulateCollection())
            return names;

        const QMutexLocker locker(&collectionMutex);
        if (!backends.size())
            return names;

        names.reserve(backends.size());
        for (const auto *factory : backends)
            names.append(factory->backendName());

        return names;
    }

    QTlsBackend *backend(const QString &name)
    {
        if (!tryPopulateCollection())
            return nullptr;

        const QMutexLocker locker(&collectionMutex);
        const auto it = std::find_if(backends.begin(), backends.end(),
                                     [&name](const auto *fct) {return fct->backendName() == name;});

        return it == backends.end()  ? nullptr : *it;
    }

private:
    std::vector<QTlsBackend *> backends;
    QMutex collectionMutex;
    bool loaded = false;
};

} // Unnamed namespace

Q_GLOBAL_STATIC(BackendCollection, backends);

namespace QSsl {

TlsKey::~TlsKey() = default;

QByteArray TlsKey::pemHeader() const
{
    if (type() == QSsl::PublicKey)
        return QByteArrayLiteral("-----BEGIN PUBLIC KEY-----");
    else if (algorithm() == QSsl::Rsa)
        return QByteArrayLiteral("-----BEGIN RSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dsa)
        return QByteArrayLiteral("-----BEGIN DSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Ec)
        return QByteArrayLiteral("-----BEGIN EC PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dh)
        return QByteArrayLiteral("-----BEGIN PRIVATE KEY-----");

    Q_UNREACHABLE();
    return {};
}

QByteArray TlsKey::pemFooter() const
{
    if (type() == QSsl::PublicKey)
        return QByteArrayLiteral("-----END PUBLIC KEY-----");
    else if (algorithm() == QSsl::Rsa)
        return QByteArrayLiteral("-----END RSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dsa)
        return QByteArrayLiteral("-----END DSA PRIVATE KEY-----");
    else if (algorithm() == QSsl::Ec)
        return QByteArrayLiteral("-----END EC PRIVATE KEY-----");
    else if (algorithm() == QSsl::Dh)
        return QByteArrayLiteral("-----END PRIVATE KEY-----");

    Q_UNREACHABLE();
    return {};
}


} // namespace QSsl

const QString QTlsBackend::builtinBackendNames[] = {
    QStringLiteral("schannel"),
    QStringLiteral("securetransport"),
    QStringLiteral("openssl")
};

QTlsBackend::QTlsBackend()
{
    if (backends())
        backends->addBackend(this);
}

QTlsBackend::~QTlsBackend()
{
    if (backends())
        backends->removeBackend(this);
}

QString QTlsBackend::backendName() const
{
    return QStringLiteral("dummyTLS");
}

QSsl::TlsKey *QTlsBackend::createKey() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot generate a key");
    return nullptr;
}

QSsl::X509Certificate *QTlsBackend::createCertificate() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot create a certificate");
    return nullptr;
}

QSsl::TlsCryptograph *QTlsBackend::createTlsCryptograph() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot create TLS session");
    return nullptr;
}

QSsl::DtlsCryptograph *QTlsBackend::createDtlsCryptograph() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot create DTLS session");
    return nullptr;
}

QSsl::DtlsCookieVerifier *QTlsBackend::createDtlsCookieVerifier() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot create DTLS cookie generator/verifier");
    return nullptr;
}

QSsl::X509ChainVerifyPtr QTlsBackend::X509Verifier() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot verify X509 chain");
    return nullptr;
}

QSsl::X509PemReaderPtr QTlsBackend::X509PemReader() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot read PEM format");
    return nullptr;
}

QSsl::X509DerReaderPtr QTlsBackend::X509DerReader() const
{
    qCWarning(lcSsl, "Dummy TLS backend, don't know how to read DER");
    return nullptr;
}

QSsl::X509Pkcs12ReaderPtr QTlsBackend::X509Pkcs12Reader() const
{
    qCWarning(lcSsl, "Dummy TLS backend, cannot read PKCS12");
    return nullptr;
}

QList<QString> QTlsBackend::availableBackendNames()
{
    if (!backends())
        return {};

    return backends->backendNames();
}

QString QTlsBackend::defaultBackendName()
{
    // We prefer native as default:
    const auto names = availableBackendNames();
    auto name = builtinBackendNames[nameIndexSchannel];
    if (names.contains(name))
        return name;
    name = builtinBackendNames[nameIndexSecureTransport];
    if (names.contains(name))
        return name;
    name = builtinBackendNames[nameIndexOpenSSL];
    if (names.contains(name))
        return name;

    return {};
}

QTlsBackend *QTlsBackend::findBackend(const QString &backendName)
{
    if (!backends())
        return {};

    if (auto *fct = backends->backend(backendName))
        return fct;

    qCWarning(lcSsl) << "Cannot create unknown backend named" << backendName;
    return nullptr;
}

QList<QSsl::SslProtocol> QTlsBackend::supportedProtocols(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->supportedProtocols();

    return {};
}

QList<QSsl::SupportedFeature> QTlsBackend::supportedFeatures(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->supportedFeatures();

    return {};
}

QList<QSsl::ImplementedClass> QTlsBackend::implementedClasses(const QString &backendName)
{
    if (!backends())
        return {};

    if (const auto *fct = backends->backend(backendName))
        return fct->implementedClasses();

    return {};
}

QT_END_NAMESPACE
