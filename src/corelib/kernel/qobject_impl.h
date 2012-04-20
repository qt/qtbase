/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef Q_QDOC

#ifndef QOBJECT_H
#error Do not include qobject_impl.h directly
#endif

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


namespace QtPrivate {
    /*
        Logic to statically generate the array of qMetaTypeId
        ConnectionTypes<FunctionPointer<Signal>::Arguments>::types() returns an array
        of int that is suitable for the types arguments of the connection functions.

        The array only exist of all the types are declared as a metatype
        (detected using the TypesAreDeclaredMetaType helper struct)
        If one of the type is not declared, the function return 0 and the signal
        cannot be used in queued connection.
    */
#ifndef Q_COMPILER_VARIADIC_TEMPLATES
    template <typename ArgList> struct TypesAreDeclaredMetaType { enum { Value = false }; };
    template <> struct TypesAreDeclaredMetaType<void> { enum { Value = true }; };
    template <typename Arg, typename Tail> struct TypesAreDeclaredMetaType<List<Arg, Tail> > { enum { Value = QMetaTypeId2<Arg>::Defined && TypesAreDeclaredMetaType<Tail>::Value }; };

    template <typename ArgList, bool Declared = TypesAreDeclaredMetaType<ArgList>::Value > struct ConnectionTypes
    { static const int *types() { return 0; } };
    template <> struct ConnectionTypes<void, true>
    { static const int *types() { static const int t[1] = { 0 }; return t; } };
    template <typename Arg1> struct ConnectionTypes<List<Arg1, void>, true>
    { static const int *types() { static const int t[2] = { QtPrivate::QMetaTypeIdHelper<Arg1>::qt_metatype_id(), 0 }; return t; } };
    template <typename Arg1, typename Arg2> struct ConnectionTypes<List<Arg1, List<Arg2, void> >, true>
    { static const int *types() { static const int t[3] = { QtPrivate::QMetaTypeIdHelper<Arg1>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg2>::qt_metatype_id(), 0 }; return t; } };
    template <typename Arg1, typename Arg2, typename Arg3> struct ConnectionTypes<List<Arg1, List<Arg2,  List<Arg3, void> > >, true>
    { static const int *types() { static const int t[4] = { QtPrivate::QMetaTypeIdHelper<Arg1>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg2>::qt_metatype_id(),
                                                            QtPrivate::QMetaTypeIdHelper<Arg3>::qt_metatype_id(), 0 }; return t; } };
    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct ConnectionTypes<List<Arg1, List<Arg2,  List<Arg3, List<Arg4, void> > > >, true>
    { static const int *types() { static const int t[4] = { QtPrivate::QMetaTypeIdHelper<Arg1>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg2>::qt_metatype_id(),
                QtPrivate::QMetaTypeIdHelper<Arg3>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg4>::qt_metatype_id(), 0 }; return t; } };
    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct ConnectionTypes<List<Arg1, List<Arg2,  List<Arg3, List<Arg4, List<Arg5, void> > > > >, true>
    { static const int *types() { static const int t[4] = { QtPrivate::QMetaTypeIdHelper<Arg1>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg2>::qt_metatype_id(),
                QtPrivate::QMetaTypeIdHelper<Arg3>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg4>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg5>::qt_metatype_id(), 0 }; return t; } };
    template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    struct ConnectionTypes<List<Arg1, List<Arg2,  List<Arg3, List<Arg4, List<Arg5, List<Arg6, void> > > > > >, true>
    { static const int *types() { static const int t[4] = { QtPrivate::QMetaTypeIdHelper<Arg1>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg2>::qt_metatype_id(),
            QtPrivate::QMetaTypeIdHelper<Arg3>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg4>::qt_metatype_id(), QtPrivate::QMetaTypeIdHelper<Arg5>::qt_metatype_id(),
            QtPrivate::QMetaTypeIdHelper<Arg6>::qt_metatype_id(), 0 }; return t; } };
#else
    template <typename ArgList> struct TypesAreDeclaredMetaType { enum { Value = false }; };
    template <> struct TypesAreDeclaredMetaType<List<>> { enum { Value = true }; };
    template <typename Arg, typename... Tail> struct TypesAreDeclaredMetaType<List<Arg, Tail...> >
    { enum { Value = QMetaTypeId2<Arg>::Defined && TypesAreDeclaredMetaType<List<Tail...>>::Value }; };

    template <typename ArgList, bool Declared = TypesAreDeclaredMetaType<ArgList>::Value > struct ConnectionTypes
    { static const int *types() { return 0; } };
    template <typename... Args> struct ConnectionTypes<List<Args...>, true>
    { static const int *types() { static const int t[sizeof...(Args) + 1] = { (QtPrivate::QMetaTypeIdHelper<Args>::qt_metatype_id())..., 0 }; return t; } };
#endif
}


QT_END_NAMESPACE

QT_END_HEADER

#endif
