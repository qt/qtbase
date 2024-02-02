// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "message.h"

#include <QDebug>

Message::Message(const QString &body, const QStringList &headers)
    : m_body(body), m_headers(headers)
{
}

//! [custom type streaming operator]
QDebug operator<<(QDebug dbg, const Message &message)
{
    QDebugStateSaver saver(dbg);
    QList<QStringView> pieces = message.body().split(u"\r\n", Qt::SkipEmptyParts);
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
