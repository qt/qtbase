/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QTCONCURRENT_FUNCTIONWRAPPERS_H
#define QTCONCURRENT_FUNCTIONWRAPPERS_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_CONCURRENT

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef qdoc

namespace QtConcurrent {

template <typename T>
class FunctionWrapper0
{
public:
    typedef T (*FunctionPointerType)();
    typedef T result_type;
    inline FunctionWrapper0(FunctionPointerType _functionPointer)
    :functionPointer(_functionPointer) { }

    inline T operator()()
    {
        return functionPointer();
    }
private:
    FunctionPointerType functionPointer;
};

template <typename T, typename U>
class FunctionWrapper1
{
public:
    typedef T (*FunctionPointerType)(U u);
    typedef T result_type;
    inline FunctionWrapper1(FunctionPointerType _functionPointer)
    :functionPointer(_functionPointer) { }

    inline T operator()(U u)
    {
        return functionPointer(u);
    }

private:
    FunctionPointerType functionPointer;
};

template <typename T, typename U, typename V>
class FunctionWrapper2
{
public:
    typedef T (*FunctionPointerType)(U u, V v);
    typedef T result_type;
    inline FunctionWrapper2(FunctionPointerType _functionPointer)
    :functionPointer(_functionPointer) { }

    inline T operator()(U u, V v)
    {
        return functionPointer(u, v);
    }
private:
    FunctionPointerType functionPointer;
};

template <typename T, typename C>
class MemberFunctionWrapper
{
public:
    typedef T (C::*FunctionPointerType)();
    typedef T result_type;
    inline MemberFunctionWrapper(FunctionPointerType _functionPointer)
    :functionPointer(_functionPointer) { }

    inline T operator()(C &c)
    {
        return (c.*functionPointer)();
    }
private:
    FunctionPointerType functionPointer;
};

template <typename T, typename C, typename U>
class MemberFunctionWrapper1
{
public:
    typedef T (C::*FunctionPointerType)(U);
    typedef T result_type;

    inline MemberFunctionWrapper1(FunctionPointerType _functionPointer)
        : functionPointer(_functionPointer)
    { }

    inline T operator()(C &c, U u)
    {
        return (c.*functionPointer)(u);
    }

private:
    FunctionPointerType functionPointer;
};

template <typename T, typename C>
class ConstMemberFunctionWrapper
{
public:
    typedef T (C::*FunctionPointerType)() const;
    typedef T result_type;
    inline ConstMemberFunctionWrapper(FunctionPointerType _functionPointer)
    :functionPointer(_functionPointer) { }

    inline T operator()(const C &c) const
    {
        return (c.*functionPointer)();
    }
private:
    FunctionPointerType functionPointer;
};

} // namespace QtConcurrent.

namespace QtPrivate {

template <typename T>
const T& createFunctionWrapper(const T& t)
{
    return t;
}

template <typename T, typename U>
QtConcurrent::FunctionWrapper1<T, U> createFunctionWrapper(T (*func)(U))
{
    return QtConcurrent::FunctionWrapper1<T, U>(func);
}

template <typename T, typename C>
QtConcurrent::MemberFunctionWrapper<T, C> createFunctionWrapper(T (C::*func)())
{
    return QtConcurrent::MemberFunctionWrapper<T, C>(func);
}

template <typename T, typename C, typename U>
QtConcurrent::MemberFunctionWrapper1<T, C, U> createFunctionWrapper(T (C::*func)(U))
{
    return QtConcurrent::MemberFunctionWrapper1<T, C, U>(func);
}

template <typename T, typename C>
QtConcurrent::ConstMemberFunctionWrapper<T, C> createFunctionWrapper(T (C::*func)() const)
{
    return QtConcurrent::ConstMemberFunctionWrapper<T, C>(func);
}

template <class T>
struct ReduceResultType;

template <class U, class V>
struct ReduceResultType<void(*)(U&,V)>
{
    typedef U ResultType;
};

template <class T, class C, class U>
struct ReduceResultType<T(C::*)(U)>
{
    typedef C ResultType;
};

template <class InputSequence, class MapFunctor>
struct MapResultType
{
    typedef typename MapFunctor::result_type ResultType;
};

template <class InputSequence, class U, class V>
struct MapResultType<InputSequence, U (*)(V)>
{
    typedef U ResultType;
};

template <class InputSequence, class T, class C>
struct MapResultType<InputSequence, T(C::*)() const>
{
    typedef T ResultType;
};

#ifndef QT_NO_TEMPLATE_TEMPLATE_PARAMETERS

template <template <typename> class InputSequence, typename MapFunctor, typename T>
struct MapResultType<InputSequence<T>, MapFunctor>
{
    typedef InputSequence<typename MapFunctor::result_type> ResultType;
};

template <template <typename> class InputSequence, class T, class U, class V>
struct MapResultType<InputSequence<T>, U (*)(V)>
{
    typedef InputSequence<U> ResultType;
};

template <template <typename> class InputSequence, class T, class U, class C>
struct MapResultType<InputSequence<T>, U(C::*)() const>
{
    typedef InputSequence<U> ResultType;
};

template <template <typename, typename> class InputSequence, typename MapFunctor, typename T, typename T2>
struct MapResultType<InputSequence<T, T2>, MapFunctor>
{
    typedef InputSequence<typename MapFunctor::result_type, T2> ResultType;
};

template <template <typename, typename> class InputSequence, class T, typename T2, class U, class V>
struct MapResultType<InputSequence<T, T2>, U (*)(V)>
{
    typedef InputSequence<U, T2> ResultType;
};

template <template <typename, typename> class InputSequence, class T, typename T2, class U, class C>
struct MapResultType<InputSequence<T, T2>, U(C::*)() const>
{
    typedef InputSequence<U, T2> ResultType;
};

#endif // QT_NO_TEMPLATE_TEMPLATE_PARAMETER

template <class MapFunctor>
struct MapResultType<QStringList, MapFunctor>
{
    typedef QList<typename MapFunctor::result_type> ResultType;
};

template <class U, class V>
struct MapResultType<QStringList, U (*)(V)>
{
    typedef QList<U> ResultType;
};

template <class U, class C>
struct MapResultType<QStringList, U(C::*)() const>
{
    typedef QList<U> ResultType;
};

template <typename ReturnType, typename T>
struct DisableIfSame
{
    typedef ReturnType Type;
};

template <typename T>
struct DisableIfSame<T, T>
{

};

} // namespace QtPrivate.

#endif //qdoc

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
