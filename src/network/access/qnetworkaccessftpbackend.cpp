/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qnetworkaccessftpbackend_p.h"
#include "qnetworkaccessmanager_p.h"
#include "QtNetwork/qauthenticator.h"
#include "private/qnoncontiguousbytedevice_p.h"
#include <QStringList>

QT_BEGIN_NAMESPACE

enum {
    DefaultFtpPort = 21
};

static QByteArray makeCacheKey(const QUrl &url)
{
    QUrl copy = url;
    copy.setPort(url.port(DefaultFtpPort));
    return "ftp-connection:" +
        copy.toEncoded(QUrl::RemovePassword | QUrl::RemovePath | QUrl::RemoveQuery |
                       QUrl::RemoveFragment);
}

QStringList QNetworkAccessFtpBackendFactory::supportedSchemes() const
{
    return QStringList(QStringLiteral("ftp"));
}

QNetworkAccessBackend *
QNetworkAccessFtpBackendFactory::create(QNetworkAccessManager::Operation op,
                                        const QNetworkRequest &request) const
{
    // is it an operation we know of?
    switch (op) {
    case QNetworkAccessManager::GetOperation:
    case QNetworkAccessManager::PutOperation:
        break;

    default:
        // no, we can't handle this operation
        return nullptr;
    }

    QUrl url = request.url();
    if (url.scheme().compare(QLatin1String("ftp"), Qt::CaseInsensitive) == 0)
        return new QNetworkAccessFtpBackend;
    return nullptr;
}

class QNetworkAccessCachedFtpConnection: public QFtp, public QNetworkAccessCache::CacheableObject
{
    // Q_OBJECT
public:
    QNetworkAccessCachedFtpConnection()
    {
        setExpires(true);
        setShareable(false);
    }

    void dispose() override
    {
        connect(this, SIGNAL(done(bool)), this, SLOT(deleteLater()));
        close();
    }

    using QFtp::clearError;
};

QNetworkAccessFtpBackend::QNetworkAccessFtpBackend()
    : ftp(nullptr), uploadDevice(nullptr), totalBytes(0), helpId(-1), sizeId(-1), mdtmId(-1), pwdId(-1),
    supportsSize(false), supportsMdtm(false), supportsPwd(false), state(Idle)
{
}

QNetworkAccessFtpBackend::~QNetworkAccessFtpBackend()
{
    //if backend destroyed while in use, then abort (this is the code path from QNetworkReply::abort)
    if (ftp && state != Disconnecting)
        ftp->abort();
    disconnectFromFtp(RemoveCachedConnection);
}

void QNetworkAccessFtpBackend::open()
{
#ifndef QT_NO_NETWORKPROXY
    QNetworkProxy proxy;
    const auto proxies = proxyList();
    for (const QNetworkProxy &p : proxies) {
        // use the first FTP proxy
        // or no proxy at all
        if (p.type() == QNetworkProxy::FtpCachingProxy
            || p.type() == QNetworkProxy::NoProxy) {
            proxy = p;
            break;
        }
    }

    // did we find an FTP proxy or a NoProxy?
    if (proxy.type() == QNetworkProxy::DefaultProxy) {
        // unsuitable proxies
        error(QNetworkReply::ProxyNotFoundError,
              tr("No suitable proxy found"));
        finished();
        return;
    }

#endif

    QUrl url = this->url();
    if (url.path().isEmpty()) {
        url.setPath(QLatin1String("/"));
        setUrl(url);
    }
    if (url.path().endsWith(QLatin1Char('/'))) {
        error(QNetworkReply::ContentOperationNotPermittedError,
              tr("Cannot open %1: is a directory").arg(url.toString()));
        finished();
        return;
    }
    state = LoggingIn;

    QNetworkAccessCache* objectCache = QNetworkAccessManagerPrivate::getObjectCache(this);
    QByteArray cacheKey = makeCacheKey(url);
    if (!objectCache->requestEntry(cacheKey, this,
                             SLOT(ftpConnectionReady(QNetworkAccessCache::CacheableObject*)))) {
        ftp = new QNetworkAccessCachedFtpConnection;
#ifndef QT_NO_BEARERMANAGEMENT // ### Qt6: Remove section
        //copy network session down to the QFtp
        ftp->setProperty("_q_networksession", property("_q_networksession"));
#endif
#ifndef QT_NO_NETWORKPROXY
        if (proxy.type() == QNetworkProxy::FtpCachingProxy)
            ftp->setProxy(proxy.hostName(), proxy.port());
#endif
        ftp->connectToHost(url.host(), url.port(DefaultFtpPort));
        ftp->login(url.userName(), url.password());

        objectCache->addEntry(cacheKey, ftp);
        ftpConnectionReady(ftp);
    }

    // Put operation
    if (operation() == QNetworkAccessManager::PutOperation) {
        uploadDevice = QNonContiguousByteDeviceFactory::wrap(createUploadByteDevice());
        uploadDevice->setParent(this);
    }
}

void QNetworkAccessFtpBackend::closeDownstreamChannel()
{
    state = Disconnecting;
    if (operation() == QNetworkAccessManager::GetOperation)
        ftp->abort();
}

void QNetworkAccessFtpBackend::downstreamReadyWrite()
{
    if (state == Transferring && ftp && ftp->bytesAvailable())
        ftpReadyRead();
}

void QNetworkAccessFtpBackend::ftpConnectionReady(QNetworkAccessCache::CacheableObject *o)
{
    ftp = static_cast<QNetworkAccessCachedFtpConnection *>(o);
    connect(ftp, SIGNAL(done(bool)), SLOT(ftpDone()));
    connect(ftp, SIGNAL(rawCommandReply(int,QString)), SLOT(ftpRawCommandReply(int,QString)));
    connect(ftp, SIGNAL(readyRead()), SLOT(ftpReadyRead()));

    // is the login process done already?
    if (ftp->state() == QFtp::LoggedIn)
        ftpDone();

    // no, defer the actual operation until after we've logged in
}

void QNetworkAccessFtpBackend::disconnectFromFtp(CacheCleanupMode mode)
{
    state = Disconnecting;

    if (ftp) {
        disconnect(ftp, nullptr, this, nullptr);

        QByteArray key = makeCacheKey(url());
        if (mode == RemoveCachedConnection) {
            QNetworkAccessManagerPrivate::getObjectCache(this)->removeEntry(key);
            ftp->dispose();
        } else {
            QNetworkAccessManagerPrivate::getObjectCache(this)->releaseEntry(key);
        }

        ftp = nullptr;
    }
}

void QNetworkAccessFtpBackend::ftpDone()
{
    // the last command we sent is done
    if (state == LoggingIn && ftp->state() != QFtp::LoggedIn) {
        if (ftp->state() == QFtp::Connected) {
            // the login did not succeed
            QUrl newUrl = url();
            QString userInfo = newUrl.userInfo();
            newUrl.setUserInfo(QString());
            setUrl(newUrl);

            QAuthenticator auth;
            authenticationRequired(&auth);

            if (!auth.isNull()) {
                // try again:
                newUrl.setUserName(auth.user());
                ftp->login(auth.user(), auth.password());
                return;
            }

            // Re insert the user info so that we can remove the cache entry.
            newUrl.setUserInfo(userInfo);
            setUrl(newUrl);

            error(QNetworkReply::AuthenticationRequiredError,
                  tr("Logging in to %1 failed: authentication required")
                  .arg(url().host()));
        } else {
            // we did not connect
            QNetworkReply::NetworkError code;
            switch (ftp->error()) {
            case QFtp::HostNotFound:
                code = QNetworkReply::HostNotFoundError;
                break;

            case QFtp::ConnectionRefused:
                code = QNetworkReply::ConnectionRefusedError;
                break;

            default:
                code = QNetworkReply::ProtocolFailure;
                break;
            }

            error(code, ftp->errorString());
        }

        // we're not connected, so remove the cache entry:
        disconnectFromFtp(RemoveCachedConnection);
        finished();
        return;
    }

    // check for errors:
    if (state == CheckingFeatures && ftp->error() == QFtp::UnknownError) {
        qWarning("QNetworkAccessFtpBackend: HELP command failed, ignoring it");
        ftp->clearError();
    } else if (ftp->error() != QFtp::NoError) {
        QString msg;
        if (operation() == QNetworkAccessManager::GetOperation)
            msg = tr("Error while downloading %1: %2");
        else
            msg = tr("Error while uploading %1: %2");
        msg = msg.arg(url().toString(), ftp->errorString());

        if (state == Statting)
            // file probably doesn't exist
            error(QNetworkReply::ContentNotFoundError,  msg);
        else
            error(QNetworkReply::ContentAccessDenied, msg);

        disconnectFromFtp(RemoveCachedConnection);
        finished();
    }

    if (state == LoggingIn) {
        state = CheckingFeatures;
        // send help command to find out if server supports SIZE, MDTM, and PWD
        if (operation() == QNetworkAccessManager::GetOperation
            || operation() == QNetworkAccessManager::PutOperation) {
            helpId = ftp->rawCommand(QLatin1String("HELP")); // get supported commands
        } else {
            ftpDone();
        }
    } else if (state == CheckingFeatures) {
        // If a URL path starts with // prefix (/%2F decoded), the resource will
        // be retrieved by an absolute path starting with the root directory.
        // For the other URLs, the working directory is retrieved by PWD command
        // and prepended to the resource path as an absolute path starting with
        // the working directory.
        state = ResolvingPath;
        QString path = url().path();
        if (path.startsWith(QLatin1String("//")) || supportsPwd == false) {
            ftpDone(); // no commands sent, move to the next state
        } else {
            // If a path starts with /~/ prefix, its prefix will be replaced by
            // the working directory as an absolute path starting with working
            // directory.
            if (path.startsWith(QLatin1String("/~/"))) {
                // Remove leading /~ symbols
                QUrl newUrl = url();
                newUrl.setPath(path.mid(2));
                setUrl(newUrl);
            }

            // send PWD command to retrieve the working directory
            pwdId = ftp->rawCommand(QLatin1String("PWD"));
        }
    } else if (state == ResolvingPath) {
        state = Statting;
        if (operation() == QNetworkAccessManager::GetOperation) {
            // logged in successfully, send the stat requests (if supported)
            const QString path = url().path();
            if (supportsSize) {
                ftp->rawCommand(QLatin1String("TYPE I"));
                sizeId = ftp->rawCommand(QLatin1String("SIZE ") + path); // get size
            }
            if (supportsMdtm)
                mdtmId = ftp->rawCommand(QLatin1String("MDTM ") + path); // get modified time
            if (!supportsSize && !supportsMdtm)
                ftpDone();      // no commands sent, move to the next state
        } else {
            ftpDone();
        }
    } else if (state == Statting) {
        // statted successfully, send the actual request
        metaDataChanged();
        state = Transferring;

        QFtp::TransferType type = QFtp::Binary;
        if (operation() == QNetworkAccessManager::GetOperation) {
            setCachingEnabled(true);
            ftp->get(url().path(), nullptr, type);
        } else {
            ftp->put(uploadDevice, url().path(), type);
        }

    } else if (state == Transferring) {
        // upload or download finished
        disconnectFromFtp();
        finished();
    }
}

void QNetworkAccessFtpBackend::ftpReadyRead()
{
    QByteArray data = ftp->readAll();
    QByteDataBuffer list;
    list.append(data);
    data.clear(); // important because of implicit sharing!
    writeDownstreamData(list);
}

void QNetworkAccessFtpBackend::ftpRawCommandReply(int code, const QString &text)
{
    //qDebug() << "FTP reply:" << code << text;
    int id = ftp->currentId();

    if ((id == helpId) && ((code == 200) || (code == 214))) {     // supported commands
        // the "FEAT" ftp command would be nice here, but it is not part of the
        // initial FTP RFC 959, neither ar "SIZE" nor "MDTM" (they are all specified
        // in RFC 3659)
        if (text.contains(QLatin1String("SIZE"), Qt::CaseSensitive))
            supportsSize = true;
        if (text.contains(QLatin1String("MDTM"), Qt::CaseSensitive))
            supportsMdtm = true;
        if (text.contains(QLatin1String("PWD"), Qt::CaseSensitive))
            supportsPwd = true;
    } else if (id == pwdId && code == 257) {
        QString pwdPath;
        int startIndex = text.indexOf('"');
        int stopIndex = text.lastIndexOf('"');
        if (stopIndex - startIndex) {
            // The working directory is a substring between \" symbols.
            startIndex++; // skip the first \" symbol
            pwdPath = text.mid(startIndex, stopIndex - startIndex);
        } else {
            // If there is no or only one \" symbol, use all the characters of
            // text.
            pwdPath = text;
        }

        // If a URL path starts with the working directory prefix, its resource
        // will be retrieved from the working directory. Otherwise, the path of
        // the working directory is prepended to the resource path.
        QString urlPath = url().path();
        if (!urlPath.startsWith(pwdPath)) {
            if (pwdPath.endsWith(QLatin1Char('/')))
                pwdPath.chop(1);
            // Prepend working directory to the URL path
            QUrl newUrl = url();
            newUrl.setPath(pwdPath % urlPath);
            setUrl(newUrl);
        }
    } else if (code == 213) {          // file status
        if (id == sizeId) {
            // reply to the size command
            setHeader(QNetworkRequest::ContentLengthHeader, text.toLongLong());
#if QT_CONFIG(datestring)
        } else if (id == mdtmId) {
            QDateTime dt = QDateTime::fromString(text, QLatin1String("yyyyMMddHHmmss"));
            setHeader(QNetworkRequest::LastModifiedHeader, dt);
#endif
        }
    }
}

QT_END_NAMESPACE
