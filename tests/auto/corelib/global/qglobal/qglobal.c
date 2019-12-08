/****************************************************************************
**
** Copyright (C) 2017 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qglobal.h>

#if __has_include(<stdbool.h>) || __STDC_VERSION__ >= 199901L
#  include <stdbool.h>
#else
#  undef true
#  define true 1
#  undef false
#  define false 0
#endif

#ifdef Q_COMPILER_THREAD_LOCAL
#  include <threads.h>
#endif

/*
 * Certain features of qglobal.h must work in C mode too. We test that
 * everything works.
 */

/* Types and Q_UNUSED */
void tst_GlobalTypes()
{
    qint8 s8;
    qint16 s16;
    qint32 s32;
    qint64 s64;
    qlonglong sll;
    Q_UNUSED(s8); Q_UNUSED(s16); Q_UNUSED(s32); Q_UNUSED(s64); Q_UNUSED(sll);

    quint8 u8;
    quint16 u16;
    quint32 u32;
    quint64 u64;
    qulonglong ull;
    Q_UNUSED(u8); Q_UNUSED(u16); Q_UNUSED(u32); Q_UNUSED(u64); Q_UNUSED(ull);

    uchar uc;
    ushort us;
    uint ui;
    ulong ul;
    Q_UNUSED(uc); Q_UNUSED(us); Q_UNUSED(ui); Q_UNUSED(ul);

    qreal qr;
    Q_UNUSED(qr);

    qsizetype qs;
    qptrdiff qp;
    qintptr qip;
    quintptr qup;
    Q_UNUSED(qs); Q_UNUSED(qp); Q_UNUSED(qip); Q_UNUSED(qup);
}

/* Qt version */
int tst_QtVersion()
{
    return QT_VERSION;
}

const char *tst_qVersion() Q_DECL_NOEXCEPT
{
#if !defined(QT_NAMESPACE)
    return qVersion();
#else
    return NULL;
#endif
}

/* Static assertion */
Q_STATIC_ASSERT(true);
Q_STATIC_ASSERT(1);
Q_STATIC_ASSERT_X(true, "Message");
Q_STATIC_ASSERT_X(1, "Message");

Q_STATIC_ASSERT(!false);
Q_STATIC_ASSERT(!0);

Q_STATIC_ASSERT(!!true);
Q_STATIC_ASSERT(!!1);

#ifdef Q_COMPILER_THREAD_LOCAL
static thread_local int gt_var;
void thread_local_test()
{
    static thread_local int t_var;
    t_var = gt_var;
}
#endif

