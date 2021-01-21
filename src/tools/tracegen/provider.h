/****************************************************************************
**
** Copyright (C) 2017 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Rafael Roquetto <rafael.roquetto@kdab.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
****************************************************************************/

#ifndef PROVIDER_H
#define PROVIDER_H

#include <qvector.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qtypeinfo.h>

struct Tracepoint
{
    struct Argument
    {
        QString type;
        QString name;
        int arrayLen;
    };

    struct Field
    {
        enum BackendType {
            Array,
            Sequence,
            Integer,
            IntegerHex,
            Float,
            String,
            Pointer,
            QtString,
            QtByteArray,
            QtUrl,
            QtRect,
            Unknown
        };

        BackendType backendType;
        QString paramType;
        QString name;
        int arrayLen;
        QString seqLen;
    };

    QString name;
    QVector<Argument> args;
    QVector<Field> fields;
};

struct Provider
{
    QString name;
    QVector<Tracepoint> tracepoints;
    QStringList prefixText;
};

Provider parseProvider(const QString &filename);

Q_DECLARE_TYPEINFO(Tracepoint::Argument, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint::Field, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(Tracepoint, Q_MOVABLE_TYPE);

#endif // PROVIDER_H
