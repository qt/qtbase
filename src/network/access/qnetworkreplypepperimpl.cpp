/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qnetworkreplypepperimpl_p.h"

#include "QtCore/qdatetime.h"
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QDebug>
#include <qglobal.h>
#include <cstring>

#include "ppapi/cpp/message_loop.h"

using namespace pp;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_NETWORK, "qt.platform.pepper.network")

const int ReadBufferSize = 32768;
extern void *qtPepperInstance; // extern pp::instance pointer set by the pepper platform plugin

QNetworkReplyPepperImpl::QNetworkReplyPepperImpl(QObject *parent, const QNetworkRequest &req, const QNetworkAccessManager::Operation op)
    : QNetworkReply(*new QNetworkReplyPepperImplPrivate(this), parent)
    , m_instance(reinterpret_cast<pp::Instance*>(qtPepperInstance))
    , m_urlRequest(m_instance)
    , m_urlLoader(m_instance)
    , callbackFactory(this)
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "Construct QNetworkReplyPepperImpl for url" << req.url();

    // QNetworkReply bookkeeping
    setRequest(req);
    setUrl(req.url());
    setOperation(op);

    // Handle error conditions
    if (!qtPepperInstance) {
        qCWarning(QT_PLATFORM_PEPPER_NETWORK) << "pepper platform plugin is not started; network operations will fail";
        fail();
        return;
    }
    if (pp::MessageLoop::GetCurrent().is_null()) {
        qCWarning(QT_PLATFORM_PEPPER_NETWORK) << "no pp::MessageLoop for the current thread; network operations will fail";
        fail();
        return;
    }

    // Configure and open the pepper url request.
    m_urlRequest.SetURL(d->url.toString().toStdString());
    m_urlRequest.SetMethod("GET");
    m_urlRequest.SetRecordDownloadProgress(true);
    pp::CompletionCallback openCallback = callbackFactory.NewCallback(&QNetworkReplyPepperImpl::onOpen);
    m_urlLoader.Open(m_urlRequest, openCallback);
}

QNetworkReplyPepperImpl::~QNetworkReplyPepperImpl()
{

}

void QNetworkReplyPepperImpl::onOpen(int32_t result)
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "onOpen" << d->url << result;

    if (result != PP_OK) {
        fail();
        return;
    }

    // Get status
    pp::URLResponseInfo response = m_urlLoader.GetResponseInfo();
    int httpCode = response.GetStatusCode();
    int64_t sofar = 0;
    int64_t total = 0;
    m_urlLoader.GetDownloadProgress(&sofar, &total);
    m_urlRequest.SetRecordDownloadProgress(false);

    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "onOpen" << d->url << "callback result" << result
                                        << "http status" << httpCode << "download size" << total;

    // The current buffer implementation is designed to hold the entire data downlod
    // in memory. This can be optimized for less memory usage by dropping data
    // read off the QNetworkReply.
    d->buffer.reserve(total);
    d->buffer.resize(ReadBufferSize);

    // Start reading data bytes
    read();
}

void QNetworkReplyPepperImpl::read()
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "read()" << d->url;

    pp::CompletionCallback readCallback = callbackFactory.NewCallback(&QNetworkReplyPepperImpl::onRead);
    int32_t result = PP_OK;
    do {
        char * buffer = d->buffer.data() + d->writeOffset;
        result = m_urlLoader.ReadResponseBody(buffer, ReadBufferSize, readCallback);

      // Handle streaming data directly. Note that we *don't* want to call
      // OnRead here, since in the case of result > 0 it will schedule
      // another call to this function. If the network is very fast, we could
      // end up with a deeply recursive stack.
      if (result > 0) {
          qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "read byes" << result;
          appendData(result);
      }
    } while (result > 0);

    if (result != PP_OK_COMPLETIONPENDING) {
      // Either we reached the end of the stream (result == PP_OK) or there was
      // an error. We want OnRead to get called no matter what to handle
      // that case, whether the error is synchronous or asynchronous. If the
      // result code *is* COMPLETIONPENDING, our callback will be called
      // asynchronously.
      readCallback.Run(result);
    }
}

void QNetworkReplyPepperImpl::onRead(int32_t result)
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "onRead" << d->url << result;

    if (result == PP_OK) {
        qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "finished" << d->url << d->writeOffset;
        setFinished(true);
        QNetworkReply::open(QIODevice::ReadOnly); // ### ???
        setHeader(QNetworkRequest::ContentLengthHeader, d->writeOffset);
        QMetaObject::invokeMethod(this, "finished");
    } else if (result > 0) {
        // The URLLoader just filled "result" number of bytes into our buffer.
        // Save them and perform another read.
        appendData(result);
        read();
    } else {
        // A read error occurred.
        fail();
    }
}

void QNetworkReplyPepperImpl::appendData(int32_t size)
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "buffering" << size << "bytes";

    // Adjust buffer size and offset
    d->writeOffset += size;
    d->buffer.resize(d->buffer.size() + size);

    QMetaObject::invokeMethod(this, "downloadProgress",
        Q_ARG(qint64, d->writeOffset), Q_ARG(qint64, d->writeOffset));
    QMetaObject::invokeMethod(this, "readyRead");
}

void QNetworkReplyPepperImpl::fail()
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "fail" << d->url;

    QString msg = QCoreApplication::translate("QNetworkAccessPepperBackend", "Error opening %1")
            .arg(d->url.toString());
    setError(QNetworkReply::ContentNotFoundError, msg);
    QMetaObject::invokeMethod(this, "error",
        Q_ARG(QNetworkReply::NetworkError, QNetworkReply::ContentNotFoundError));
    QMetaObject::invokeMethod(this, "finished");
}

void QNetworkReplyPepperImpl::close()
{
    Q_D(QNetworkReplyPepperImpl);
    qCDebug(QT_PLATFORM_PEPPER_NETWORK) << "close";

    QNetworkReply::close();
}

void QNetworkReplyPepperImpl::abort()
{
    Q_D(QNetworkReplyPepperImpl);
    QNetworkReply::close();
}

qint64 QNetworkReplyPepperImpl::bytesAvailable() const
{
    Q_D(const QNetworkReplyPepperImpl);
    quint64 available = QNetworkReply::bytesAvailable() + d->writeOffset - d->readOffset;
    return available;
}

bool QNetworkReplyPepperImpl::isSequential () const
{
    return false;
}

qint64 QNetworkReplyPepperImpl::size() const
{
    Q_D(const QNetworkReplyPepperImpl);
    return d->writeOffset;
}

/*!
    \internal
*/
qint64 QNetworkReplyPepperImpl::readData(char *data, qint64 maxlen)
{
    Q_D(QNetworkReplyPepperImpl);

    qint64 available = d->writeOffset - d->readOffset;

    if (available <= 0)
        return -1;

    qint64 toRead = qMin(maxlen, available);
    std::memcpy(data, d->buffer.constData() + d->readOffset, toRead);
    d->readOffset += toRead;

    return toRead;
}

QNetworkReplyPepperImplPrivate::QNetworkReplyPepperImplPrivate(QNetworkReplyPepperImpl *q)
    : QNetworkReplyPrivate()
    , readOffset(0)
    , writeOffset(0)
{

}

QT_END_NAMESPACE

