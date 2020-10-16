/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
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
    ~QException() noexcept;
    virtual void raise() const;
    virtual QException *clone() const;
};

class QUnhandledExceptionPrivate;
class Q_CORE_EXPORT QUnhandledException final : public QException
{
public:
    QUnhandledException(std::exception_ptr exception = nullptr) noexcept;
    ~QUnhandledException() noexcept override;

    QUnhandledException(QUnhandledException &&other) noexcept;
    QUnhandledException(const QUnhandledException &other) noexcept;

    void swap(QUnhandledException &other) noexcept { qSwap(d, other.d); }

    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QUnhandledException)
    QUnhandledException &operator=(const QUnhandledException &other) noexcept;

    void raise() const override;
    QUnhandledException *clone() const override;

    std::exception_ptr exception() const;

private:
    QSharedDataPointer<QUnhandledExceptionPrivate> d;
};

namespace QtPrivate {

class Q_CORE_EXPORT ExceptionStore
{
public:
    void setException(const QException &e);
    void setException(std::exception_ptr e);
    bool hasException() const;
    std::exception_ptr exception() const;
    void throwPossibleException();
    std::exception_ptr exceptionHolder;
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
