/****************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QPPSATTRIBUTEPRIVATE_P_H
#define QPPSATTRIBUTEPRIVATE_P_H

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

#include "qppsattribute_p.h"

#include <QList>
#include <QMap>
#include <QSharedData>
#include <QString>
#include <QVariant>

QT_BEGIN_NAMESPACE

class QPpsAttributePrivate : public QSharedData
{
public:
    QPpsAttributePrivate();

    static QPpsAttribute createPpsAttribute(double value, QPpsAttribute::Flags flags);
    static QPpsAttribute createPpsAttribute(long long value, QPpsAttribute::Flags flags);
    static QPpsAttribute createPpsAttribute(int value, QPpsAttribute::Flags flags);
    static QPpsAttribute createPpsAttribute(bool value, QPpsAttribute::Flags flags);
    static QPpsAttribute createPpsAttribute(const QString &value, QPpsAttribute::Flags flags);
    static QPpsAttribute createPpsAttribute(const QPpsAttributeList &value,
                                            QPpsAttribute::Flags flags);
    static QPpsAttribute createPpsAttribute(const QPpsAttributeMap &value,
                                            QPpsAttribute::Flags flags);

private:
    friend class QPpsAttribute;

    QVariant data;
    QPpsAttribute::Type type;
    QPpsAttribute::Flags flags;
};

QT_END_NAMESPACE

#endif // QPPSATTRIBUTEPRIVATE_P_H
