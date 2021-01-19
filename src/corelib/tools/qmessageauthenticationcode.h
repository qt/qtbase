/****************************************************************************
**
** Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
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

#ifndef QMESSAGEAUTHENTICATIONCODE_H
#define QMESSAGEAUTHENTICATIONCODE_H

#include <QtCore/qcryptographichash.h>

QT_BEGIN_NAMESPACE


class QMessageAuthenticationCodePrivate;
class QIODevice;

class Q_CORE_EXPORT QMessageAuthenticationCode
{
public:
    explicit QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                        const QByteArray &key = QByteArray());
    ~QMessageAuthenticationCode();

    void reset();

    void setKey(const QByteArray &key);

    void addData(const char *data, int length);
    void addData(const QByteArray &data);
    bool addData(QIODevice *device);

    QByteArray result() const;

    static QByteArray hash(const QByteArray &message, const QByteArray &key,
                           QCryptographicHash::Algorithm method);

private:
    Q_DISABLE_COPY(QMessageAuthenticationCode)
    QMessageAuthenticationCodePrivate *d;
};

QT_END_NAMESPACE

#endif
