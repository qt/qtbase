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

#ifndef QTCORE_QEXCEPTION_H
#define QTCORE_QEXCEPTION_H

#include <QtCore/qatomic.h>
#include <QtCore/qshareddata.h>

#ifndef QT_NO_EXCEPTIONS
#  include <exception>
#endif

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE


#if !defined(QT_NO_EXCEPTIONS) || defined(Q_CLANG_QDOC)

class Q_CORE_EXPORT QException : public std::exception
{
public:
    ~QException()
#ifdef Q_COMPILER_NOEXCEPT
    noexcept
#else
    throw()
#endif
    ;
    virtual void raise() const;
    virtual QException *clone() const;
};

class Q_CORE_EXPORT QUnhandledException : public QException
{
public:
    ~QUnhandledException()
#ifdef Q_COMPILER_NOEXCEPT
    noexcept
#else
    throw()
#endif
    ;
    void raise() const override;
    QUnhandledException *clone() const override;
};

namespace QtPrivate {

class Base;
class Q_CORE_EXPORT ExceptionHolder
{
public:
    ExceptionHolder(QException *exception = nullptr);
    ExceptionHolder(const ExceptionHolder &other);
    void operator=(const ExceptionHolder &other); // ### Qt6: copy-assign operator shouldn't return void. Remove this method and the copy-ctor, they are unneeded.
    ~ExceptionHolder();
    QException *exception() const;
    QExplicitlySharedDataPointer<Base> base;
};

class Q_CORE_EXPORT ExceptionStore
{
public:
    void setException(const QException &e);
    bool hasException() const;
    ExceptionHolder exception();
    void throwPossibleException();
    bool hasThrown() const;
    ExceptionHolder exceptionHolder;
};

} // namespace QtPrivate

#else // QT_NO_EXCEPTIONS

namespace QtPrivate {

class Q_CORE_EXPORT ExceptionStore
{
public:
    ExceptionStore() { }
    inline void throwPossibleException() {}
};

} // namespace QtPrivate

#endif // QT_NO_EXCEPTIONS

QT_END_NAMESPACE

#endif
