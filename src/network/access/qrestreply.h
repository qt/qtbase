// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QRESTREPLY_H
#define QRESTREPLY_H

#include <QtNetwork/qnetworkreply.h>

QT_BEGIN_NAMESPACE

class QDebug;
class QRestReplyPrivate;
class Q_NETWORK_EXPORT QRestReply : public QObject
{
    Q_OBJECT

public:
    ~QRestReply() override;

    QNetworkReply *networkReply() const;

    std::optional<QJsonDocument> json();
    QByteArray body();
    QString text();

    bool isSuccess() const
    {
        return !hasError() && isHttpStatusSuccess();
    }
    int httpStatus() const;
    bool isHttpStatusSuccess() const;

    bool hasError() const;
    QNetworkReply::NetworkError error() const;
    QString errorString() const;

    bool isFinished() const;
    qint64 bytesAvailable() const;

public Q_SLOTS:
    void abort();

Q_SIGNALS:
    void finished(QRestReply *reply);
    void errorOccurred(QRestReply *reply);
    void readyRead(QRestReply *reply);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal, QRestReply *reply);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal, QRestReply* reply);

private:
    friend class QRestAccessManagerPrivate;
#ifndef QT_NO_DEBUG_STREAM
    friend Q_NETWORK_EXPORT QDebug operator<<(QDebug debug, const QRestReply *reply);
#endif
    explicit QRestReply(QNetworkReply *reply, QObject *parent = nullptr);
    Q_DECLARE_PRIVATE(QRestReply)
    Q_DISABLE_COPY_MOVE(QRestReply)
};

QT_END_NAMESPACE

#endif // QRESTREPLY_H
