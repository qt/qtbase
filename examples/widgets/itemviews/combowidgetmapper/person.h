// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PERSON_H
#define PERSON_H

#include <QtCore/QString>
#include <QtCore/QMetaObject>

//! [Person]
struct Person
{
private:
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString address MEMBER address)
    Q_PROPERTY(QString type MEMBER type)
public:

    QString name;
    QString address;
    QString type;
};
//! [Person]

#endif // PERSON_H
