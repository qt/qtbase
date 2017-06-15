/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QENDIAN_P_H
#define QENDIAN_P_H

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

#include "qendian.h"

QT_BEGIN_NAMESPACE

template<class S>
class QSpecialInteger
{
    typedef typename S::StorageType T;
    T val;
public:
    QSpecialInteger() = default;
    explicit Q_DECL_CONSTEXPR QSpecialInteger(T i) : val(S::toSpecial(i)) {}

    QSpecialInteger &operator =(T i) { val = S::toSpecial(i); return *this; }
    operator T() const { return S::fromSpecial(val); }

    bool operator ==(QSpecialInteger<S> i) const { return val == i.val; }
    bool operator !=(QSpecialInteger<S> i) const { return val != i.val; }

    QSpecialInteger &operator +=(T i)
    {   return (*this = S::fromSpecial(val) + i); }
    QSpecialInteger &operator -=(T i)
    {   return (*this = S::fromSpecial(val) - i); }
    QSpecialInteger &operator *=(T i)
    {   return (*this = S::fromSpecial(val) * i); }
    QSpecialInteger &operator >>=(T i)
    {   return (*this = S::fromSpecial(val) >> i); }
    QSpecialInteger &operator <<=(T i)
    {   return (*this = S::fromSpecial(val) << i); }
    QSpecialInteger &operator /=(T i)
    {   return (*this = S::fromSpecial(val) / i); }
    QSpecialInteger &operator %=(T i)
    {   return (*this = S::fromSpecial(val) % i); }
    QSpecialInteger &operator |=(T i)
    {   return (*this = S::fromSpecial(val) | i); }
    QSpecialInteger &operator &=(T i)
    {   return (*this = S::fromSpecial(val) & i); }
    QSpecialInteger &operator ^=(T i)
    {   return (*this = S::fromSpecial(val) ^ i); }
};

template<typename T>
class QLittleEndianStorageType {
public:
    typedef T StorageType;
    static Q_DECL_CONSTEXPR T toSpecial(T source) { return qToLittleEndian(source); }
    static Q_DECL_CONSTEXPR T fromSpecial(T source) { return qFromLittleEndian(source); }
};

template<typename T>
class QBigEndianStorageType {
public:
    typedef T StorageType;
    static Q_DECL_CONSTEXPR T toSpecial(T source) { return qToBigEndian(source); }
    static Q_DECL_CONSTEXPR T fromSpecial(T source) { return qFromBigEndian(source); }
};

template<typename T>
using QLEInteger = QSpecialInteger<QLittleEndianStorageType<T>>;

template<typename T>
using QBEInteger = QSpecialInteger<QBigEndianStorageType<T>>;

template <typename T>
class QTypeInfo<QLEInteger<T> >
    : public QTypeInfoMerger<QLEInteger<T>, T> {};

template <typename T>
class QTypeInfo<QBEInteger<T> >
    : public QTypeInfoMerger<QBEInteger<T>, T> {};

typedef QLEInteger<qint16> qint16_le;
typedef QLEInteger<qint32> qint32_le;
typedef QLEInteger<qint64> qint64_le;
typedef QLEInteger<quint16> quint16_le;
typedef QLEInteger<quint32> quint32_le;
typedef QLEInteger<quint64> quint64_le;

typedef QBEInteger<qint16> qint16_be;
typedef QBEInteger<qint32> qint32_be;
typedef QBEInteger<qint64> qint64_be;
typedef QBEInteger<quint16> quint16_be;
typedef QBEInteger<quint32> quint32_be;
typedef QBEInteger<quint64> quint64_be;

QT_END_NAMESPACE

#endif // QENDIAN_P_H
