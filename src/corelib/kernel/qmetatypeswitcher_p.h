/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
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

    typedef void *UnknownType;
    template<class ReturnType, class DelegateObject>
    static ReturnType switcher(DelegateObject &logic, int type, const void *data);
};


#define QT_METATYPE_SWICHER_CASE_PRIMITIVE(TypeName, TypeId, Name)\
    case QMetaType::TypeName: return logic.delegate(static_cast<const Name *>(data));

#define QT_METATYPE_SWICHER_CASE_PRIMITIVE_POINTER(TypeName, TypeId, Name)\
    case QMetaType::TypeName: return logic.delegate(static_cast< Name * const *>(data));

#define QT_METATYPE_SWICHER_CASE_POINTER(TypeName, TypeId, Name)\
    case QMetaType::TypeName: return logic.delegate(static_cast< QT_PREPEND_NAMESPACE(Name) * const *>(data));

#define QT_METATYPE_SWICHER_CASE_QCLASS(TypeName, TypeId, Name)\
    case QMetaType::TypeName: return logic.delegate(static_cast<const QT_PREPEND_NAMESPACE(Name) *>(data));

template<class ReturnType, class DelegateObject>
ReturnType QMetaTypeSwitcher::switcher(DelegateObject &logic, int type, const void *data)
{
    switch (QMetaType::Type(type)) {
    QT_FOR_EACH_STATIC_PRIMITIVE_TYPE(QT_METATYPE_SWICHER_CASE_PRIMITIVE)
    QT_FOR_EACH_STATIC_PRIMITIVE_POINTER(QT_METATYPE_SWICHER_CASE_PRIMITIVE_POINTER)
    QT_FOR_EACH_STATIC_CORE_POINTER(QT_METATYPE_SWICHER_CASE_POINTER)
    QT_FOR_EACH_STATIC_CORE_CLASS(QT_METATYPE_SWICHER_CASE_QCLASS)
    QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_METATYPE_SWICHER_CASE_QCLASS)
    QT_FOR_EACH_STATIC_GUI_CLASS(QT_METATYPE_SWICHER_CASE_QCLASS)
    QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_METATYPE_SWICHER_CASE_QCLASS)

    default:
        return logic.delegate(static_cast<const UnknownType *>(data));
    }
}

#undef QT_METATYPE_SWICHER_CASE_PRIMITIVE
#undef QT_METATYPE_SWICHER_CASE_PRIMITIVE_POINTER
#undef QT_METATYPE_SWICHER_CASE_QCLASS
#undef QT_METATYPE_SWICHER_CASE_POINTER

QT_END_NAMESPACE

#endif // QMETATYPESWITCHER_P_H
