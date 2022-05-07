/****************************************************************************
**
** Copyright (C) 2014 Robin Burchell <robin.burchell@viroteck.net>
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

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
