// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MESSAGE_H
#define MESSAGE_H

#include <QMetaType>
#include <QStringList>

//! [custom type definition]
class Message
{
public:
    Message() = default;
    ~Message() = default;
    Message(const Message &) = default;
    Message &operator=(const Message &) = default;

    Message(const QString &body, const QStringList &headers);

    QString body() const;
    QStringList headers() const;

private:
    QString m_body;
    QStringList m_headers;
};
//! [custom type definition]

//! [custom type meta-type declaration]
Q_DECLARE_METATYPE(Message);
//! [custom type meta-type declaration]

#endif
