// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QCoreApplication>
#include <QDebug>
#include <QVariant>

//message.h

//! [custom type definition]
class Message
{
public:
    Message() = default;
    ~Message() = default;
    Message(const Message &) = default;
    Message &operator=(const Message &) = default;

    Message(const QString &body, const QStringList &headers);

    QStringView body() const;
    QStringList headers() const;

private:
    QString m_body;
    QStringList m_headers;
};
//! [custom type definition]

//! [custom type meta-type declaration]
Q_DECLARE_METATYPE(Message);
//! [custom type meta-type declaration]

//! [custom type streaming operator declaration]
QDebug operator<<(QDebug dbg, const Message &message);
//! [custom type streaming operator declaration]

// message.cpp

//! [custom type streaming operator]
QDebug operator<<(QDebug dbg, const Message &message)
{
    const QList<QStringView> pieces = message.body().split(u"\r\n", Qt::SkipEmptyParts);
    if (pieces.isEmpty())
        dbg.nospace() << "Message()";
    else if (pieces.size() == 1)
        dbg.nospace() << "Message(" << pieces.first() << ")";
    else
        dbg.nospace() << "Message(" << pieces.first() << " ...)";
    return dbg;
}
//! [custom type streaming operator]

//! [getter functions]
QStringView Message::body() const
{
    return m_body;
}

QStringList Message::headers() const
{
    return m_headers;
}
//! [getter functions]

//main.cpp

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList headers;
    headers << "Subject: Hello World"
            << "From: address@example.com";
    QString body = "This is a test.\r\n";
    //! [printing a custom type]
    Message message(body, headers);
    qDebug() << "Original:" << message;
    //! [printing a custom type]
    //! [storing a custom value]
    QVariant stored;
    stored.setValue(message);
    //! [storing a custom value]
    qDebug() << "Stored:" << stored;
    //! [retrieving a custom value]
    Message retrieved = qvariant_cast<Message>(stored);
    qDebug() << "Retrieved:" << retrieved;
    retrieved = qvariant_cast<Message>(stored);
    qDebug() << "Retrieved:" << retrieved;
    //! [retrieving a custom value]
    return 0;
}
