/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef QMETATYPE_P_H
#define QMETATYPE_P_H

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

#include "qmetatype.h"

QT_BEGIN_NAMESPACE

enum { /* TYPEMODULEINFO flags */
    Q_CORE_TYPE = 1,
    Q_GUI_TYPE = 2,
    Q_WIDGET_TYPE = 3
};

template <typename T>
class QTypeModuleInfo
{
public:
    enum Module {
        IsCore = !QTypeInfo<T>::isComplex, // Primitive types are in Core
        IsWidget = false,
        IsGui = false,
        IsUnknown = !IsCore
    };
    static inline int module() { return IsCore ? Q_CORE_TYPE : 0; }
};

#define QT_ASSIGN_TYPE_TO_MODULE(TYPE, MODULE) \
template<> \
class QTypeModuleInfo<TYPE > \
{ \
public: \
    enum Module { \
        IsCore = (((MODULE) == (Q_CORE_TYPE))), \
        IsWidget = (((MODULE) == (Q_WIDGET_TYPE))), \
        IsGui = (((MODULE) == (Q_GUI_TYPE))), \
        IsUnknown = !(IsCore || IsWidget || IsGui) \
    }; \
    static inline int module() { return MODULE; } \
    Q_STATIC_ASSERT((IsUnknown && !(IsCore || IsWidget || IsGui)) \
                 || (IsCore && !(IsUnknown || IsWidget || IsGui)) \
                 || (IsWidget && !(IsUnknown || IsCore || IsGui)) \
                 || (IsGui && !(IsUnknown || IsCore || IsWidget))); \
};


#define QT_DECLARE_CORE_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, Q_CORE_TYPE);
#define QT_DECLARE_GUI_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, Q_GUI_TYPE);
#define QT_DECLARE_WIDGETS_MODULE_TYPES_ITER(TypeName, TypeId, Name) \
    QT_ASSIGN_TYPE_TO_MODULE(Name, Q_WIDGET_TYPE);

QT_FOR_EACH_STATIC_CORE_CLASS(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_CORE_TEMPLATE(QT_DECLARE_CORE_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_GUI_CLASS(QT_DECLARE_GUI_MODULE_TYPES_ITER)
QT_FOR_EACH_STATIC_WIDGETS_CLASS(QT_DECLARE_WIDGETS_MODULE_TYPES_ITER)

#undef QT_DECLARE_CORE_MODULE_TYPES_ITER
#undef QT_DECLARE_GUI_MODULE_TYPES_ITER
#undef QT_DECLARE_WIDGETS_MODULE_TYPES_ITER

class QMetaTypeInterface
{
private:
    template<typename T>
    struct Impl {
        static void *creator(const T *t)
        {
            if (t)
                return new T(*t);
            return new T();
        }

        static void deleter(T *t) { delete t; }
    #ifndef QT_NO_DATASTREAM
        static void saver(QDataStream &stream, const T *t) { stream << *t; }
        static void loader(QDataStream &stream, T *t) { stream >> *t; }
    #endif // QT_NO_DATASTREAM
        static void destructor(T *t)
        {
            Q_UNUSED(t) // Silence MSVC that warns for POD types.
            t->~T();
        }
        static void *constructor(void *where, const T *t)
        {
            if (t)
                return new (where) T(*static_cast<const T*>(t));
            return new (where) T;
        }
    };
public:
    template<typename T>
    QMetaTypeInterface(T * = 0)
        : creator(reinterpret_cast<QMetaType::Creator>(Impl<T>::creator))
              , deleter(reinterpret_cast<QMetaType::Deleter>(Impl<T>::deleter))
          #ifndef QT_NO_DATASTREAM
              , saveOp(reinterpret_cast<QMetaType::SaveOperator>(Impl<T>::saver))
              , loadOp(reinterpret_cast<QMetaType::LoadOperator>(Impl<T>::loader))
          #endif
              , constructor(reinterpret_cast<QMetaType::Constructor>(Impl<T>::constructor))
              , destructor(reinterpret_cast<QMetaType::Destructor>(Impl<T>::destructor))
              , size(sizeof(T))
    {}

    QMetaTypeInterface()
        : creator(0)
        , deleter(0)
    #ifndef QT_NO_DATASTREAM
        , saveOp(0)
        , loadOp(0)
    #endif
        , constructor(0)
        , destructor(0)
        , size(0)
    {}

    QMetaType::Creator creator;
    QMetaType::Deleter deleter;
#ifndef QT_NO_DATASTREAM
    QMetaType::SaveOperator saveOp;
    QMetaType::LoadOperator loadOp;
#endif
    QMetaType::Constructor constructor;
    QMetaType::Destructor destructor;
    int size;
};

QT_END_NAMESPACE

#endif // QMETATYPE_P_H
