// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MAILHEADER_H
#define MAILHEADER_H

#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QTime>

//! [0]
struct MailHeader
{
private:
    Q_GADGET
    Q_PROPERTY(QString subject MEMBER subject)
    Q_PROPERTY(QString sender MEMBER sender)
    Q_PROPERTY(QDateTime date MEMBER date)

public:
    QString subject;
    QString sender;
    QDateTime date;
};
//! [0]

#endif // MAILHEADER_H
