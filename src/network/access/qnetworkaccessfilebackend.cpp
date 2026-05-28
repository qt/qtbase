// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qnetworkaccessfilebackend_p.h"
#include "qfileinfo.h"
#include "qdir.h"
#include "private/qnoncontiguousbytedevice_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QStringList QNetworkAccessFileBackendFactory::supportedSchemes() const
{
    QStringList schemes;
    schemes << QStringLiteral("file")
            << QStringLiteral("qrc");
#if defined(Q_OS_ANDROID)
    schemes << QStringLiteral("assets");
#endif
    return schemes;
}

QNetworkAccessBackend *
QNetworkAccessFileBackendFactory::create(QNetworkAccessManager::Operation op,
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
    if (url.scheme().compare("qrc"_L1, Qt::CaseInsensitive) == 0
#if defined(Q_OS_ANDROID)
            || url.scheme().compare("assets"_L1, Qt::CaseInsensitive) == 0
#endif
            || url.isLocalFile()) {
        return new QNetworkAccessFileBackend;
    } else if (!url.scheme().isEmpty() && url.authority().isEmpty() && (url.scheme().size() > 1)) {
        // check if QFile could, in theory, open this URL via the file engines
        // it has to be in the format:
        //    prefix:path/to/file
        // or prefix:/path/to/file
        //
        // this construct here must match the one below in open()
        QFileInfo fi(url.toString(QUrl::RemoveAuthority | QUrl::RemoveFragment | QUrl::RemoveQuery));
        if (fi.exists() || (op == QNetworkAccessManager::PutOperation && fi.dir().exists()))
            return new QNetworkAccessFileBackend;
    }

    return nullptr;
}

// We pass TargetType::Local even though it's kind of Networked but we're using a QFile to access
// the resource so it cannot use proxies anyway
QNetworkAccessFileBackend::QNetworkAccessFileBackend()
    : QNetworkAccessBackend(QNetworkAccessBackend::TargetType::Local),
      totalBytes(0),
      hasUploadFinished(false)
{
}

QNetworkAccessFileBackend::~QNetworkAccessFileBackend()
{
}

void QNetworkAccessFileBackend::open()
{
    QUrl url = this->url();

    if (url.host() == "localhost"_L1)
        url.setHost(QString());
#if !defined(Q_OS_WIN)
    // do not allow UNC paths on Unix
    if (!url.host().isEmpty()) {
        // we handle only local files
        error(QNetworkReply::ProtocolInvalidOperationError,
              QCoreApplication::translate("QNetworkAccessFileBackend", "Request for opening non-local file %1").arg(url.toString()));
        finished();
        return;
    }
#endif // !defined(Q_OS_WIN)
    if (url.path().isEmpty())
        url.setPath("/"_L1);
    setUrl(url);

    QString fileName = url.toLocalFile();
    if (fileName.isEmpty()) {
        if (url.scheme() == "qrc"_L1) {
            fileName = u':' + url.path();
        } else {
#if defined(Q_OS_ANDROID)
            if (url.scheme() == "assets"_L1)
                fileName = "assets:"_L1 + url.path();
            else
#endif
                fileName = url.toString(QUrl::RemoveAuthority | QUrl::RemoveFragment | QUrl::RemoveQuery);
        }
    }
    file.setFileName(fileName);

    if (operation() == QNetworkAccessManager::GetOperation) {
        if (!loadFileInfo())
            return;
    }

    QIODevice::OpenMode mode;
    switch (operation()) {
    case QNetworkAccessManager::GetOperation:
        mode = QIODevice::ReadOnly;
        break;
    case QNetworkAccessManager::PutOperation:
        mode = QIODevice::WriteOnly | QIODevice::Truncate;
        createUploadByteDevice();
        QObject::connect(uploadByteDevice(), SIGNAL(readyRead()), this, SLOT(uploadReadyReadSlot()));
        QMetaObject::invokeMethod(this, "uploadReadyReadSlot", Qt::QueuedConnection);
        break;
    default:
        Q_ASSERT_X(false, "QNetworkAccessFileBackend::open",
                   "Got a request operation I cannot handle!!");
        return;
    }

    mode |= QIODevice::Unbuffered;
    bool opened = file.open(mode);
    if (file.isSequential())
        connect(&file, &QIODevice::readChannelFinished, this, [this]() { finished(); });

    // could we open the file?
    if (!opened) {
        QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Error opening %1: %2")
                                                .arg(this->url().toString(), file.errorString());

        // why couldn't we open the file?
        // if we're opening for reading, either it doesn't exist, or it's access denied
        // if we're opening for writing, not existing means it's access denied too
        if (file.exists() || operation() == QNetworkAccessManager::PutOperation)
            error(QNetworkReply::ContentAccessDenied, msg);
        else
            error(QNetworkReply::ContentNotFoundError, msg);
        finished();
    }
}

void QNetworkAccessFileBackend::uploadReadyReadSlot()
{
    if (hasUploadFinished)
        return;

    forever {
        QByteArray data(16 * 1024, Qt::Uninitialized);
        qint64 haveRead = uploadByteDevice()->peek(data.data(), data.size());
        if (haveRead == -1) {
            // EOF
            hasUploadFinished = true;
            file.flush();
            file.close();
            finished();
            break;
        } else if (haveRead == 0) {
            // nothing to read right now, we will be called again later
            break;
        } else {
            qint64 haveWritten;
            data.truncate(haveRead);
            haveWritten = file.write(data);

            if (haveWritten < 0) {
                // write error!
                QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Write error writing to %1: %2")
                              .arg(url().toString(), file.errorString());
                error(QNetworkReply::ProtocolFailure, msg);

                finished();
                return;
            } else {
                uploadByteDevice()->skip(haveWritten);
            }


            file.flush();
        }
    }
}

void QNetworkAccessFileBackend::close()
{
    if (operation() == QNetworkAccessManager::GetOperation) {
        file.close();
    }
}

bool QNetworkAccessFileBackend::loadFileInfo()
{
    QFileInfo fi(file);
    setHeader(QNetworkRequest::LastModifiedHeader, fi.lastModified());
    setHeader(QNetworkRequest::ContentLengthHeader, fi.size());

    // signal we're open
    metaDataChanged();

    if (fi.isDir()) {
        error(QNetworkReply::ContentOperationNotPermittedError,
              QCoreApplication::translate("QNetworkAccessFileBackend", "Cannot open %1: Path is a directory").arg(url().toString()));
        finished();
        return false;
    }

    return true;
}

qint64 QNetworkAccessFileBackend::bytesAvailable() const
{
    if (operation() != QNetworkAccessManager::GetOperation)
        return 0;
    return file.bytesAvailable();
}

qint64 QNetworkAccessFileBackend::read(char *data, qint64 maxlen)
{
    if (operation() != QNetworkAccessManager::GetOperation)
        return 0;
    qint64 actuallyRead = file.read(data, maxlen);
    if (actuallyRead <= 0) {
        // EOF or error
        if (file.error() != QFile::NoError) {
            QString msg = QCoreApplication::translate("QNetworkAccessFileBackend", "Read error reading from %1: %2")
                            .arg(url().toString(), file.errorString());
            error(QNetworkReply::ProtocolFailure, msg);

            finished();
            return -1;
        }

        finished();
        return actuallyRead;
    }
    if (!file.isSequential() && file.atEnd())
        finished();
    totalBytes += actuallyRead;
    return actuallyRead;
}

QT_END_NAMESPACE

#include "moc_qnetworkaccessfilebackend_p.cpp"
