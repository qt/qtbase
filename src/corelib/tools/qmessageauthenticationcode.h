// Copyright (C) 2013 Ruslan Nigmatullin <euroelessar@yandex.ru>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMESSAGEAUTHENTICATIONCODE_H
#define QMESSAGEAUTHENTICATIONCODE_H

#include <QtCore/qcryptographichash.h>

QT_BEGIN_NAMESPACE


class QMessageAuthenticationCodePrivate;
class QIODevice;

// implemented in qcryptographichash.cpp
class Q_CORE_EXPORT QMessageAuthenticationCode
{
public:
    explicit QMessageAuthenticationCode(QCryptographicHash::Algorithm method,
                                        const QByteArray &key = QByteArray());
    QMessageAuthenticationCode(QMessageAuthenticationCode &&other) noexcept
        : d{std::exchange(other.d, nullptr)} {}
    ~QMessageAuthenticationCode();

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(QMessageAuthenticationCode)
    void swap(QMessageAuthenticationCode &other) noexcept
    { qt_ptr_swap(d, other.d); }

    void reset();

    void setKey(const QByteArray &key);

    void addData(const char *data, qsizetype length);
    void addData(const QByteArray &data);
    bool addData(QIODevice *device);

    QByteArrayView resultView() const noexcept;
    QByteArray result() const;

    static QByteArray hash(const QByteArray &message, const QByteArray &key,
                           QCryptographicHash::Algorithm method);

private:
    Q_DISABLE_COPY(QMessageAuthenticationCode)
    QMessageAuthenticationCodePrivate *d;
};

QT_END_NAMESPACE

#endif
