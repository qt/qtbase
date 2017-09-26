/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
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

#ifndef QRANDOM_H
#define QRANDOM_H

#include <QtCore/qglobal.h>
#include <algorithm>    // for std::generate

QT_BEGIN_NAMESPACE

class QRandomGenerator
{
    // restrict the template parameters to unsigned integers 32 bits wide or larger
    template <typename UInt> using IfValidUInt =
        typename std::enable_if<std::is_unsigned<UInt>::value && sizeof(UInt) >= sizeof(uint), bool>::type;
public:
    QRandomGenerator() = default;

    // ### REMOVE BEFORE 5.10
    static quint32 get32() { return generate(); }
    static quint64 get64() { return generate64(); }
    static qreal getReal() { return generateDouble(); }

    static Q_CORE_EXPORT quint32 generate();
    static Q_CORE_EXPORT quint64 generate64();
    static double generateDouble()
    {
        // use get64() to get enough bits
        return double(generate64()) / ((std::numeric_limits<quint64>::max)() + double(1.0));
    }

    static qreal bounded(qreal sup)
    {
        return generateDouble() * sup;
    }

    static quint32 bounded(quint32 sup)
    {
        quint64 value = generate();
        value *= sup;
        value /= (max)() + quint64(1);
        return quint32(value);
    }

    static int bounded(int sup)
    {
        return int(bounded(quint32(sup)));
    }

    static quint32 bounded(quint32 min, quint32 sup)
    {
        return bounded(sup - min) + min;
    }

    static int bounded(int min, int sup)
    {
        return bounded(sup - min) + min;
    }

    template <typename UInt, IfValidUInt<UInt> = true>
    static void fillRange(UInt *buffer, qssize_t count)
    {
        fillRange_helper(buffer, buffer + count);
    }

    template <typename UInt, size_t N, IfValidUInt<UInt> = true>
    static void fillRange(UInt (&buffer)[N])
    {
        fillRange_helper(buffer, buffer + N);
    }

    // API like std::seed_seq
    template <typename ForwardIterator>
    void generate(ForwardIterator begin, ForwardIterator end)
    {
        auto generator = static_cast<quint32 (*)()>(&QRandomGenerator::generate);
        std::generate(begin, end, generator);
    }

    void generate(quint32 *begin, quint32 *end)
    {
        fillRange_helper(begin, end);
    }

    // API like std::random_device
    typedef quint32 result_type;
    result_type operator()() { return generate(); }
    double entropy() const Q_DECL_NOTHROW { return 0.0; }
    static Q_DECL_CONSTEXPR result_type min() { return (std::numeric_limits<result_type>::min)(); }
    static Q_DECL_CONSTEXPR result_type max() { return (std::numeric_limits<result_type>::max)(); }

private:
    Q_DISABLE_COPY(QRandomGenerator)
    static Q_CORE_EXPORT void fillRange_helper(void *buffer, void *bufferEnd);
};

class QRandomGenerator64
{
public:
    QRandomGenerator64() = default;

    static quint64 generate() { return QRandomGenerator::generate64(); }

    // API like std::random_device
    typedef quint64 result_type;
    result_type operator()() { return QRandomGenerator::generate64(); }
    double entropy() const Q_DECL_NOTHROW { return 0.0; }
    static Q_DECL_CONSTEXPR result_type min() { return (std::numeric_limits<result_type>::min)(); }
    static Q_DECL_CONSTEXPR result_type max() { return (std::numeric_limits<result_type>::max)(); }

private:
    Q_DISABLE_COPY(QRandomGenerator64)
};


QT_END_NAMESPACE

#endif // QRANDOM_H
