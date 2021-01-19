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

#ifndef QMETATYPESWITCHER_P_H
#define QMETATYPESWITCHER_P_H

#include "qmetatype.h"

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

QT_BEGIN_NAMESPACE

class QMetaTypeSwitcher {
public:
    class NotBuiltinType;   // type is not a built-in type, but it may be a custom type or an unknown type
    class UnknownType;      // type not known to QMetaType system
    template<class ReturnType, class DelegateObject>
    static ReturnType switcher(DelegateObject &logic, int type, const void *data = nullptr);
};


#define QT_METATYPE_SWICHER_CASE(TypeName, TypeId, Name)\
    case QMetaType::TypeName: return logic.delegate(static_cast<Name const *>(data));

template<class ReturnType, class DelegateObject>
ReturnType QMetaTypeSwitcher::switcher(DelegateObject &logic, int type, const void *data)
{
    switch (QMetaType::Type(type)) {
    QT_FOR_EACH_STATIC_TYPE(QT_METATYPE_SWICHER_CASE)

    case QMetaType::UnknownType:
        return logic.delegate(static_cast<UnknownType const *>(data));
    default:
        if (type < QMetaType::User)
            return logic.delegate(static_cast<UnknownType const *>(data));
        return logic.delegate(static_cast<NotBuiltinType const *>(data));
    }
}

#undef QT_METATYPE_SWICHER_CASE

QT_END_NAMESPACE

#endif // QMETATYPESWITCHER_P_H
