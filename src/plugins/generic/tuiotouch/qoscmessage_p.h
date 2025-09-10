// Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOSCMESSAGE_P_H
#define QOSCMESSAGE_P_H

#include <QtCore/QByteArray>
#include <QtCore/QVariant>
#include <QtCore/QList>


QT_BEGIN_NAMESPACE

class QOscMessage
{
    QOscMessage(); // for QList, don't use
    friend class QList<QOscMessage>;
public:
    explicit QOscMessage(const QByteArray &data);

    bool isValid() const { return m_isValid; }

    QByteArray addressPattern() const { return m_addressPattern; }
    QVariantList arguments() const { return m_arguments; }

private:
    bool m_isValid;
    QByteArray m_addressPattern;
    QVariantList m_arguments;
};
Q_DECLARE_TYPEINFO(QOscMessage, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
