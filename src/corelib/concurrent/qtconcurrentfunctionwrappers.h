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

#endif //qdoc

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
