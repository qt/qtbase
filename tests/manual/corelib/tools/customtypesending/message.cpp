// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "message.h"

Message::Message(const QString &body, const QStringList &headers)
    : m_body(body), m_headers(headers)
{
}

QString Message::body() const
{
    return m_body;
}

QStringList Message::headers() const
{
    return m_headers;
}
