/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef QBINARYJSONOBJECT_H
#define QBINARYJSONOBJECT_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qbinaryjsonvalue_p.h"

QT_REQUIRE_CONFIG(binaryjson);

QT_BEGIN_NAMESPACE

class QBinaryJsonObject
{
    Q_DISABLE_COPY(QBinaryJsonObject)
public:
    QBinaryJsonObject() = default;
    ~QBinaryJsonObject();

    QBinaryJsonObject(QBinaryJsonObject &&other) noexcept
        : d(other.d), o(other.o)
    {
        other.d = nullptr;
        other.o = nullptr;
    }

    QBinaryJsonObject &operator =(QBinaryJsonObject &&other) noexcept
    {
        qSwap(d, other.d);
        qSwap(o, other.o);
        return *this;
    }

    static QBinaryJsonObject fromJsonObject(const QJsonObject &object);
    char *takeRawData(uint *size) const;

private:
    friend class QBinaryJsonValue;

    void insert(const QString &key, const QBinaryJsonValue &value);
    bool detach(uint reserve = 0);
    void compact();

    QBinaryJsonPrivate::MutableData *d = nullptr;
    QBinaryJsonPrivate::Object *o = nullptr;
};

QT_END_NAMESPACE

#endif // QBINARYJSONOBJECT_P_H
