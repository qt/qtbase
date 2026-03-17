// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CONTACT_H
#define CONTACT_H

#include <QDataStream>
#include <QString>
#include <QtCompare>

//! [0]
struct Contact
{
    QString name;
    QString address;

    friend bool comparesEqual(const Contact &lhs, const Contact &rhs) noexcept
    {
        return lhs.name == rhs.name && lhs.address == rhs.address;
    }

    friend Qt::strong_ordering compareThreeWay(const Contact &lhs, const Contact  &rhs) noexcept
    {
        int cmp = lhs.name.compare(rhs.name);
        if (cmp == 0)
            cmp = lhs.address.compare(rhs.address);
        return Qt::compareThreeWay(cmp, 0);
    }

    Q_DECLARE_STRONGLY_ORDERED(Contact)
};

inline QDataStream &operator<<(QDataStream &stream, const Contact &contact)
{
    return stream << contact.name << contact.address;
}

inline QDataStream &operator>>(QDataStream &stream, Contact &contact)
{
    return stream >> contact.name >> contact.address;
}
//! [0]

#endif // CONTACT_H
